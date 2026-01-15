#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <limits>
#include <locale>
#include <codecvt>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <string_view>
using namespace std;
#define OK 1
#define ERROR 0
#define MAXSIZE 1000

typedef int Status;
typedef struct
{
    char *base;
    char *top;
    int stacksize;
} SqStack;

static bool Outer_html(const string &path);
static bool Text(const string &path);
static inline bool is_alpha_num(char c);
static string normalize_text_preserve_newlines(const string &s);
static string extract_text_in_range(string_view s, size_t contentBeg, size_t contentEnd);

Status initStack(SqStack &S)
{
    S.stacksize = MAXSIZE;
    S.base = new char[MAXSIZE];
    if (!S.base)
        return ERROR;
    S.top = S.base;
    return OK;
}
Status Push(SqStack &S, char c)
{
    if (S.top - S.base >= S.stacksize)
    {
        // 栈满追加存储空间
        int newsize = S.stacksize + MAXSIZE;
        char *newbase = new char[newsize];
        if (!newbase)
            return ERROR;
        memcpy(newbase, S.base, S.stacksize);
        delete[] S.base;
        S.base = newbase;
        S.top = S.base + S.stacksize;
        S.stacksize = newsize;
    }
    *S.top++ = c;
    return OK;
}
Status Pop(SqStack &S, char &c)
{
    if (S.top == S.base)
        return ERROR; // 栈空
    c = *--S.top;
    return OK;
}

// ========== 错误信息结构体与全局变量 ==========
struct Error
{
    vector<string> errors; // 合法性检查错误
};

static Error g_doc;            // 存储错误信息
static char *g_html = nullptr; // HTML内容
static size_t g_html_len = 0;  // HTML长度

// =========================
// 路径解析与匹配
static inline void trim_inplace(string &s)
{
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r'))
        ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r'))
        --b;
    if (a != 0 || b != s.size())
        s = s.substr(a, b - a);
}

static inline vector<string> parse_path_tokens(const string &pathRaw, bool &isAbsolute, bool &isRootAll)
{
    string p = pathRaw;
    trim_inplace(p);
    isAbsolute = false;
    isRootAll = false;
    vector<string> tokens;
    if (p.empty() || p == "/")
    {
        isRootAll = true; // 表示选择全部内容
        return tokens;
    }
    if (!p.empty() && p.front() == '/')
    {
        isAbsolute = true;
    }
    // 按'/'分割，忽略空片段
    size_t i = 0, n = p.size();
    while (i < n)
    {
        // 跳过分隔符
        while (i < n && p[i] == '/')
            ++i;
        if (i >= n)
            break;
        size_t j = i;
        while (j < n && p[j] != '/')
            ++j;
        string seg = p.substr(i, j - i);
        trim_inplace(seg);
        if (!seg.empty())
        {
            // 仅保留由Tag名构成（字母数字下划线中划线），并转大写
            string t;
            for (char ch : seg)
            {
                if (is_alpha_num(ch))
                {
                    if (ch >= 'a' && ch <= 'z')
                        t.push_back(char(ch - 'a' + 'A'));
                    else
                        t.push_back(ch);
                }
            }
            if (!t.empty())
                tokens.push_back(t);
        }
        i = j + 1;
    }
    return tokens;
}

// 大小写不敏感查找子串
static inline size_t find_ci_sv(string_view hay, string_view needle, size_t from)
{
    if (needle.empty() || hay.size() < needle.size() || from >= hay.size())
        return string_view::npos;
    size_t last = hay.size() - needle.size();
    auto lower = [](char ch)
    { if (ch>='A' && ch<='Z') return char(ch - 'A' + 'a'); return ch; };
    for (size_t i = from; i <= last; ++i)
    {
        size_t j = 0;
        for (; j < needle.size(); ++j)
            if (lower(hay[i + j]) != lower(needle[j]))
                break;
        if (j == needle.size())
            return i;
    }
    return string_view::npos;
}

// 从 pos ('<' 位置) 开始，找到匹配的 '>'，考虑属性引号
static inline size_t find_tag_gt(string_view s, size_t pos)
{
    bool inS = false, inD = false;
    for (size_t i = pos + 1; i < s.size(); ++i)
    {
        char ch = s[i];
        if (ch == '"' && !inS)
            inD = !inD;
        else if (ch == '\'' && !inD)
            inS = !inS;
        else if (ch == '>' && !inS && !inD)
            return i;
    }
    return string_view::npos;
}

