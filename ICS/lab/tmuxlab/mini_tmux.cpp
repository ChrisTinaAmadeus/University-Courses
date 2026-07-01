#include <algorithm>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace {

constexpr uint32_t kMsgHello = 1;
constexpr uint32_t kMsgHelloAck = 2;
constexpr uint32_t kMsgInput = 3;
constexpr uint32_t kMsgOutput = 4;
constexpr uint32_t kMsgCmd = 5;
constexpr uint32_t kMsgDetach = 6;
constexpr uint32_t kMsgResize = 7;
constexpr uint32_t kMsgError = 8;

constexpr uint32_t kFlagReadonly = 1u << 0;
constexpr uint32_t kFlagCreateNewSession = 1u << 1;

constexpr size_t kMaxFramePayload = 1u << 20;
constexpr size_t kCaptureMaxLines = 1000;
constexpr size_t kReplayHistoryMaxBytes = 256 * 1024;

struct FrameHeader {
    uint32_t type;
    uint32_t flags;
    uint32_t value;
    uint32_t length;
};

static_assert(sizeof(FrameHeader) == 16, "Unexpected frame header size");

int g_sigchld_write_fd = -1;
int g_sigwinch_write_fd = -1;

void sigchld_handler(int) {
    if (g_sigchld_write_fd < 0) {
        return;
    }
    int saved = errno;
    const uint8_t b = 1;
    ssize_t rc = write(g_sigchld_write_fd, &b, 1);
    (void)rc;
    errno = saved;
}

void sigwinch_handler(int) {
    if (g_sigwinch_write_fd < 0) {
        return;
    }
    int saved = errno;
    const uint8_t b = 1;
    ssize_t rc = write(g_sigwinch_write_fd, &b, 1);
    (void)rc;
    errno = saved;
}

bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return false;
    }
    return true;
}

void close_fd(int& fd) {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

bool write_all_nb(int fd, const uint8_t* data, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, data + off, len - off);
        if (n > 0) {
            off += static_cast<size_t>(n);
            continue;
        }
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            pollfd pfd{};
            pfd.fd = fd;
            pfd.events = POLLOUT;
            int pr = poll(&pfd, 1, 1000);
            if (pr < 0 && errno == EINTR) {
                continue;
            }
            if (pr <= 0) {
                return false;
            }
            continue;
        }
        return false;
    }
    return true;
}

void write_best_effort_nb(int fd, const uint8_t* data, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, data + off, len - off);
        if (n > 0) {
            off += static_cast<size_t>(n);
            continue;
        }
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        }
        break;
    }
}

bool send_frame(int fd, uint32_t type, uint32_t flags, uint32_t value, const std::vector<uint8_t>& payload) {
    FrameHeader hdr{};
    hdr.type = type;
    hdr.flags = flags;
    hdr.value = value;
    hdr.length = static_cast<uint32_t>(payload.size());

    if (!write_all_nb(fd, reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr))) {
        return false;
    }
    if (!payload.empty()) {
        if (!write_all_nb(fd, payload.data(), payload.size())) {
            return false;
        }
    }
    return true;
}

bool send_frame_str(int fd, uint32_t type, uint32_t flags, uint32_t value, const std::string& s) {
    std::vector<uint8_t> payload(s.begin(), s.end());
    return send_frame(fd, type, flags, value, payload);
}

bool read_into_buffer(int fd, std::vector<uint8_t>& buf, bool& peer_closed) {
    peer_closed = false;
    std::array<uint8_t, 65536> tmp{};
    while (true) {
        ssize_t n = read(fd, tmp.data(), tmp.size());
        if (n > 0) {
            buf.insert(buf.end(), tmp.data(), tmp.data() + n);
            continue;
        }
        if (n == 0) {
            peer_closed = true;
            return true;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return true;
        }
        return false;
    }
}

bool pop_one_frame(std::vector<uint8_t>& buf, FrameHeader& hdr, std::vector<uint8_t>& payload) {
    if (buf.size() < sizeof(FrameHeader)) {
        return false;
    }

    std::memcpy(&hdr, buf.data(), sizeof(FrameHeader));
    if (hdr.length > kMaxFramePayload) {
        hdr.type = 0;
        hdr.length = 0;
        return true;
    }

    size_t need = sizeof(FrameHeader) + static_cast<size_t>(hdr.length);
    if (buf.size() < need) {
        return false;
    }

    payload.clear();
    if (hdr.length > 0) {
        payload.insert(payload.end(),
                       buf.begin() + static_cast<std::ptrdiff_t>(sizeof(FrameHeader)),
                       buf.begin() + static_cast<std::ptrdiff_t>(need));
    }

    buf.erase(buf.begin(), buf.begin() + static_cast<std::ptrdiff_t>(need));
    return true;
}

std::string trim(std::string s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) {
        s.pop_back();
    }
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) {
        ++i;
    }
    if (i > 0) {
        s.erase(0, i);
    }
    return s;
}

bool parse_int_token(const std::string& s, int& out) {
    if (s.empty()) {
        return false;
    }
    char* end = nullptr;
    long v = std::strtol(s.c_str(), &end, 10);
    if (end == s.c_str() || *end != '\0') {
        return false;
    }
    if (v < 0 || v > 1'000'000) {
        return false;
    }
    out = static_cast<int>(v);
    return true;
}

std::string sanitize_name(std::string name) {
    if (name.empty()) {
        name = "default";
    }
    for (char& c : name) {
        bool ok = (c >= 'a' && c <= 'z') ||
                  (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') ||
                  c == '_' || c == '-' || c == '.';
        if (!ok) {
            c = '_';
        }
    }
    return name;
}

std::string socket_path_for_server(const std::string& server_name) {
    uid_t uid = getuid();
    std::string sanitized = sanitize_name(server_name);
    return "/tmp/mini-tmux-" + std::to_string(static_cast<unsigned long long>(uid)) + "-" + sanitized + ".sock";
}

std::string detect_self_exe() {
    std::array<char, 4096> buf{};
    ssize_t n = readlink("/proc/self/exe", buf.data(), buf.size() - 1);
    if (n <= 0) {
        return "./mini-tmux";
    }
    buf[static_cast<size_t>(n)] = '\0';
    return std::string(buf.data());
}

std::pair<int, int> terminal_size_or_default() {
    winsize ws{};
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
        return {ws.ws_row, ws.ws_col};
    }
    return {24, 80};
}

