# 第三步（Step 3）Benchmark 操作全流程：如何拿到不同 glibc 版本并做对比

这份文档解决一个核心问题：**同一台机器上，怎么“切换”到不同版本的 glibc（malloc），并用同一份 benchmark 做公平对比**。

你有两条路线：

- **路线 A（推荐）**：用容器跑不同 Ubuntu 版本（每个镜像自带不同 glibc）
- **路线 B（备选）**：不依赖容器，直接从 `glibc-upstream/` 的 tag 构建多个 glibc，并用对应的动态链接器运行程序

只需要完成其中一条路线即可。

---

## 路线 A：用容器（Docker/Podman）获取不同 glibc 版本（推荐）

### A0. 你最终会得到什么

- 每个镜像一份结果文件：`benchmark/results/<image>.txt`
- 结果里会包含：镜像信息、glibc 版本、以及每个 workload 的耗时（ns/op）
- 你可以把结果表格贴进报告 [report/report.md](../report/report.md)

### A1. 准备容器运行环境

1) **普通 Linux**：安装 Docker Engine 或 Podman（按你发行版的官方文档来即可）。

2) **如果你在 WSL2 + Docker Desktop**（你当前终端提示过这个问题）：

- Docker Desktop → Settings → Resources → WSL integration
- 勾选你的 WSL 发行版
- 重新打开终端验证：

```bash
docker version
```

只要 `docker version` 能正常输出（不报 WSL integration 的错误），就可以继续。

### A2. 选择 3 个“有代表性”的版本

你至少要 3 个不同 glibc 版本。推荐两套选择：

**（简单稳定）**：都比较常见、镜像通常还能顺利 `apt-get` 装编译器

- `ubuntu:18.04`（通常 glibc 2.27 左右）
- `ubuntu:20.04`（通常 glibc 2.31 左右）
- `ubuntu:22.04`（通常 glibc 2.35 左右，包含 THP 相关 tunable）

**（更贴近“tcache 引入前后”）**：包含更老版本（可能需要把 apt 源切到 old-releases）

- `ubuntu:16.04`（通常 glibc 2.23 左右，tcache 之前）
- `ubuntu:18.04`
- `ubuntu:20.04`

不需要死记“Ubuntu ↔ glibc”的对应关系，因为下一步会在容器里用命令**现场验证**。

### A3. 验证每个镜像里的 glibc 版本（非常关键）

你可以直接跑：

```bash
docker run --rm ubuntu:22.04 bash -lc 'getconf GNU_LIBC_VERSION'
# 或者：
# docker run --rm ubuntu:22.04 bash -lc 'ldd --version | head -n1'
```

输出形如：`glibc 2.35`，把它记到你的实验记录里。

### A4. 跑 benchmark（一键脚本）

仓库里我已经放好：

- 基准程序源码：`benchmark/bench_malloc.c`
- 一键运行脚本：`benchmark/run_docker_bench.sh`

直接运行（默认会测 16.04/18.04/20.04）：

```bash
bash benchmark/run_docker_bench.sh
```

> 当前脚本默认会跑一组“比较均衡”的 workload：`small + burst + large_touch`，并在支持的版本上额外跑 `tcache_off` 对照（便于解释 tcache 的贡献）。

如果你想自定义镜像：

```bash
bash benchmark/run_docker_bench.sh ubuntu:16.04 ubuntu:18.04 ubuntu:20.04
```

结果会写到：`benchmark/results/`。

### A5. 这个 benchmark 在测什么（和第二步优化点如何对应）

`benchmark/bench_malloc.c` 内置了 3 类 workload（同一个程序，在不同 glibc 下跑）：

- `small`：反复 `malloc(size)` / `free()`，典型用来观察 **tcache** 对小对象热路径的影响
- `burst`：一次性分配一批小对象再释放，容易触发/填满 tcache，观察缓存命中/回收行为
- `large_touch`：分配大块内存并触摸页面（touch pages），对 **THP（Transparent Huge Pages）** 更敏感

脚本默认还会跑两个“变体”（用 tunable 强行开/关某些优化，帮助你解释现象）：

- `variant=tcache_off`：`GLIBC_TUNABLES=glibc.malloc.tcache_count=0`（在 2.26+ 可基本禁用 tcache）
- `variant=thp_madvise`：`GLIBC_TUNABLES=glibc.malloc.hugetlb=1`（在 2.35+ 启用 `madvise(MADV_HUGEPAGE)` 路径）

这样你即使没选到“tcache 引入前”的老版本，也可以通过同版本开/关 tcache 来证明它的性能贡献。

> 提示：tcache 引入前（例如 glibc 2.23）没有 tcache，因此脚本会在结果里输出 `skip=tcache_off (glibc<2.26, no tcache)`，这是正常的。