// 跳过注释，返回跳过后的位置
static inline size_t skip_comment(string_view s, size_t i)
{
    // 进入时 s[i] == '<' 且后续是 "!--"
    size_t end = s.find("-->", i + 4);
    return (end == string::npos) ? s.size() : end + 3;
}

// 跳过 <SCRIPT>/<STYLE> 整块，返回跳过后的位置（包含关闭标签）
static inline size_t skip_script_style_block(string_view s, size_t ltOpen, size_t gtOpen, const string &nameU)
{
    // 构造 </NAME>
    string closeTag = string("</") + nameU + ">";
    size_t closePos = find_ci_sv(s, closeTag, gtOpen + 1);
    if (closePos == string::npos)
        return gtOpen + 1; // 宽松：未找到则仅跳过当前起始标签
    size_t endGt = find_tag_gt(s, closePos);
    if (endGt == string::npos)
        return gtOpen + 1;
    return endGt + 1;
}

// 路径匹配：
// - isAbsolute=true: 绝对路径，需要完整匹配
// - isAbsolute=false: 相对/部分路径，允许匹配为当前路径的后缀
static inline bool path_matches(const vector<string> &openNames, const vector<string> &tokens, bool isAbsolute)
{
    if (tokens.empty())
        return false;
    if (isAbsolute)
    {
        if (openNames.size() != tokens.size())
            return false;
        for (size_t i = 0; i < tokens.size(); ++i)
            if (openNames[i] != tokens[i])
                return false;
        return true;
    }
    else
    {
        if (openNames.size() < tokens.size())
            return false;
        size_t off = openNames.size() - tokens.size();
        for (size_t i = 0; i < tokens.size(); ++i)
            if (openNames[off + i] != tokens[i])
                return false;
        return true;
    }
}

// =========================

// ========== 工具函数 ==========
static inline bool is_alpha_num(char c)
{
    return isalnum((unsigned char)c) || c == '-' || c == '_';
}

static inline string to_upper(const string &s)
{
    string t = s;
    for (char &c : t)
    {
        if (c >= 'a' && c <= 'z')
            c = char(c - 'a' + 'A');
    }
    return t;
}

static inline bool is_self_closing_by_name(const string &nameU)
{
    // 常见自闭合（HTML5 自闭合行为宽松，这里列举常见的）
    static const char *arr[] = {"BR", "HR", "IMG", "META", "LINK", "INPUT", "AREA", "BASE", "COL", "COMMAND", "EMBED", "KEYGEN", "PARAM", "SOURCE", "TRACK", "WBR"};
    for (auto s : arr)
        if (nameU == s)
            return true;
    return false;
}

enum class QueryMode
{
    OUTER,
    TEXT
};

// 在从 (lt, gt, nameU) 打开的标签处，找到该元素匹配关闭后的 endPos，并返回关闭标签的起始 '<' 位置 closeLt。
// 仅用于非自闭合标签。
static inline pair<size_t, size_t> find_element_end_and_close_lt(string_view s, size_t lt, size_t gt, const string &nameU)
{
    size_t n = s.size();
    int depth = 1;
    size_t j = gt + 1;
    while (j < n && depth > 0)
    {
        if (s[j] != '<')
        {
            ++j;
            continue;
        }
        // 注释
        if (j + 3 < n && s.substr(j, 4) == "<!--")
        {
            j = skip_comment(s, j);
            continue;
        }
        size_t gt2 = find_tag_gt(s, j);
        if (gt2 == string::npos)
            break;

        size_t p2 = j + 1;
        bool closing2 = false;
        if (p2 < n && s[p2] == '/')
        {
            closing2 = true;
            ++p2;
        }
        size_t nb2 = p2;
        while (nb2 < n && nb2 <= gt2 && is_alpha_num(s[nb2]))
            ++nb2;
        string nameU2 = to_upper(string(s.substr(p2, nb2 - p2)));

        // script/style 整块跳过（不改变深度）
        if (!closing2 && (nameU2 == "SCRIPT" || nameU2 == "STYLE"))
        {
            j = skip_script_style_block(s, j, gt2, nameU2);
            continue;
        }

        bool self2 = false;
        size_t q2 = gt2;
        while (q2 > j && isspace((unsigned char)s[q2 - 1]))
            --q2;
        if (q2 > j && s[q2 - 1] == '/')
            self2 = true;
        if (is_self_closing_by_name(nameU2))
            self2 = true;

        if (!closing2)
        {
            if (!self2 && nameU2 == nameU)
                ++depth;
        }
        else
        {
            if (nameU2 == nameU)
                --depth;
        }
        j = gt2 + 1;
    }
    // j 指向匹配关闭的 '>' 之后，向回寻找该关闭标签的 '<'
    size_t endPos = j;
    size_t closeLt = endPos;
    while (closeLt > (gt + 1) && s[closeLt - 1] != '<')
        --closeLt;
    return {endPos, closeLt};
}