class RawModeGuard {
public:
    bool enable() {
        if (!isatty(STDIN_FILENO)) {
            return true;
        }
        if (tcgetattr(STDIN_FILENO, &old_) < 0) {
            return false;
        }
        termios raw = old_;
        cfmakeraw(&raw);
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) < 0) {
            return false;
        }
        active_ = true;
        return true;
    }

    void restore() {
        if (!active_) {
            return;
        }
        (void)tcsetattr(STDIN_FILENO, TCSANOW, &old_);
        active_ = false;
    }

    ~RawModeGuard() {
        restore();
    }

private:
    bool active_ = false;
    termios old_{};
};

struct ClientState {
    int fd = -1;
    bool hello = false;
    bool readonly = false;
    int session_id = -1;
    int rows = 24;
    int cols = 80;
    std::vector<uint8_t> inbuf;
};

struct PaneState {
    int pane_id = -1;
    int master_fd = -1;
    pid_t child_pid = -1;
    int rows = 24;
    int cols = 80;

    int log_fd = -1;
    pid_t pipe_pid = -1;
    int pipe_wfd = -1;

    bool esc = false;
    bool csi = false;
    std::deque<std::string> lines;
    std::string current_line;
    std::deque<uint8_t> replay_history;
};

struct SessionState {
    int session_id = -1;
    std::map<int, PaneState> panes;
    std::vector<int> pane_order;
    int focus_pane_id = -1;
    int next_pane_id = 0;
    int last_rows = 24;
    int last_cols = 80;
};

enum class ChildType {
    Pane,
    Pipe,
};

struct ChildRef {
    ChildType type;
    int session_id;
    int pane_id;
};

class Server {
public:
    explicit Server(std::string socket_path)
        : socket_path_(std::move(socket_path)) {}

    bool init() {
        if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
            return false;
        }

        listen_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (listen_fd_ < 0) {
            return false;
        }

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        if (socket_path_.size() >= sizeof(addr.sun_path)) {
            return false;
        }
        std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socket_path_.c_str());

        unlink(socket_path_.c_str());
        if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            return false;
        }
        if (listen(listen_fd_, 64) < 0) {
            return false;
        }
        if (!set_nonblocking(listen_fd_)) {
            return false;
        }

        if (pipe(sig_pipe_) < 0) {
            return false;
        }
        if (!set_nonblocking(sig_pipe_[0]) || !set_nonblocking(sig_pipe_[1])) {
            return false;
        }

        g_sigchld_write_fd = sig_pipe_[1];
        struct sigaction sa{};
        sa.sa_handler = sigchld_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
        if (sigaction(SIGCHLD, &sa, nullptr) < 0) {
            return false;
        }

        return true;
    }

    int run() {
        if (!init()) {
            return 1;
        }

        running_ = true;
        while (running_) {
            std::vector<pollfd> pfds;
            pfds.reserve(2 + clients_.size() + master_to_pane_.size());

            pfds.push_back(pollfd{listen_fd_, POLLIN, 0});
            pfds.push_back(pollfd{sig_pipe_[0], POLLIN, 0});

            for (const auto& kv : clients_) {
                pfds.push_back(pollfd{kv.first, POLLIN, 0});
            }
            for (const auto& kv : master_to_pane_) {
                pfds.push_back(pollfd{kv.first, POLLIN, 0});
            }

            int pr = poll(pfds.data(), static_cast<nfds_t>(pfds.size()), 1000);
            if (pr < 0) {
                if (errno == EINTR) {
                    continue;
                }
                break;
            }

            for (const auto& pfd : pfds) {
                if (pfd.fd == listen_fd_ && (pfd.revents & POLLIN)) {
                    accept_new_clients();
                }
            }

            for (const auto& pfd : pfds) {
                if (pfd.fd == sig_pipe_[0] && (pfd.revents & POLLIN)) {
                    drain_sig_pipe();
                    reap_children();
                }
            }

            std::vector<int> cfd_list;
            cfd_list.reserve(clients_.size());
            for (const auto& kv : clients_) {
                cfd_list.push_back(kv.first);
            }

            for (int cfd : cfd_list) {
                short revents = 0;
                for (const auto& pfd : pfds) {
                    if (pfd.fd == cfd) {
                        revents = pfd.revents;
                        break;
                    }
                }
                if (revents == 0) {
                    continue;
                }
                if (revents & (POLLHUP | POLLERR | POLLNVAL)) {
                    close_client(cfd);
                    continue;
                }
                if (revents & POLLIN) {
                    if (!handle_client_read(cfd)) {
                        close_client(cfd);
                    }
                }
            }

            std::vector<int> mfd_list;
            mfd_list.reserve(master_to_pane_.size());
            for (const auto& kv : master_to_pane_) {
                mfd_list.push_back(kv.first);
            }

            for (int mfd : mfd_list) {
                short revents = 0;
                for (const auto& pfd : pfds) {
                    if (pfd.fd == mfd) {
                        revents = pfd.revents;
                        break;
                    }
                }
                if (revents == 0) {
                    continue;
                }
                auto it = master_to_pane_.find(mfd);
                if (it == master_to_pane_.end()) {
                    continue;
                }
                if (revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) {
                    handle_pane_output(it->second.first, it->second.second);
                }
            }

            reap_children();
        }

        cleanup();
        return 0;
    }

