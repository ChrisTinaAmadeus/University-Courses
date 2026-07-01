# Step 3 Benchmark 结果分析（tcache 引入前后：glibc 2.23 vs 2.27 vs 2.31）

本分析基于仓库内的容器化脚本与同一份微基准程序，对三个 Ubuntu 镜像（对应三套 glibc）在同一台机器上进行对比评测，重点解释：

- 各项指标含义（`ns_per_op` 等）
- 结果表格与三版本对比
- tcache 是否真的是性能差异的主要原因（通过 `tcache_off` 对照验证）

---

## 1. 实验设置（可复现）

- 运行脚本：`benchmark/run_docker_bench.sh`
- 基准程序：`benchmark/bench_malloc.c`
- 镜像（3 个版本）：
  - `ubuntu:16.04` → glibc **2.23**（tcache 引入前）
  - `ubuntu:18.04` → glibc **2.27**（tcache 已存在）
  - `ubuntu:20.04` → glibc **2.31**（tcache 已存在）
- 结果文件：
  - `benchmark/results/ubuntu_16.04.txt`
  - `benchmark/results/ubuntu_18.04.txt`
  - `benchmark/results/ubuntu_20.04.txt`

运行命令（与本次结果一致）：

```bash
bash benchmark/run_docker_bench.sh ubuntu:16.04 ubuntu:18.04 ubuntu:20.04
```

> 注：本次是在 WSL2 内核环境下跑的（结果文件里会打印 `kernel=...WSL2`）。因此绝对数值会受宿主机/WSL 调度影响，但版本间相对趋势仍然可用。

---

## 2. 评测 workload（测什么）

基准程序内置 3 类 workload：

1) `small`
- 重复 `malloc(size)` + 写 1 字节 + `free()`
- 典型用于观察 **tcache 对小对象热路径**的影响

2) `burst`
- 每轮分配 `burst` 个小对象，写 1 字节，再全部释放（倒序 free）
- 更像“批量申请/批量释放”，会反复经历 tcache 的填充/回收

3) `large_touch`
- 分配大块内存、`memset` 并按 4KB 步长触摸页面，再释放
- 更偏向“内存触摸/页行为”，与 tcache 关系较弱（更多是 THP、TLB、内核策略等）

此外脚本会尝试跑一个对照变体：

- `variant=tcache_off`：对 glibc >= 2.26，设置 `GLIBC_TUNABLES=glibc.malloc.tcache_count=0`，用于“在同一版本里把 tcache 基本关掉”，验证性能差异是否由 tcache 引起。
- 对 glibc 2.23（tcache 之前）会显示 `skip=tcache_off`，这是正常现象。

---

## 3. 输出指标（每行意味着什么）

结果文件中每个测量行形如：

```
mode=small,threads=1,iters=2000000,size=64,burst=1024,ns_total=...,ns_per_op=...
```

关键字段含义：

- `mode`：workload 类型（`small` / `burst` / `large_touch`）
- `threads`：线程数（本脚本对 `small`/`burst` 跑 1 和 8）
- `iters`：迭代次数
- `size`：单次分配大小（字节；这里是 64）
- `burst`：仅 `burst` 模式有效，每轮分配对象个数（这里是 1024）
- `ns_total`：该次测试总耗时（纳秒）
- `ns_per_op`：**每次操作平均耗时（ns/op，越小越快）**

> 注意：当 `threads>1` 时，`ns_per_op` 更接近“总吞吐的倒数”（总体越快，平均每 op 越小），不等同于“单线程延迟”。

---

## 4. 结果表格（baseline）

参数（脚本默认）：

- `small`: `iters=2,000,000`, `size=64`
- `burst`: `rounds(iters)=20,000`, `size=64`, `burst=1024`
- `large_touch`: `iters=20`, `size=64MiB`

### 4.1 baseline：ns/op（越小越快）

| 版本（镜像 / glibc） | small t=1 | small t=8 | burst t=1 | burst t=8 | large_touch t=1 |
| -------------------- | --------: | --------: | --------: | --------: | --------------: |
| 16.04 / 2.23         |     55.36 |      8.14 |     62.40 |      7.26 |   85,320,759.85 |
| 18.04 / 2.27         |      7.63 |      1.18 |     43.20 |      5.41 |   84,111,742.05 |
| 20.04 / 2.31         |      7.86 |      1.64 |     42.10 |      5.18 |   80,591,140.05 |