// 执行查询
static bool perform_query(const vector<string> &tokens, bool isAbsolute, string_view s, QueryMode mode)
{
    vector<string> openNames;
    openNames.reserve(64);
    size_t i = 0, n = s.size();
    bool any = false;
    bool firstOut = true;

    while (i < n)
    {
        if (s[i] != '<')
        {
            ++i;
            continue;
        }
        // 注释
        if (i + 3 < n && s.substr(i, 4) == "<!--")
        {
            i = skip_comment(s, i);
            continue;
        }
        size_t gt = find_tag_gt(s, i);
        if (gt == string::npos)
            break;

        // 解析标签
        size_t p = i + 1;
        bool closing = false;
        if (p < n && s[p] == '/')
        {
            closing = true;
            ++p;
        }
        size_t nb = p;
        while (nb < n && nb <= gt && is_alpha_num(s[nb]))
            ++nb;
        string nameU = to_upper(string(s.substr(p, nb - p)));

        if (nameU.empty())
        {
            i = gt + 1;
            continue;
        }

        // script/style 跳过块（不入栈）
        if (!closing && (nameU == "SCRIPT" || nameU == "STYLE"))
        {
            i = skip_script_style_block(s, i, gt, nameU);
            continue;
        }

        // 自闭合判断
        bool selfclose = false;
        size_t q = gt;
        while (q > i && isspace((unsigned char)s[q - 1]))
            --q;
        if (q > i && s[q - 1] == '/')
            selfclose = true;
        if (is_self_closing_by_name(nameU))
            selfclose = true;

        if (!closing)
        {
            size_t oldSz = openNames.size();
            openNames.push_back(nameU);

            bool matched = path_matches(openNames, tokens, isAbsolute);
            if (matched)
            {
                if (mode == QueryMode::OUTER)
                {
                    if (selfclose)
                    {
                        if (!firstOut)
                            cout << '\n';
                        cout << string(s.substr(i, gt - i + 1));
                        firstOut = false;
                        any = true;
                        openNames.resize(oldSz);
                        i = gt + 1;
                        continue;
                    }
                    auto [endPos, closeLt] = find_element_end_and_close_lt(s, i, gt, nameU);
                    if (!firstOut)
                        cout << '\n';
                    cout << string(s.substr(i, endPos - i));
                    firstOut = false;
                    any = true;
                    openNames.resize(oldSz);
                    i = endPos;
                    continue;
                }
                else // TEXT
                {
                    if (selfclose)
                    {
                        openNames.resize(oldSz);
                        i = gt + 1;
                        continue;
                    }
                    auto [endPos, closeLt] = find_element_end_and_close_lt(s, i, gt, nameU);
                    size_t contentBeg = gt + 1;
                    size_t contentEnd = (closeLt > contentBeg ? closeLt : contentBeg);
                    string txt = extract_text_in_range(s, contentBeg, contentEnd);
                    if (!firstOut)
                        cout << '\n';
                    cout << txt;
                    firstOut = false;
                    any = true;
                    openNames.resize(oldSz);
                    i = endPos;
                    continue;
                }
            }

            if (selfclose)
                openNames.pop_back();
            i = gt + 1;
        }
        else
        {
            if (!openNames.empty())
                openNames.pop_back();
            i = gt + 1;
        }
    }

    return any;
}

// 块级元素集合（用于Text输出时换行）
static inline bool is_block_element(const string &nameU)
{
    static const char *arr[] = {"HTML", "HEAD", "BODY", "ADDRESS", "ARTICLE", "ASIDE", "BLOCKQUOTE", "CANVAS", "DD", "DIV", "DL", "DT", "FIELDSET", "FIGCAPTION", "FIGURE", "FOOTER", "FORM", "H1", "H2", "H3", "H4", "H5", "H6", "HEADER", "HR", "LI", "MAIN", "NAV", "NOSCRIPT", "OL", "P", "PRE", "SECTION", "TABLE", "TFOOT", "UL", "VIDEO", "TR", "TD", "TH"};
    for (auto s : arr)
        if (nameU == s)
            return true;
    return false;
}