private:
    void cleanup() {
        std::vector<int> cfd;
        for (const auto& kv : clients_) {
            cfd.push_back(kv.first);
        }
        for (int fd : cfd) {
            close_client(fd);
        }

        std::vector<int> sids;
        for (const auto& kv : sessions_) {
            sids.push_back(kv.first);
        }
        for (int sid : sids) {
            remove_session(sid);
        }

        close_fd(listen_fd_);
        close_fd(sig_pipe_[0]);
        close_fd(sig_pipe_[1]);
        g_sigchld_write_fd = -1;

        unlink(socket_path_.c_str());
    }

    void drain_sig_pipe() {
        std::array<uint8_t, 256> buf{};
        while (true) {
            ssize_t n = read(sig_pipe_[0], buf.data(), buf.size());
            if (n > 0) {
                continue;
            }
            if (n < 0 && errno == EINTR) {
                continue;
            }
            break;
        }
    }

    void accept_new_clients() {
        while (true) {
            int cfd = accept(listen_fd_, nullptr, nullptr);
            if (cfd < 0) {
                if (errno == EINTR) {
                    continue;
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                break;
            }
            if (!set_nonblocking(cfd)) {
                close(cfd);
                continue;
            }
            ClientState c;
            c.fd = cfd;
            clients_.emplace(cfd, std::move(c));
        }
    }

    bool handle_client_read(int cfd) {
        auto it = clients_.find(cfd);
        if (it == clients_.end()) {
            return false;
        }

        ClientState& c = it->second;
        bool peer_closed = false;
        if (!read_into_buffer(c.fd, c.inbuf, peer_closed)) {
            return false;
        }
        if (peer_closed) {
            return false;
        }

        while (true) {
            FrameHeader hdr{};
            std::vector<uint8_t> payload;
            if (!pop_one_frame(c.inbuf, hdr, payload)) {
                break;
            }
            if (hdr.type == 0) {
                return false;
            }
            if (!handle_client_frame(c, hdr, payload)) {
                return false;
            }
        }

        return true;
    }

    bool handle_client_frame(ClientState& c, const FrameHeader& hdr, const std::vector<uint8_t>& payload) {
        if (!c.hello) {
            if (hdr.type != kMsgHello) {
                return false;
            }
            return handle_hello(c, hdr, payload);
        }

        if (hdr.type == kMsgInput) {
            if (c.readonly) {
                return true;
            }
            SessionState* s = get_session(c.session_id);
            if (s == nullptr) {
                return true;
            }
            PaneState* p = get_focus_pane(*s);
            if (p == nullptr) {
                return true;
            }
            if (!payload.empty()) {
                if (!write_all_nb(p->master_fd, payload.data(), payload.size())) {
                    remove_pane(c.session_id, p->pane_id);
                }
            }
            return true;
        }

        if (hdr.type == kMsgCmd) {
            std::string cmd(payload.begin(), payload.end());
            handle_command(c, cmd);
            return true;
        }

        if (hdr.type == kMsgDetach) {
            return false;
        }

        if (hdr.type == kMsgResize) {
            if (payload.size() >= 8) {
                uint32_t rows = 0;
                uint32_t cols = 0;
                std::memcpy(&rows, payload.data(), sizeof(uint32_t));
                std::memcpy(&cols, payload.data() + 4, sizeof(uint32_t));
                if (rows > 0 && cols > 0) {
                    c.rows = static_cast<int>(rows);
                    c.cols = static_cast<int>(cols);
                    recompute_layout(c.session_id);
                }
            }
            return true;
        }

        return true;
    }

    bool handle_hello(ClientState& c, const FrameHeader& hdr, const std::vector<uint8_t>& payload) {
        int rows = 24;
        int cols = 80;
        if (payload.size() >= 8) {
            uint32_t r = 0;
            uint32_t cc = 0;
            std::memcpy(&r, payload.data(), sizeof(uint32_t));
            std::memcpy(&cc, payload.data() + 4, sizeof(uint32_t));
            if (r > 0) {
                rows = static_cast<int>(r);
            }
            if (cc > 0) {
                cols = static_cast<int>(cc);
            }
        }

        c.rows = rows;
        c.cols = cols;
        c.readonly = (hdr.flags & kFlagReadonly) != 0;

        int sid = -1;
        bool want_new = (hdr.flags & kFlagCreateNewSession) != 0;
        if (sessions_.empty() || want_new) {
            sid = create_session(rows, cols);
            if (sid < 0) {
                send_frame_str(c.fd, kMsgError, 0, 0, "failed to create session");
                return false;
            }
        } else {
            sid = default_session_id();
            if (sid < 0) {
                return false;
            }
        }

        c.session_id = sid;
        c.hello = true;

        std::vector<uint8_t> empty;
        send_frame(c.fd, kMsgHelloAck, 0, static_cast<uint32_t>(sid), empty);
        recompute_layout(sid);
        replay_focus_history_to_client(c);
        return true;
    }

    SessionState* get_session(int sid) {
        auto it = sessions_.find(sid);
        if (it == sessions_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    PaneState* get_pane(int sid, int pane_id) {
        SessionState* s = get_session(sid);
        if (s == nullptr) {
            return nullptr;
        }
        auto it = s->panes.find(pane_id);
        if (it == s->panes.end()) {
            return nullptr;
        }
        return &it->second;
    }

    PaneState* get_focus_pane(SessionState& s) {
        auto it = s.panes.find(s.focus_pane_id);
        if (it == s.panes.end()) {
            return nullptr;
        }
        return &it->second;
    }

    int create_session(int rows, int cols) {
        SessionState s;
        s.session_id = next_session_id_++;
        s.last_rows = rows;
        s.last_cols = cols;

        sessions_[s.session_id] = s;
        if (create_pane(s.session_id) < 0) {
            sessions_.erase(s.session_id);
            return -1;
        }
        recompute_layout(s.session_id);
        return s.session_id;
    }

    int create_pane_process(int sid, int pane_id, int rows, int cols, PaneState& out) {
        int master_fd = -1;
        int slave_fd = -1;

        winsize ws{};
        ws.ws_row = static_cast<unsigned short>(std::max(1, rows));
        ws.ws_col = static_cast<unsigned short>(std::max(1, cols));

        if (openpty(&master_fd, &slave_fd, nullptr, nullptr, &ws) < 0) {
            return -1;
        }

        pid_t pid = fork();
        if (pid < 0) {
            close(master_fd);
            close(slave_fd);
            return -1;
        }

        if (pid == 0) {
            if (setsid() < 0) {
                _exit(127);
            }

            (void)ioctl(slave_fd, TIOCSCTTY, 0);
            (void)tcsetpgrp(slave_fd, getpid());

            if (dup2(slave_fd, STDIN_FILENO) < 0 ||
                dup2(slave_fd, STDOUT_FILENO) < 0 ||
                dup2(slave_fd, STDERR_FILENO) < 0) {
                _exit(127);
            }

            if (slave_fd > STDERR_FILENO) {
                close(slave_fd);
            }
            close(master_fd);

            long max_fd = sysconf(_SC_OPEN_MAX);
            if (max_fd < 0) {
                max_fd = 1024;
            }
            for (int fd = 3; fd < max_fd; ++fd) {
                close(fd);
            }

            const char* shell = getenv("SHELL");
            if (shell == nullptr || *shell == '\0') {
                shell = "/bin/sh";
            }
            execl(shell, shell, "-i", static_cast<char*>(nullptr));
            execl("/bin/sh", "/bin/sh", "-i", static_cast<char*>(nullptr));
            _exit(127);
        }

        close(slave_fd);
        if (!set_nonblocking(master_fd)) {
            close(master_fd);
            (void)kill(pid, SIGKILL);
            (void)waitpid(pid, nullptr, 0);
            return -1;
        }

        out = PaneState{};
        out.pane_id = pane_id;
        out.master_fd = master_fd;
        out.child_pid = pid;
        out.rows = rows;
        out.cols = cols;

        master_to_pane_[master_fd] = {sid, pane_id};
        children_[pid] = ChildRef{ChildType::Pane, sid, pane_id};
        return 0;
    }

    int create_pane(int sid) {
        SessionState* s = get_session(sid);
        if (s == nullptr) {
            return -1;
        }

        int pane_id = s->next_pane_id++;
        PaneState pane;
        if (create_pane_process(sid, pane_id, s->last_rows, s->last_cols, pane) < 0) {
            return -1;
        }

        s->pane_order.push_back(pane_id);
        s->panes.emplace(pane_id, std::move(pane));
        s->focus_pane_id = pane_id;
        recompute_layout(sid);
        return pane_id;
    }

    void push_capture_line(PaneState& p, const std::string& line) {
        p.lines.push_back(line);
        while (p.lines.size() > kCaptureMaxLines) {
            p.lines.pop_front();
        }
    }

    void append_capture(PaneState& p, const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            unsigned char ch = data[i];

            if (p.esc) {
                if (!p.csi) {
                    if (ch == '[') {
                        p.csi = true;
                    } else {
                        p.esc = false;
                    }
                    continue;
                }
                if (ch >= 0x40 && ch <= 0x7e) {
                    p.esc = false;
                    p.csi = false;
                }
                continue;
            }

            if (ch == 0x1b) {
                p.esc = true;
                p.csi = false;
                continue;
            }

            if (ch == '\r') {
                continue;
            }

            if (ch == '\n') {
                push_capture_line(p, p.current_line);
                p.current_line.clear();
                continue;
            }

            if (ch == '\t' || (ch >= 32 && ch < 127)) {
                if (p.current_line.size() < 4096) {
                    p.current_line.push_back(static_cast<char>(ch));
                }
            }
        }
    }

    void append_replay_history(PaneState& p, const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            p.replay_history.push_back(data[i]);
        }
        while (p.replay_history.size() > kReplayHistoryMaxBytes) {
            p.replay_history.pop_front();
        }
    }

    void replay_focus_history_to_client(const ClientState& c) {
        SessionState* s = get_session(c.session_id);
        if (s == nullptr) {
            return;
        }

        PaneState* p = get_focus_pane(*s);
        if (p == nullptr && !s->pane_order.empty()) {
            p = get_pane(s->session_id, s->pane_order.front());
        }
        if (p == nullptr || p->replay_history.empty()) {
            return;
        }

        std::vector<uint8_t> payload;
        payload.reserve(p->replay_history.size());
        for (uint8_t b : p->replay_history) {
            payload.push_back(b);
        }
        (void)send_frame(c.fd, kMsgOutput, 0, 0, payload);
    }

    void cleanup_pipe(PaneState& p, bool wait_child) {
        if (p.pipe_wfd >= 0) {
            close_fd(p.pipe_wfd);
        }

        if (p.pipe_pid > 0) {
            pid_t pid = p.pipe_pid;
            children_.erase(pid);
            p.pipe_pid = -1;

            if (wait_child) {
                for (int i = 0; i < 50; ++i) {
                    int st = 0;
                    pid_t r = waitpid(pid, &st, WNOHANG);
                    if (r == pid) {
                        return;
                    }
                    if (r < 0 && errno != EINTR) {
                        return;
                    }
                    usleep(10 * 1000);
                }
                (void)kill(pid, SIGTERM);
                (void)waitpid(pid, nullptr, 0);
            }
        }
    }

    void cleanup_pane_resources(PaneState& p, bool wait_pipe) {
        cleanup_pipe(p, wait_pipe);
        if (p.log_fd >= 0) {
            close_fd(p.log_fd);
        }
        if (p.master_fd >= 0) {
            close_fd(p.master_fd);
        }
    }

    void remove_pane(int sid, int pane_id) {
        SessionState* s = get_session(sid);
        if (s == nullptr) {
            return;
        }
        auto pit = s->panes.find(pane_id);
        if (pit == s->panes.end()) {
            return;
        }

        PaneState pane = std::move(pit->second);
        s->panes.erase(pit);
        s->pane_order.erase(std::remove(s->pane_order.begin(), s->pane_order.end(), pane_id), s->pane_order.end());

        if (pane.master_fd >= 0) {
            master_to_pane_.erase(pane.master_fd);
        }
        if (pane.child_pid > 0) {
            children_.erase(pane.child_pid);
        }

        cleanup_pane_resources(pane, true);

        if (!s->pane_order.empty()) {
            if (s->focus_pane_id == pane_id || s->panes.find(s->focus_pane_id) == s->panes.end()) {
                s->focus_pane_id = s->pane_order.front();
            }
            recompute_layout(sid);
            return;
        }

        remove_session(sid);
    }

    void remove_session(int sid) {
        auto sit = sessions_.find(sid);
        if (sit == sessions_.end()) {
            return;
        }

        SessionState s = std::move(sit->second);
        sessions_.erase(sit);

        for (auto& kv : s.panes) {
            PaneState& p = kv.second;
            if (p.master_fd >= 0) {
                master_to_pane_.erase(p.master_fd);
            }
            if (p.child_pid > 0) {
                children_.erase(p.child_pid);
            }
            cleanup_pane_resources(p, true);
        }

        std::vector<int> affected_clients;
        for (const auto& kv : clients_) {
            if (kv.second.hello && kv.second.session_id == sid) {
                affected_clients.push_back(kv.first);
            }
        }

        if (sessions_.empty()) {
            for (int cfd : affected_clients) {
                close_client(cfd);
            }
            running_ = false;
            return;
        }

        int fallback = default_session_id();
        for (int cfd : affected_clients) {
            auto cit = clients_.find(cfd);
            if (cit != clients_.end()) {
                cit->second.session_id = fallback;
            }
        }
        recompute_layout(fallback);
    }

    void handle_pane_output(int sid, int pane_id) {
        PaneState* p = get_pane(sid, pane_id);
        if (p == nullptr) {
            return;
        }

        std::array<uint8_t, 65536> buf{};
        while (true) {
            ssize_t n = read(p->master_fd, buf.data(), buf.size());
            if (n > 0) {
                append_capture(*p, buf.data(), static_cast<size_t>(n));
                append_replay_history(*p, buf.data(), static_cast<size_t>(n));

                if (p->log_fd >= 0) {
                    write_best_effort_nb(p->log_fd, buf.data(), static_cast<size_t>(n));
                }

                if (p->pipe_wfd >= 0) {
                    ssize_t wr = write(p->pipe_wfd, buf.data(), static_cast<size_t>(n));
                    if (wr < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                        cleanup_pipe(*p, false);
                    }
                }

                std::vector<uint8_t> payload(buf.begin(), buf.begin() + n);
                broadcast_to_session(sid, payload);
                continue;
            }

            if (n == 0) {
                remove_pane(sid, pane_id);
                return;
            }
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            if (errno == EIO) {
                remove_pane(sid, pane_id);
                return;
            }
            remove_pane(sid, pane_id);
            return;
        }
    }

    void broadcast_to_session(int sid, const std::vector<uint8_t>& payload) {
        std::vector<int> dead;
        for (const auto& kv : clients_) {
            const ClientState& c = kv.second;
            if (!c.hello || c.session_id != sid) {
                continue;
            }
            if (!send_frame(c.fd, kMsgOutput, 0, 0, payload)) {
                dead.push_back(c.fd);
            }
        }
        for (int cfd : dead) {
            close_client(cfd);
        }
    }

    void reap_children() {
        while (true) {
            int status = 0;
            pid_t pid = waitpid(-1, &status, WNOHANG);
            if (pid == 0) {
                break;
            }
            if (pid < 0) {
                if (errno == EINTR) {
                    continue;
                }
                break;
            }

            auto it = children_.find(pid);
            if (it == children_.end()) {
                continue;
            }
            ChildRef ref = it->second;
            children_.erase(it);

            if (ref.type == ChildType::Pane) {
                remove_pane(ref.session_id, ref.pane_id);
            } else {
                PaneState* p = get_pane(ref.session_id, ref.pane_id);
                if (p != nullptr && p->pipe_pid == pid) {
                    p->pipe_pid = -1;
                    close_fd(p->pipe_wfd);
                }
            }
        }
    }

    int default_session_id() const {
        if (sessions_.empty()) {
            return -1;
        }
        return sessions_.begin()->first;
    }

    std::vector<int> attached_clients_for_session(int sid) const {
        std::vector<int> out;
        for (const auto& kv : clients_) {
            if (kv.second.hello && kv.second.session_id == sid) {
                out.push_back(kv.first);
            }
        }
        return out;
    }

    void recompute_layout(int sid) {
        SessionState* s = get_session(sid);
        if (s == nullptr) {
            return;
        }
        if (s->pane_order.empty()) {
            return;
        }

        int rows = s->last_rows;
        int cols = s->last_cols;

        auto attached = attached_clients_for_session(sid);
        if (!attached.empty()) {
            rows = clients_[attached[0]].rows;
            cols = clients_[attached[0]].cols;
            for (size_t i = 1; i < attached.size(); ++i) {
                rows = std::min(rows, clients_[attached[i]].rows);
                cols = std::min(cols, clients_[attached[i]].cols);
            }
        }

        rows = std::max(rows, 1);
        cols = std::max(cols, 1);
        s->last_rows = rows;
        s->last_cols = cols;

        int n = static_cast<int>(s->pane_order.size());
        int sep = (rows > n) ? (n - 1) : 0;
        int usable = rows - sep;
        if (usable < n) {
            sep = 0;
            usable = rows;
        }

        std::vector<int> pane_rows(n, 1);
        if (usable >= n) {
            int base = usable / n;
            int extra = usable % n;
            for (int i = 0; i < n; ++i) {
                pane_rows[i] = base + (i < extra ? 1 : 0);
            }
        }

        for (int i = 0; i < n; ++i) {
            int pid = s->pane_order[i];
            auto it = s->panes.find(pid);
            if (it == s->panes.end()) {
                continue;
            }
            PaneState& p = it->second;
            p.rows = std::max(1, pane_rows[i]);
            p.cols = cols;

            winsize ws{};
            ws.ws_row = static_cast<unsigned short>(p.rows);
            ws.ws_col = static_cast<unsigned short>(p.cols);
            (void)ioctl(p.master_fd, TIOCSWINSZ, &ws);
            if (p.child_pid > 0) {
                (void)kill(-p.child_pid, SIGWINCH);
            }
        }
    }

    bool parse_prefixed_one_int(const std::string& cmd, const std::string& key, int& val) {
        if (cmd.rfind(key, 0) != 0) {
            return false;
        }
        std::string tail = trim(cmd.substr(key.size()));
        return parse_int_token(tail, val);
    }

    bool parse_prefixed_int_rest(const std::string& cmd, const std::string& key, int& val, std::string& rest) {
        if (cmd.rfind(key, 0) != 0) {
            return false;
        }
        std::string tail = trim(cmd.substr(key.size()));
        if (tail.empty()) {
            return false;
        }

        size_t pos = 0;
        while (pos < tail.size() && tail[pos] != ' ' && tail[pos] != '\t') {
            ++pos;
        }
        std::string n = tail.substr(0, pos);
        if (!parse_int_token(n, val)) {
            return false;
        }
        rest = trim(tail.substr(pos));
        return true;
    }

    void start_log(PaneState& p, const std::string& path) {
        if (p.log_fd >= 0) {
            close_fd(p.log_fd);
        }
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (fd >= 0) {
            p.log_fd = fd;
        }
    }

    void start_pipeout(PaneState& p, int sid, const std::string& cmd) {
        cleanup_pipe(p, true);

        int fds[2] = {-1, -1};
        if (pipe(fds) < 0) {
            return;
        }

        pid_t pid = fork();
        if (pid < 0) {
            close_fd(fds[0]);
            close_fd(fds[1]);
            return;
        }

        if (pid == 0) {
            if (dup2(fds[0], STDIN_FILENO) < 0) {
                _exit(127);
            }
            close_fd(fds[0]);
            close_fd(fds[1]);

            long max_fd = sysconf(_SC_OPEN_MAX);
            if (max_fd < 0) {
                max_fd = 1024;
            }
            for (int fd = 3; fd < max_fd; ++fd) {
                close(fd);
            }

            execl("/bin/sh", "sh", "-c", cmd.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }

        close_fd(fds[0]);
        set_nonblocking(fds[1]);

        p.pipe_pid = pid;
        p.pipe_wfd = fds[1];
        children_[pid] = ChildRef{ChildType::Pipe, sid, p.pane_id};
    }

    void capture_pane(PaneState& p, const std::string& path) {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) {
            return;
        }

        for (const std::string& line : p.lines) {
            if (!line.empty()) {
                write_best_effort_nb(fd, reinterpret_cast<const uint8_t*>(line.data()), line.size());
            }
            static const uint8_t nl = '\n';
            write_best_effort_nb(fd, &nl, 1);
        }
        if (!p.current_line.empty()) {
            write_best_effort_nb(fd,
                                 reinterpret_cast<const uint8_t*>(p.current_line.data()),
                                 p.current_line.size());
            static const uint8_t nl = '\n';
            write_best_effort_nb(fd, &nl, 1);
        }
        close_fd(fd);
    }

    void focus_next(SessionState& s, bool next) {
        if (s.pane_order.empty()) {
            return;
        }
        auto it = std::find(s.pane_order.begin(), s.pane_order.end(), s.focus_pane_id);
        size_t idx = 0;
        if (it != s.pane_order.end()) {
            idx = static_cast<size_t>(std::distance(s.pane_order.begin(), it));
        }
        if (next) {
            idx = (idx + 1) % s.pane_order.size();
        } else {
            idx = (idx + s.pane_order.size() - 1) % s.pane_order.size();
        }
        s.focus_pane_id = s.pane_order[idx];
    }

    void handle_command(ClientState& c, const std::string& raw) {
        std::string cmd = trim(raw);
        if (cmd.empty()) {
            return;
        }

        SessionState* s = get_session(c.session_id);
        if (s == nullptr) {
            return;
        }

        if (cmd == ":detach") {
            close_client(c.fd);
            return;
        }

        if (cmd == ":new") {
            (void)create_pane(c.session_id);
            return;
        }

        if (cmd == ":focus-next") {
            focus_next(*s, true);
            return;
        }
        if (cmd == ":focus-prev") {
            focus_next(*s, false);
            return;
        }

        int pane_id = -1;
        if (parse_prefixed_one_int(cmd, ":focus", pane_id)) {
            if (s->panes.count(pane_id)) {
                s->focus_pane_id = pane_id;
            }
            return;
        }

        if (parse_prefixed_one_int(cmd, ":kill", pane_id)) {
            PaneState* p = get_pane(c.session_id, pane_id);
            if (p != nullptr && p->child_pid > 0) {
                (void)kill(-p->child_pid, SIGHUP);
            }
            return;
        }

        std::string rest;
        if (parse_prefixed_int_rest(cmd, ":log", pane_id, rest)) {
            PaneState* p = get_pane(c.session_id, pane_id);
            if (p != nullptr && !rest.empty()) {
                start_log(*p, rest);
            }
            return;
        }

        if (parse_prefixed_one_int(cmd, ":log-stop", pane_id)) {
            PaneState* p = get_pane(c.session_id, pane_id);
            if (p != nullptr && p->log_fd >= 0) {
                close_fd(p->log_fd);
            }
            return;
        }

        if (parse_prefixed_int_rest(cmd, ":pipeout", pane_id, rest)) {
            PaneState* p = get_pane(c.session_id, pane_id);
            if (p != nullptr && !rest.empty()) {
                start_pipeout(*p, c.session_id, rest);
            }
            return;
        }

        if (parse_prefixed_one_int(cmd, ":pipeout-stop", pane_id)) {
            PaneState* p = get_pane(c.session_id, pane_id);
            if (p != nullptr) {
                cleanup_pipe(*p, true);
            }
            return;
        }

        if (parse_prefixed_int_rest(cmd, ":capture", pane_id, rest)) {
            PaneState* p = get_pane(c.session_id, pane_id);
            if (p != nullptr && !rest.empty()) {
                capture_pane(*p, rest);
            }
            return;
        }
    }

    void close_client(int cfd) {
        auto it = clients_.find(cfd);
        if (it == clients_.end()) {
            return;
        }
        int sid = it->second.session_id;
        close_fd(it->second.fd);
        clients_.erase(it);

        if (sid >= 0) {
            recompute_layout(sid);
        }
    }

    std::string socket_path_;
    int listen_fd_ = -1;
    int sig_pipe_[2] = {-1, -1};
    bool running_ = false;

    std::map<int, ClientState> clients_;
    std::map<int, SessionState> sessions_;
    int next_session_id_ = 0;

    std::map<int, std::pair<int, int>> master_to_pane_;
    std::map<pid_t, ChildRef> children_;
};