### A5.1（可选）启用/关闭某些 workload 或变体

如果你只想聚焦 tcache（少跑点、结果更“干净”），可以这样关掉 `large_touch`：

```bash
FOCUS_TCACHE=1 bash benchmark/run_docker_bench.sh ubuntu:16.04 ubuntu:18.04 ubuntu:20.04
```

如果你确实想测 THP tunable（需要 glibc 2.35+，例如 `ubuntu:22.04`），可以显式打开：

```bash
RUN_THP=1 \
  bash benchmark/run_docker_bench.sh ubuntu:18.04 ubuntu:20.04 ubuntu:22.04
```

### A6. 结果怎么用到报告里

每个 `benchmark/results/*.txt` 文件里都有类似这样的行：

- `glibc=2.35`（程序自己打印的）
- `mode=small,...,ns_per_op=...`

你最简单的做法：

1) 从三个结果文件里，挑同一个 `mode`/`threads` 的 `ns_per_op`
2) 做一个表格（越小越快）
3) 在分析里对应解释：tcache / THP 为什么会让某个 workload 变快（或为什么没变快）

### A7. 让数据更可信的几个小技巧

- 同一组参数至少跑 3 次，取中位数（或最小值）
- 尽量避免同时跑别的重负载程序
- 如果你在物理机上：可以用 `taskset` 把进程绑核（例如 `taskset -c 0 ...`），减少调度噪声

---

## 路线 B：不用容器，自己构建多个 glibc（备选）

当你没有 Docker/Podman（或学校机器不让开容器）时，可以走这条路线。

> 这条路线比容器法更“硬核”，但原理更直接：你在本机上同时装好多个 `glibc`，然后用它们各自的 `ld-linux` 来启动同一个程序。

### B1. 选择要构建的 glibc tag

建议优先选“第二步时间线”里的关键节点，例如：

- `glibc-2.25`（tunables 框架已进入主线）
- `glibc-2.26`（tcache 引入）
- `glibc-2.35`（THP 相关 tunable：`glibc.malloc.hugetlb`）

### B2. 准备依赖

不同发行版依赖名略有差异，Ubuntu/WSL 上常见的一组是：

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential gawk bison python3 texinfo gettext \
  libselinux1-dev
```

（如果你缺包，configure/make 会报错；按报错把依赖补齐即可。）

### B3. 为每个版本单独建 build/install 目录

推荐用 `git worktree`，避免你来回 `git checkout`：

```bash
cd glibc-upstream
mkdir -p ../glibc-worktrees

git worktree add ../glibc-worktrees/src-2.25 glibc-2.25
git worktree add ../glibc-worktrees/src-2.26 glibc-2.26
git worktree add ../glibc-worktrees/src-2.35 glibc-2.35
```

然后每个版本都单独构建：

```bash
mkdir -p ../glibc-build/build-2.25 ../glibc-build/install-2.25
cd ../glibc-build/build-2.25

../../glibc-worktrees/src-2.25/configure \
  --prefix=$PWD/../install-2.25 \
  --disable-werror

make -j"$(nproc)"
make install
```

对 2.26 / 2.35 重复上述过程（改路径即可）。

### B4. 用“指定版本 glibc”运行同一个程序

关键点：不要直接 `./a.out`，而是用目标 glibc 的动态链接器启动：

1) 先找到该 glibc 安装目录里的 loader（不同版本/配置可能在 `lib` 或 `lib64`）：

```bash
find ../glibc-build/install-2.35 -name 'ld-linux*so*'
```

2) 用它运行基准程序（示例）：

```bash
LOADER=../glibc-build/install-2.35/lib/ld-linux-x86-64.so.2
LIBPATH=../glibc-build/install-2.35/lib

$LOADER --library-path "$LIBPATH" \
  ./benchmark/bench_malloc
```

> 注意：为了避免“新 glibc 编译出来的二进制在旧 glibc 上跑不了”，建议你用**最老的那套 glibc**编译一次 benchmark，再用不同 loader 去跑它（旧编译出来的程序通常能在新 glibc 上跑）。

---

## 常见坑（你遇到时优先看这里）

- **容器里 apt-get update 失败**：老镜像可能 EOL，脚本已经自动把源切到 `old-releases.ubuntu.com`，但如果还失败，建议换更近的 Ubuntu 版本，或者走路线 B。
- **THP 看不到提升**：`glibc.malloc.hugetlb=1` 只是“请求”内核用 THP；是否真的变成 THP 取决于内核/系统策略与访问模式。
- **结果波动很大**：把 `iters` 调大、跑多次取中位数，必要时绑核。