// 在 [contentBeg, contentEnd) 范围内提取文本（忽略标签、注释、脚本样式），并在块级元素闭合时插入换行
static string extract_text_in_range(string_view s, size_t contentBeg, size_t contentEnd)
{
    string acc;
    size_t i = contentBeg;
    while (i < contentEnd)
    {
        if (s[i] != '<')
        {
            acc.push_back(s[i]);
            ++i;
            continue;
        }

        // 注释
        if (i + 3 < contentEnd && s.substr(i, 4) == "<!--")
        {
            i = skip_comment(s, i);
            continue;
        }

        size_t gt = find_tag_gt(s, i);
        if (gt == string::npos || gt >= contentEnd)
            break;

        // 识别标签名
        size_t p = i + 1;
        bool closing = false;
        if (p <= gt && s[p] == '/')
        {
            closing = true;
            ++p;
        }
        size_t nb = p;
        while (nb <= gt && is_alpha_num(nb < s.size() ? s[nb] : '\0'))
            ++nb;
        string nameU = to_upper(string(s.substr(p, nb - p)));

        // script/style 整块跳过
        if (!closing && (nameU == "SCRIPT" || nameU == "STYLE"))
        {
            i = skip_script_style_block(s, i, gt, nameU);
            continue;
        }

        // 自闭合判断
        bool selfclose = false;
        size_t q = gt;
        while (q > i && isspace((unsigned char)s[q - 1]))
            --q;
        if (q > i && s[q - 1] == '/')
            selfclose = true;
        if (is_self_closing_by_name(nameU))
            selfclose = true;

        // 文本换行策略
        if (nameU == "BR")
        {
            acc.push_back('\n');
        }
        else if (closing && is_block_element(nameU))
        {
            acc.push_back('\n');
        }
        // 跳过标签本身
        i = gt + 1;
    }
    return normalize_text_preserve_newlines(acc);
}

// 合并空白但保留换行：
// - 连续空格/Tab/\r 合并为单个空格
// - 保留我们在解析过程中插入的'\n'，并折叠为单个'\n'
// - 去除每行首尾空格
static string normalize_text_preserve_newlines(const string &s)
{
    string tmp;
    tmp.reserve(s.size());
    bool inSpace = false;
    for (char ch : s)
    {
        if (ch == '\r' || ch == '\t' || ch == ' ')
        {
            if (!inSpace)
            {
                tmp.push_back(' ');
                inSpace = true;
            }
        }
        else if (ch == '\n')
        {
            // 去掉行尾空格，再保留一个换行
            if (!tmp.empty() && tmp.back() == ' ')
                tmp.pop_back();
            if (tmp.empty() || tmp.back() != '\n')
                tmp.push_back('\n');
            inSpace = false;
        }
        else
        {
            tmp.push_back(ch);
            inSpace = false;
        }
    }
    // 行级修剪：去除每行首尾空格与多余的尾部换行
    string out;
    out.reserve(tmp.size());
    size_t pos = 0;
    while (pos < tmp.size())
    {
        size_t lineEnd = tmp.find('\n', pos);
        if (lineEnd == string::npos)
            lineEnd = tmp.size();
        size_t a = pos;
        while (a < lineEnd && tmp[a] == ' ')
            ++a;
        size_t b = lineEnd;
        while (b > a && tmp[b - 1] == ' ')
            --b;
        if (b > a)
            out.append(tmp.substr(a, b - a));
        if (lineEnd < tmp.size())
            out.push_back('\n');
        pos = (lineEnd == tmp.size() ? lineEnd : lineEnd + 1);
    }
    if (!out.empty() && out.back() == '\n')
        out.pop_back();
    return out;
}

// ========== 内容规则 ==========
static inline bool is_special_block_only_inline(const string &nameU)
{
    if (nameU == "P" || nameU == "DT")
        return true;
    if (nameU.size() == 2 && nameU[0] == 'H')
    {
        char d = nameU[1];
        return d >= '1' && d <= '6';
    }
    return false;
}

static inline bool is_inline_element(const string &nameU)
{
    return !is_block_element(nameU);
}