bool connect_to_socket_once(const std::string& sock_path, int& out_fd) {
    out_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (out_fd < 0) {
        return false;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (sock_path.size() >= sizeof(addr.sun_path)) {
        close_fd(out_fd);
        errno = ENAMETOOLONG;
        return false;
    }
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock_path.c_str());

    if (connect(out_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close_fd(out_fd);
        return false;
    }

    if (!set_nonblocking(out_fd)) {
        close_fd(out_fd);
        return false;
    }

    return true;
}

bool connect_with_retry(const std::string& sock_path, int& out_fd, int attempts, int sleep_ms) {
    for (int i = 0; i < attempts; ++i) {
        if (connect_to_socket_once(sock_path, out_fd)) {
            return true;
        }
        int err = errno;
        if (err != ENOENT && err != ECONNREFUSED && err != EAGAIN) {
            return false;
        }
        usleep(static_cast<useconds_t>(sleep_ms * 1000));
    }
    return false;
}

bool spawn_server_process(const std::string& self_exe) {
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }
    if (pid == 0) {
        if (setsid() < 0) {
            _exit(127);
        }
        execl(self_exe.c_str(), self_exe.c_str(), "--server", static_cast<char*>(nullptr));
        _exit(127);
    }
    return true;
}

struct InputState {
    bool cmd_mode = false;
    bool prefix_pending = false;
    bool line_start = true;
    bool swallow_lf = false;
    std::string cmd;
};