### 4.2 相对 16.04（glibc 2.23）的加速比（越大越快）

（计算方式：`speedup = (2.23 的 ns/op) / (目标版本的 ns/op)`）

| 指标            | 18.04 / 2.27 | 20.04 / 2.31 |
| --------------- | -----------: | -----------: |
| small t=1       |        7.26× |        7.04× |
| small t=8       |        6.90× |        4.96× |
| burst t=1       |        1.44× |        1.48× |
| burst t=8       |        1.34× |        1.40× |
| large_touch t=1 |        1.01× |        1.06× |

**直观看法**：
- `small` 在 2.23→2.27/2.31 的提升是“量级级别”的（约 7×），非常符合 tcache 引入的预期。
- `burst` 有提升，但幅度明显更小（约 1.3~1.5×），说明该 workload 的开销并不完全由“单次 malloc/free 热路径”决定（还有循环/指针数组/写入等成本）。
- `large_touch` 差别很小，说明它主要受“触摸内存/页行为”支配，而不是 allocator 热路径。

---

## 5. tcache 归因验证：baseline vs tcache_off

下面是**同一 glibc 版本**下，把 tcache 基本关掉后的变化（用于证明“快是因为 tcache”）。

| 版本         | small t=1 baseline | small t=1 tcache_off | tcache 带来的加速 | burst t=1 baseline | burst t=1 tcache_off | tcache 带来的加速 |
| ------------ | -----------------: | -------------------: | ----------------: | -----------------: | -------------------: | ----------------: |
| 16.04 / 2.23 |              55.36 |     N/A（无 tcache） |               N/A |              62.40 |                  N/A |               N/A |
| 18.04 / 2.27 |               7.63 |                44.39 |             5.82× |              43.20 |                50.74 |             1.17× |
| 20.04 / 2.31 |               7.86 |                45.70 |             5.81× |              42.10 |                52.73 |             1.25× |

**结论（很关键）**：
- 在 glibc 2.27/2.31 中，`small` 关闭 tcache 后性能立刻从 ~7–8 ns/op 回到 ~44–46 ns/op，几乎接近 2.23 的水平。
- 这说明：**2.23→2.27 的 `small` 巨幅提升，主要归因于 tcache。**

---

## 6. 三版本对比：结果说明了什么

### 6.1 tcache 引入前后（2.23 vs 2.27）

- `small`：2.27 比 2.23 快约 7×，并且 `tcache_off` 后会退回到“接近 2.23”的速度。
- 这是一个非常“干净”的故事线：**tcache 引入显著降低小对象分配/释放的热路径成本**。

### 6.2 2.27 vs 2.31

- 两者在 `small`/`burst` 上非常接近（本次数据中 2.31 的 `small t=1` 略慢于 2.27，但差距很小）。
- 这通常意味着：
  - tcache 的“决定性提升”已经在 2.26 引入完成；
  - 后续版本更多是细节打磨/安全加固/其他子系统变动，微基准下不一定总能体现出持续单调提升。

### 6.3 为什么 `large_touch` 基本看不出差异

- `large_touch` 的成本主要来自：`memset` + 触摸 64MiB 内存（页访问、缓存、TLB、内核策略）。
- tcache 只影响分配器热路径；对“真正的内存带宽/页触摸”影响很小。
- 如果你要研究 THP（glibc 2.35+ 的 `glibc.malloc.hugetlb`），建议换到 `ubuntu:22.04` 及以上，并显式 `RUN_THP=1` 再看差异。

---

## 7. 如何把这些结果写进报告（建议模板）

你可以在报告中用这样的结构：

1) 背景：glibc 2.26 引入 tcache，预期小对象分配/释放显著加速。
2) 方法：同一份 benchmark（`small/burst/large_touch`），用容器隔离不同 glibc 版本。
3) 数据表格：引用本节 baseline 表。
4) 归因：引用 `tcache_off` 表，证明差异来自 tcache。
5) 讨论：为什么 `burst` 提升较小、为什么 `large_touch` 不敏感。

---

## 8. 备注（影响实验解释的因素）

- 该微基准是“合成负载”（synthetic），能放大 allocator 差异，但不等价于真实应用。
- 在 WSL2/虚拟化环境下，调度与 I/O 可能带来额外噪声；建议关键数据跑 3 次取中位数。
- `threads=8` 时 `ns_per_op` 反映的是总体吞吐（并发越高，总吞吐越大，ns/op 越小）。