// ========== 内容规则校验 ==========
static void validate_content_model(Error &doc)
{
    if (g_html == nullptr || g_html_len == 0)
        return;

    string_view s(g_html, g_html_len);
    size_t n = s.size();
    // 维护路径栈（大写标签名）
    vector<string> pathStack;
    pathStack.reserve(64);

    // 工具：大小写不敏感查找
    auto find_ci = [](string_view hay, string_view needle, size_t from) -> size_t
    {
        if (needle.empty() || hay.size() < needle.size() || from >= hay.size())
            return string_view::npos;
        size_t last = hay.size() - needle.size();
        auto lower = [](char ch)
        { if (ch>='A' && ch<='Z') return char(ch - 'A' + 'a'); return ch; };
        for (size_t i = from; i <= last; ++i)
        {
            size_t j = 0;
            for (; j < needle.size(); ++j)
                if (lower(hay[i + j]) != lower(needle[j]))
                    break;
            if (j == needle.size())
                return i;
        }
        return string_view::npos;
    };

    auto is_block = [&](const string &nm)
    { return is_block_element(nm); };
    auto is_inline = [&](const string &nm)
    { return !is_block_element(nm); };

    for (size_t i = 0; i < n;)
    {
        if (s[i] != '<')
        {
            ++i;
            continue;
        }

        // 注释
        if (i + 3 < n && s.compare(i, 4, "<!--") == 0)
        {
            size_t end = s.find("-->", i + 4);
            i = (end == string::npos) ? n : end + 3;
            continue;
        }

        size_t lt = i;
        size_t gt = s.find('>', lt + 1);
        if (gt == string::npos)
        {
            doc.errors.push_back("缺少闭合的 '>' at pos=" + to_string(lt));
            break;
        }

        // 标签体
        size_t p = lt + 1;
        bool closing = false;
        if (p < gt && s[p] == '/')
        {
            closing = true;
            ++p;
        }
        size_t nb = p;
        while (nb < gt && is_alpha_num(s[nb]))
            ++nb;
        string nameU = to_upper(string(s.substr(p, nb - p)));

        // 跳过 SCRIPT/STYLE 块（不参与栈）
        if (!closing && (nameU == "SCRIPT" || nameU == "STYLE"))
        {
            string needle = string("</") + nameU + ">";
            size_t closePos = find_ci(s, needle, gt + 1);
            if (closePos == string::npos)
            {
                doc.errors.push_back(string("未找到 ") + needle + " 的闭合");
                i = gt + 1;
            }
            else
            {
                size_t endGt = s.find('>', closePos);
                if (endGt == string::npos)
                {
                    doc.errors.push_back("SCRIPT/STYLE 关闭标签缺少 '>'");
                    i = gt + 1;
                }
                else
                {
                    i = endGt + 1; // 整块跳过
                }
            }
            continue;
        }

        // 空标签或无名，跳过
        if (nameU.empty())
        {
            i = gt + 1;
            continue;
        }

        // 判断自闭合
        bool selfclose = false;
        size_t q = gt;
        while (q > lt && isspace((unsigned char)s[q - 1]))
            --q;
        if (q > lt && s[q - 1] == '/')
            selfclose = true;
        if (is_self_closing_by_name(nameU))
            selfclose = true;

        if (!closing)
        {
            // 内容模型检查点（在入栈前后效果一致，这里选择入栈后检查祖先方便些）
            pathStack.push_back(nameU);

            // R1: 只要打开的是块级元素，祖先中若存在行内且不为A，则报错
            if (is_block(nameU))
            {
                for (const auto &anc : pathStack)
                {
                    if (anc == nameU)
                        break; // 只看至自身
                    if (anc != "A" && is_inline(anc))
                    {
                        doc.errors.push_back(string("行内元素 <") + anc + "> 内不能嵌套块级元素 <" + nameU + "> at pos=" + to_string(lt));
                        break;
                    }
                }
            }

            // R2: 祖先出现 H1-H6/P/DT，则不允许出现块级后代
            if (is_block(nameU))
            {
                for (const auto &anc : pathStack)
                {
                    if (anc == nameU)
                        break;
                    if (is_special_block_only_inline(anc))
                    {
                        doc.errors.push_back(string("特殊块级元素 <") + anc + "> 内只允许行内元素，发现块级元素 <" + nameU + "> at pos=" + to_string(lt));
                        break;
                    }
                }
            }

            // R3: A 不能包含 A 后代
            if (nameU == "A")
            {
                for (const auto &anc : pathStack)
                {
                    if (anc == nameU)
                        break;
                    if (anc == "A")
                    {
                        doc.errors.push_back(string("元素 <A> 不能包含嵌套的 <A> at pos=") + to_string(lt));
                        break;
                    }
                }
            }

            if (selfclose)
            {
                pathStack.pop_back();
            }

            i = gt + 1;
        }
        else
        {
            // 关闭标签：与栈匹配（仅用于顺序校验，放宽容错）
            if (!pathStack.empty())
            {
                // 若顶不匹配，仅弹出一个以尽量不中断
                if (pathStack.back() != nameU)
                {
                    doc.errors.push_back(string("标签不匹配，期望 </") + pathStack.back() + "> 实得 </" + nameU + "> at pos=" + to_string(lt));
                }
                pathStack.pop_back();
            }
            else
            {
                doc.errors.push_back(string("多余的关闭标签 </") + nameU + "> at pos=" + to_string(lt));
            }
            i = gt + 1;
        }
    }

    // 栈未清空 -> 有未闭合标签
    while (!pathStack.empty())
    {
        doc.errors.push_back(string("未闭合标签 <") + pathStack.back() + "> at pos=-1");
        pathStack.pop_back();
    }
}