bool send_resize_frame(int sock_fd) {
    auto [rows, cols] = terminal_size_or_default();
    uint32_t r = static_cast<uint32_t>(rows);
    uint32_t c = static_cast<uint32_t>(cols);
    std::vector<uint8_t> payload(8);
    std::memcpy(payload.data(), &r, sizeof(uint32_t));
    std::memcpy(payload.data() + 4, &c, sizeof(uint32_t));
    return send_frame(sock_fd, kMsgResize, 0, 0, payload);
}

bool process_input_bytes(int sock_fd, const uint8_t* data, size_t len, InputState& st, bool readonly) {
    for (size_t i = 0; i < len; ++i) {
        uint8_t b = data[i];

        if (st.swallow_lf && b == '\n') {
            st.swallow_lf = false;
            st.line_start = true;
            continue;
        }
        st.swallow_lf = false;

        if (st.cmd_mode) {
            if (b == '\r' || b == '\n') {
                if (!readonly) {
                    if (!send_frame_str(sock_fd, kMsgCmd, 0, 0, st.cmd)) {
                        return false;
                    }
                }
                st.cmd.clear();
                st.cmd_mode = false;
                st.line_start = true;
                if (b == '\r') {
                    st.swallow_lf = true;
                }
                continue;
            }
            if (b == 0x7f || b == 0x08) {
                if (!st.cmd.empty() && st.cmd.size() > 1) {
                    st.cmd.pop_back();
                }
                continue;
            }
            if (b >= 32 && b <= 126) {
                st.cmd.push_back(static_cast<char>(b));
            }
            continue;
        }

        if (st.prefix_pending) {
            st.prefix_pending = false;
            if (b == 'd' || b == 'D') {
                std::vector<uint8_t> no_payload;
                (void)send_frame(sock_fd, kMsgDetach, 0, 0, no_payload);
                return false;
            }
            if (b == ':') {
                st.cmd_mode = true;
                st.cmd = ":";
                continue;
            }
            if (b == 'n' || b == 'N') {
                if (!readonly) {
                    if (!send_frame_str(sock_fd, kMsgCmd, 0, 0, ":focus-next")) {
                        return false;
                    }
                }
                continue;
            }
            if (b == 'p' || b == 'P') {
                if (!readonly) {
                    if (!send_frame_str(sock_fd, kMsgCmd, 0, 0, ":focus-prev")) {
                        return false;
                    }
                }
                continue;
            }
            if (!readonly) {
                std::vector<uint8_t> payload;
                payload.push_back(0x02);
                payload.push_back(b);
                if (!send_frame(sock_fd, kMsgInput, 0, 0, payload)) {
                    return false;
                }
            }
            st.line_start = (b == '\r' || b == '\n');
            if (b == '\r') {
                st.swallow_lf = true;
            }
            continue;
        }

        if (b == 0x02) {
            st.prefix_pending = true;
            continue;
        }

        if (st.line_start && b == ':') {
            st.cmd_mode = true;
            st.cmd = ":";
            continue;
        }

        if (!readonly) {
            std::vector<uint8_t> payload(1, b);
            if (!send_frame(sock_fd, kMsgInput, 0, 0, payload)) {
                return false;
            }
        }

        st.line_start = (b == '\r' || b == '\n');
        if (b == '\r') {
            st.swallow_lf = true;
        }
    }

    return true;
}