// ========== 使用栈封装标签名入栈/出栈 ==========
static Status PushTag(SqStack &S, const string &tagUpper)
{
    // 先压入分隔符'\0'，再压入名称字符
    if (Push(S, '\0') != OK)
        return ERROR;
    for (char ch : tagUpper)
    {
        if (Push(S, ch) != OK)
            return ERROR;
    }
    return OK;
}

static bool PopTag(SqStack &S, string &tagUpper)
{
    // 按字符弹出直到遇到分隔符'\0'，收集反向字符串
    vector<char> rev;
    char c;
    while (Pop(S, c) == OK)
    {
        if (c == '\0')
            break;
        rev.push_back(c);
    }
    if (rev.empty() && (S.top == S.base))
    {
        // 可能弹到了空仍未遇到'\0'，视为失败
        if (rev.empty())
            return false;
    }
    reverse(rev.begin(), rev.end());
    tagUpper.assign(rev.begin(), rev.end());
    return true;
}

// =========================
// 预处理：仅缓存源与基础错误（配对、SCRIPT/STYLE块）
static void preprocess_html(const char *buf, size_t len, Error &doc, SqStack &S)
{
    // 清空错误
    doc.errors.clear();
    // 重置字符栈
    S.top = S.base;

    // 用于记录未闭合标签（只存名称，位置仅用于报错文字）
    struct OpenTag
    {
        string name;
        size_t lt;
    };
    vector<OpenTag> openTags;
    openTags.reserve(128);

    string_view s(buf, len);
    size_t i = 0, n = s.size();
    while (i < n)
    {
        if (s[i] == '<')
        {
            // 注释 <!-- ... -->
            if (i + 3 < n && s.compare(i, 4, "<!--") == 0)
            {
                size_t end = s.find("-->", i + 4);
                if (end == string::npos)
                {
                    i = n;
                    break;
                }
                else
                {
                    i = end + 3;
                    continue;
                }
            }
            size_t lt = i;
            size_t gt = s.find('>', lt + 1);
            if (gt == string::npos)
            {
                // 不完整标签，记错误并退出
                doc.errors.push_back("缺少闭合的 '>' at pos=" + to_string(lt));
                break;
            }
            // 提取标签体
            size_t p = lt + 1;
            bool closing = false;
            if (p < gt && s[p] == '/')
            {
                closing = true;
                ++p;
            }
            // 读取标签名
            size_t nameBeg = p;
            while (p < gt && is_alpha_num(s[p]))
                ++p;
            string name = to_upper(string(s.substr(nameBeg, p - nameBeg)));
            // 特殊：脚本/样式块整体跳过（不入栈）
            if (!closing && (name == "SCRIPT" || name == "STYLE"))
            {
                // 大小写不敏感搜索
                auto find_ci = [](string_view hay, string_view needle, size_t from) -> size_t
                {
                    if (needle.empty() || hay.size() < needle.size() || from >= hay.size())
                        return string_view::npos;
                    size_t last = hay.size() - needle.size();
                    auto lower = [](char ch)
                    { if (ch>='A' && ch<='Z') return char(ch - 'A' + 'a'); return ch; };
                    for (size_t i = from; i <= last; ++i)
                    {
                        size_t j = 0;
                        for (; j < needle.size(); ++j)
                            if (lower(hay[i + j]) != lower(needle[j]))
                                break;
                        if (j == needle.size())
                            return i;
                    }
                    return string_view::npos;
                };
                string closeTag = string("</") + name + ">";
                size_t closePos = find_ci(s, closeTag, gt + 1);
                if (closePos == string::npos)
                {
                    doc.errors.push_back(string("未找到 ") + closeTag + " 的闭合");
                    i = gt + 1;
                    continue;
                }
                size_t endGt = s.find('>', closePos);
                if (endGt == string::npos)
                {
                    doc.errors.push_back("SCRIPT/STYLE 关闭标签缺少 '>'");
                    i = gt + 1;
                    continue;
                }
                i = endGt + 1;
                continue;
            }

            if (name.empty())
            {
                i = gt + 1;
                continue;
            }

            bool selfclose = false;
            // 判断 '/>' 或已知自闭合
            size_t q = gt;
            while (q > lt && isspace((unsigned char)s[q - 1]))
                --q;
            if (q > lt && s[q - 1] == '/')
                selfclose = true;
            if (is_self_closing_by_name(name))
                selfclose = true;

            if (!closing)
            {
                if (!selfclose)
                {
                    openTags.push_back({name, lt});
                    // 使用提供的字符栈记录配对检查
                    PushTag(S, name);
                }
                i = gt + 1;
                continue;
            }
            else
            {
                // 关闭标签：出栈并匹配
                string expect;
                bool ok = PopTag(S, expect);
                if (!ok)
                {
                    doc.errors.push_back(string("多余的关闭标签 </") + name + "> at pos=" + to_string(lt));
                }
                else if (expect != name)
                {
                    doc.errors.push_back(string("标签不匹配，期望 </") + expect + "> 实得 </" + name + "> at pos=" + to_string(lt));
                }
                // 与打开表对齐（宽松处理：无论匹配与否，都弹出一个）
                if (!openTags.empty())
                    openTags.pop_back();
                else
                    doc.errors.push_back(string("没有对应打开的关闭标签 </") + name + "> at pos=" + to_string(lt));
                i = gt + 1;
                continue;
            }
        }
        else
        {
            ++i;
        }
    }

    // 未闭合的标签
    while (!openTags.empty())
    {
        auto t = openTags.back();
        openTags.pop_back();
        doc.errors.push_back(string("未闭合标签 <") + t.name + "> at pos=" + to_string(t.lt));
    }
}

// 读取本地文件为内存块（new[] 分配），以二进制读，去除UTF-8 BOM
Status load_file(const string &path, char *&html_content, size_t &html_len)
{
    html_content = nullptr;
    html_len = 0;

    if (!filesystem::exists(path))
    {
        cerr << "文件不存在: " << path << endl;
        return ERROR;
    }

    ifstream in(path, ios::binary);
    if (!in)
    {
        cerr << "无法打开文件: " << path << endl;
        return ERROR;
    }
    in.seekg(0, ios::end);
    streampos end = in.tellg();
    in.seekg(0, ios::beg);
    streampos start = in.tellg();
    size_t sz = static_cast<size_t>(end - start);
    string data;
    data.resize(sz);
    if (sz > 0)
        in.read(&data[0], static_cast<streamsize>(sz));
    in.close();

    // 去除 UTF-8 BOM
    if (data.size() >= 3 &&
        (unsigned char)data[0] == 0xEF &&
        (unsigned char)data[1] == 0xBB &&
        (unsigned char)data[2] == 0xBF)
    {
        data.erase(0, 3);
    }

    html_len = data.size();
    html_content = new char[html_len + 1];
    if (html_len)
        memcpy(html_content, data.data(), html_len);
    html_content[html_len] = '\0';
    return OK;
}

// =========================
// Outer_html 实现
static bool Outer_html(const string &path)
{
    if (g_html == nullptr || g_html_len == 0)
        return false;

    bool isAbsolute = false, isRootAll = false;
    vector<string> tokens = parse_path_tokens(path, isAbsolute, isRootAll);

    string_view s(g_html, g_html_len);

    if (isRootAll)
    {
        // 输出全部源代码
        cout.write(g_html, (streamsize)g_html_len);
        cout << '\n';
        return true;
    }
    if (tokens.empty())
        return false;
    return perform_query(tokens, isAbsolute, s, QueryMode::OUTER);
}

// Text 实现
static bool Text(const string &path)
{
    if (g_html == nullptr || g_html_len == 0)
        return false;

    bool isAbsolute = false, isRootAll = false;
    vector<string> tokens = parse_path_tokens(path, isAbsolute, isRootAll);

    string_view s(g_html, g_html_len);

    if (isRootAll)
    {
        // 对整棵树提取文本
        string txt = extract_text_in_range(s, 0, s.size());
        if (!txt.empty())
        {
            cout << txt << '\n';
            return true;
        }
        return false;
    }
    if (tokens.empty())
        return false;
    return perform_query(tokens, isAbsolute, s, QueryMode::TEXT);
}