int run_client(const std::string& socket_path, bool attach_mode, bool readonly_mode) {
    int sock_fd = -1;

    bool connected = connect_to_socket_once(socket_path, sock_fd);
    bool started_server = false;

    if (!connected) {
        if (attach_mode) {
            std::fprintf(stderr, "mini-tmux: no server to attach\n");
            return 1;
        }
        std::string self_exe = detect_self_exe();
        if (!spawn_server_process(self_exe)) {
            std::fprintf(stderr, "mini-tmux: failed to spawn server\n");
            return 1;
        }
        started_server = true;
        if (!connect_with_retry(socket_path, sock_fd, 120, 50)) {
            std::fprintf(stderr, "mini-tmux: failed to connect to server\n");
            return 1;
        }
    }

    uint32_t flags = 0;
    if (readonly_mode) {
        flags |= kFlagReadonly;
    }
    if (!attach_mode && !started_server) {
        flags |= kFlagCreateNewSession;
    }

    auto [rows, cols] = terminal_size_or_default();
    uint32_t r = static_cast<uint32_t>(rows);
    uint32_t c = static_cast<uint32_t>(cols);
    std::vector<uint8_t> hello_payload(8);
    std::memcpy(hello_payload.data(), &r, sizeof(uint32_t));
    std::memcpy(hello_payload.data() + 4, &c, sizeof(uint32_t));

    if (!send_frame(sock_fd, kMsgHello, flags, 0, hello_payload)) {
        std::fprintf(stderr, "mini-tmux: failed to send hello\n");
        close_fd(sock_fd);
        return 1;
    }

    RawModeGuard raw;
    if (!raw.enable()) {
        std::fprintf(stderr, "mini-tmux: failed to enable raw mode\n");
        close_fd(sock_fd);
        return 1;
    }

    int winch_pipe[2] = {-1, -1};
    if (pipe(winch_pipe) == 0) {
        set_nonblocking(winch_pipe[0]);
        set_nonblocking(winch_pipe[1]);
        g_sigwinch_write_fd = winch_pipe[1];

        struct sigaction sa{};
        sa.sa_handler = sigwinch_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        sigaction(SIGWINCH, &sa, nullptr);
    }

    signal(SIGPIPE, SIG_IGN);

    std::vector<uint8_t> sock_buf;
    InputState input_state;

    bool running = true;
    while (running) {
        pollfd pfds[3]{};
        nfds_t nfd = 0;

        pfds[nfd++] = pollfd{STDIN_FILENO, POLLIN, 0};
        pfds[nfd++] = pollfd{sock_fd, POLLIN, 0};
        if (winch_pipe[0] >= 0) {
            pfds[nfd++] = pollfd{winch_pipe[0], POLLIN, 0};
        }

        int pr = poll(pfds, nfd, -1);
        if (pr < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        if (pfds[0].revents & POLLIN) {
            std::array<uint8_t, 4096> in{};
            ssize_t n = read(STDIN_FILENO, in.data(), in.size());
            if (n <= 0) {
                running = false;
            } else {
                if (!process_input_bytes(sock_fd, in.data(), static_cast<size_t>(n), input_state, readonly_mode)) {
                    running = false;
                }
            }
        }

        if (pfds[1].revents & (POLLHUP | POLLERR | POLLNVAL)) {
            running = false;
        } else if (pfds[1].revents & POLLIN) {
            bool peer_closed = false;
            if (!read_into_buffer(sock_fd, sock_buf, peer_closed) || peer_closed) {
                running = false;
            } else {
                while (true) {
                    FrameHeader hdr{};
                    std::vector<uint8_t> payload;
                    if (!pop_one_frame(sock_buf, hdr, payload)) {
                        break;
                    }
                    if (hdr.type == 0) {
                        running = false;
                        break;
                    }
                    if (hdr.type == kMsgOutput) {
                        if (!payload.empty()) {
                            if (!write_all_nb(STDOUT_FILENO, payload.data(), payload.size())) {
                                running = false;
                                break;
                            }
                        }
                    } else if (hdr.type == kMsgError) {
                        std::string msg(payload.begin(), payload.end());
                        std::string out = "mini-tmux server error: " + msg + "\n";
                        (void)write_all_nb(STDERR_FILENO,
                                           reinterpret_cast<const uint8_t*>(out.data()),
                                           out.size());
                    }
                }
            }
        }

        if (nfd >= 3 && (pfds[2].revents & POLLIN)) {
            std::array<uint8_t, 256> tmp{};
            while (true) {
                ssize_t n = read(winch_pipe[0], tmp.data(), tmp.size());
                if (n > 0) {
                    continue;
                }
                if (n < 0 && errno == EINTR) {
                    continue;
                }
                break;
            }
            if (!send_resize_frame(sock_fd)) {
                running = false;
            }
        }
    }

    raw.restore();

    if (winch_pipe[0] >= 0) {
        close_fd(winch_pipe[0]);
    }
    if (winch_pipe[1] >= 0) {
        close_fd(winch_pipe[1]);
    }
    g_sigwinch_write_fd = -1;

    close_fd(sock_fd);
    return 0;
}

void usage() {
    std::fprintf(stderr,
                 "Usage:\n"
                 "  mini-tmux\n"
                 "  mini-tmux attach\n"
                 "  mini-tmux attach -r\n");
}

}  // namespace

int main(int argc, char** argv) {
    std::string server_name = "default";
    if (const char* env = std::getenv("MINI_TMUX_SERVER")) {
        server_name = env;
    }

    std::string sock_path = socket_path_for_server(server_name);

    if (argc >= 2 && std::string(argv[1]) == "--server") {
        Server server(sock_path);
        return server.run();
    }

    bool attach_mode = false;
    bool readonly_mode = false;

    if (argc == 1) {
        attach_mode = false;
    } else if (argc >= 2 && std::string(argv[1]) == "attach") {
        attach_mode = true;
        if (argc == 3 && std::string(argv[2]) == "-r") {
            readonly_mode = true;
        } else if (argc != 2) {
            usage();
            return 1;
        }
    } else {
        usage();
        return 1;
    }

    return run_client(sock_path, attach_mode, readonly_mode);
}