int main()
{
    SqStack S;
    if (initStack(S) != OK)
    {
        cerr << "初始化栈失败！" << endl;
        return ERROR;
    }
    // 读取命令，开始操作
    cout << endl;
    cout << "[1] 解析新的文件" << endl
         << "[2] 检查此文件合法性" << endl
         << "[3] 输出对应路径下的html代码段" << endl
         << "[4] 输出对应路径下的文本" << endl // （3、4中路径为绝对路径或相对路径）
         << "[5] 退出程序，结束操作" << endl;
    cout << "请输入命令: ";
    string cmdLine;
    bool stop = false;
    while (!stop && getline(cin, cmdLine))
    {
        // 去除前后空白
        auto trim = [](string &s)
        {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r'))
                s.erase(s.begin());
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r'))
                s.pop_back();
        };
        trim(cmdLine);
        if (cmdLine.empty())
        {
            cout << "请输入命令: " << endl;
            continue;
        }
        // 将cmdLine转为int类型的ops
        int ops = 0;
        bool isInt = true;
        try
        {
            size_t idx = 0;
            ops = stoi(cmdLine, &idx);
            // 检查是否有多余字符
            if (idx != cmdLine.size())
                isInt = false;
        }
        catch (...)
        {
            isInt = false;
        }

        if (isInt)
        {
            switch (ops)
            {
            case 1:
            {
                cout << "请输入文件路径：";
                string input_path;
                getline(cin, input_path);

                char *html_content = nullptr;
                size_t html_len = 0;
                if (!load_file(input_path, html_content, html_len))
                {
                    cout << "加载HTML内容失败！" << endl;
                    delete[] html_content;
                    break;
                }
                cout << "加载成功，字节数: " << html_len << endl;
                // 替换全局HTML缓冲（保持到退出或下次加载）
                if (g_html)
                {
                    delete[] g_html;
                    g_html = nullptr;
                    g_html_len = 0;
                }
                g_html = html_content;
                g_html_len = html_len;

                // 预处理 + 内容模型校验（使用全局缓冲）
                preprocess_html(g_html, g_html_len, g_doc, S);
                validate_content_model(g_doc);
                if (!g_doc.errors.empty())
                {
                    cout << "[CheckHTML] 发现潜在不合法之处 " << g_doc.errors.size() << " 条" << endl;
                }
                else
                {
                    cout << "[CheckHTML] 标签配对正常。" << endl;
                }
                break;
            }
            case 2:
            {
                if (g_html == nullptr || g_html_len == 0)
                {
                    cout << "尚未加载任何HTML文件。" << endl;
                    break;
                }
                if (g_doc.errors.empty())
                {
                    cout << "HTML 合法!" << endl;
                }
                else
                {
                    cout << "HTML 存在问题，共 " << g_doc.errors.size() << " 条：" << endl;
                    for (auto &e : g_doc.errors)
                        cout << "- " << e << endl;
                }
                break;
            }
            case 3:
            {
                if (g_html == nullptr || g_html_len == 0)
                {
                    cout << "尚未加载任何HTML文件。" << endl;
                    break;
                }
                cout << "请输入路径（支持绝对/相对/部分路径）：";
                string path;
                getline(cin, path);
                {
                    bool any = Outer_html(path);
                    if (!any)
                    {
                        cout << "该路径无匹配！" << endl;
                        break;
                    }
                    cout << endl;
                }
                break;
            }
            case 4:
            {
                if (g_html == nullptr || g_html_len == 0)
                {
                    cout << "尚未加载任何HTML文件。" << endl;
                    break;
                }
                cout << "请输入路径（支持绝对/相对/部分路径）：";
                string path;
                getline(cin, path);
                {
                    bool any = Text(path);
                    if (!any)
                    {
                        cout << "该路径无匹配！" << endl;
                        break;
                    }
                    cout << endl;
                }
                break;
            }
            case 5:
            {
                cout << "退出程序，结束操作" << endl;
                if (g_html)
                {
                    delete[] g_html;
                    g_html = nullptr;
                    g_html_len = 0;
                }
                stop = true;
                break;
            }
            default:
                cout << "未识别命令：" << cmdLine << endl;
                break;
            }
        }
        else
        {
            cout << "未识别命令：" << cmdLine << endl;
        }
        if (!stop)
        {
            cout << "[1] 解析新的文件" << endl
                 << "[2] 检查此文件合法性" << endl
                 << "[3] 输出对应路径下的html代码段" << endl
                 << "[4] 输出对应路径下的文本" << endl
                 << "[5] 退出程序，结束操作" << endl;
            cout << "请输入命令: ";
        }
    }
    return 0;
}