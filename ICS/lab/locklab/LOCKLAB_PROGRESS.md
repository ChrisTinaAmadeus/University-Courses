# Lock Lab 初步进展

更新时间：2026-06-19

## 0. 当前状态

已完成：

- 阅读并梳理 `README.md` 的实验要求。
- 获取 zap、Lucene/Solr、Netty 三个用户态 bug 的上游源码。
- 为三个 bug 各建立 buggy / fixed 两个 worktree，后续复现不用反复 checkout。
- 定位三个 bug 的修复 commit、父提交、回归测试名称和核心锁问题。
- 按“只服务当前 lab”的方式安装本地工具链到 `.lab-tools/`，没有使用系统包管理器，也没有修改 base 环境。
- 完成 zap / Lucene / Netty 三个 bug 的复现与 fixed 版验证，日志保存在 `results/`。
- 获取 Linux 内核源码的按需 sparse checkout，用于研究内核并发检测机制。
- 完成 Linux 内核三个自动检测机制的源码级讲解，文档保存在 `results/linux/linux-detection-analysis.md`。

当前本地工具链：

- 环境脚本：`source ./lab-env.sh`
- Go：`go1.26.4 linux/amd64`
- Java：`Temurin OpenJDK 1.8.0_492`
- Ant：`Apache Ant 1.10.14`
- Maven：`Apache Maven 3.9.9`
- 复现总汇总：`results/SUMMARY.md`
- 项目分析文档：`results/zap/zap-analysis.md`，`results/netty/netty-analysis.md`，`results/lucene/lucene-analysis.md`

源码目录：

| 项目 | 主源码 | buggy worktree | fixed worktree |
| --- | --- | --- | --- |
| zap | `sources/zap` | `sources/zap-buggy` | `sources/zap-fixed` |
| Lucene/Solr | `sources/lucene-solr` | `sources/lucene-solr-buggy` | `sources/lucene-solr-fixed` |
| Netty | `sources/netty` | `sources/netty-buggy` | `sources/netty-fixed` |
| Linux | `sources/linux` | 不适用 | 不适用 |

`sources/` 已加入 `.gitignore`，避免把外部上游源码误提交到作业仓库。

## 1. zap：BufferedWriteSyncer Stop / flush 自死锁

上游仓库：<https://github.com/uber-go/zap>

定位结果：

| 项 | 内容 |
| --- | --- |
| 修复 commit | `3ffa0c00594205c2c623aa9493847217982b558d` |
| 父提交 buggy 版 | `b15585bc7a2b383592004f75df35fa2088db5481` |
| 修复标题 | `Fix deadlock when Stop and flush race (#1430)` |
| 回归测试 | `TestBufferWriter/stop_race_with_flush` |
| 相关文件 | `zapcore/buffered_write_syncer.go`，`zapcore/buffered_write_syncer_test.go` |

初步理解：

- 锁类型：Go `sync.Mutex` 上的自死锁 / goroutine 间环形等待。
- 关键锁：`BufferedWriteSyncer.mu`。
- buggy 交错：
  - `Stop()` 获得 `s.mu`。
  - `Stop()` 停 ticker、关闭 `s.stop`，然后仍然在临界区内执行 `<-s.done`，等待后台 `flushLoop` 退出。
  - 如果此时 `flushLoop` 已经从 `s.ticker.C` 分支醒来，它会调用 `s.Sync()`。
  - `Sync()` 也需要获得 `s.mu`，但 `s.mu` 被 `Stop()` 持有。
  - 于是 `Stop()` 等 `flushLoop` 关闭 `done`，`flushLoop` 等 `Stop()` 释放 `s.mu`，形成死锁。
- 修复方式：
  - `Stop()` 在持锁区域内只修改状态、关闭 stop channel。
  - 把 `<-s.done` 移到释放 `s.mu` 之后。
  - 这样即使 `flushLoop` 正在准备调用 `Sync()`，也能拿到 `s.mu` 并退出。

后续复现命令草案：

```bash
cd sources/zap-fixed
go test ./zapcore -run '^TestBufferWriter$/^stop_race_with_flush$' -count=1000 -timeout=5s
```

buggy 版本需要把修复 commit 中新增的测试补到父提交上，只补测试、不补源码修复：

```bash
cd sources/zap-buggy
git show 3ffa0c00594205c2c623aa9493847217982b558d -- zapcore/buffered_write_syncer_test.go | git apply
go test ./zapcore -run '^TestBufferWriter$/^stop_race_with_flush$' -count=1000 -timeout=5s
```

实际结果：

- buggy：`results/zap/zap-buggy.log` 中 3s 超时，栈显示 `Stop()` 等 `<-s.done`，`flushLoop()` 卡在 `Sync()` 获取 `s.mu`。
- fixed：`results/zap/zap-fixed.log` 通过。

## 2. Lucene：commitLock 与 merge stall 的环形等待

上游仓库：<https://github.com/apache/lucene-solr>

定位结果：

| 项 | 内容 |
| --- | --- |
| 修复 commit | `ea3f8363319955c589eb3a7df59a031621852d3e` |
| 父提交 buggy 版 | `72537fd2cd8caf1d85c0ec0ffac52b167f6982b2` |
| 修复标题 | `LUCENE-7570: don't run merges while holding the commitLock to prevent deadlock when merges are stalled and a tragic merge exception strikes` |
| 回归测试 | `TestTragicIndexWriterDeadlock.testDeadlockStalledMerges` |
| 相关文件 | `lucene/core/src/java/org/apache/lucene/index/IndexWriter.java`，`lucene/core/src/test/org/apache/lucene/index/TestTragicIndexWriterDeadlock.java` |

初步理解：

- 锁类型：Java monitor 上的 ABBA / 环形等待。
- 关键同步对象：
  - `IndexWriter.commitLock`
  - `ConcurrentMergeScheduler` 对象监视器，也就是其 `synchronized` 方法上的锁。
- buggy 交错：
  - 提交线程进入 `prepareCommitInternal` / `commitInternal`，持有 `commitLock`。
  - 提交过程中如果 flush 产生新 segment，会在持有 `commitLock` 时调用 `maybeMerge(...)`。
  - `maybeMerge(...)` 进入 `ConcurrentMergeScheduler.merge(...)`，当 pending merges 太多且 merge 线程数达到上限时，调用 `maybeStall` / `doStall` 等待后台 merge 消化积压。
  - 另一边，后台 merge 线程如果遇到 fatal exception，会进入 `tragicEvent(...)`，最终触发 rollback。
  - rollback 路径需要进入 `rollbackInternal()`，而它也要获取 `commitLock`。
  - 于是提交线程持有 `commitLock` 等 merge 完成；merge 线程要处理 tragedy / rollback，又等 `commitLock`，形成确定性死锁。
- 修复方式：
  - 不再在持有 `commitLock` 时运行 `maybeMerge(...)`。
  - `prepareCommitInternal` 只记录 `doMaybeMerge[0] = true`。
  - `prepareCommit()` / `commitInternal()` 释放 `commitLock` 后，再根据标志调用 `maybeMerge(...)`。
  - 这样 commitLock 不再覆盖可能 stall 的 merge 调度路径，打破等待环。

后续复现命令草案：

Lucene 这个历史版本需要 JDK 8 和 Ant。建议进入 `lucene/core` 子目录运行单测：

```bash
cd sources/lucene-solr-fixed/lucene/core
ant test -Dtestcase=TestTragicIndexWriterDeadlock -Dtests.method=testDeadlockStalledMerges
```

buggy 版本同样需要把修复 commit 中新增的测试补到父提交上，只补测试、不补源码修复：

```bash
cd sources/lucene-solr-buggy
git show ea3f8363319955c589eb3a7df59a031621852d3e -- lucene/core/src/test/org/apache/lucene/index/TestTragicIndexWriterDeadlock.java | git apply
cd lucene/core
ant test -Dtestcase=TestTragicIndexWriterDeadlock -Dtests.method=testDeadlockStalledMerges
```

构建注意：

- 必须使用 JDK 8；更高版本可能因旧 Ant / Ivy / forbidden-apis 配置失败。
- 如果 Ivy 下载依赖时访问旧 HTTP Maven 源失败，优先检查：
  - `lucene/common-build.xml` 中的 `ivy_bootstrap_url1`
  - `lucene/default-nested-ivy-settings.xml` 中的 Maven mirror
- 可将 `http://repo1.maven.org/maven2` 等旧地址改为 `https://repo1.maven.org/maven2` 或 `https://repo.maven.apache.org/maven2`。

实际结果：

- buggy：`results/lucene/lucene-buggy.log` 中目标 suite 启动后卡住，被 `timeout` 在 45s 后结束，退出码 `124`。
- fixed：`results/lucene/lucene-fixed.log` 通过，`TestTragicIndexWriterDeadlock.testDeadlockStalledMerges` 为 `OK`。

## 3. Netty：DefaultPromise 条件变量丢唤醒

上游仓库：<https://github.com/netty/netty>

定位结果：

| 项 | 内容 |
| --- | --- |
| 修复 commit | `9380fdeb43fb841b567612947bdadad6ba3cddec` |
| 父提交 buggy 版 | `ff51dfce032e55b7bd0f46a69a2578771075257e` |
| 修复标题 | `Avoid missed signals on a default promise` |
| 回归测试 | `DefaultPromiseTest.testSignalRace` |
| 相关文件 | `common/src/main/java/io/netty/util/concurrent/DefaultPromise.java`，`common/src/test/java/io/netty/util/concurrent/DefaultPromiseTest.java` |

初步理解：

- 锁类型：Java monitor 条件变量丢唤醒，不是传统死锁。
- 关键同步对象：`DefaultPromise` 自身，也就是 `synchronized (this)` / `wait()` / `notifyAll()` 使用的监视器。
- buggy 交错：
  - 等待线程在 `await0` 开头的锁外执行 `isDone()`，结果为 false。
  - 等待线程准备进入 `synchronized (this)`，但尚未 `incWaiters()` / `wait()`。
  - 完成线程调用 `setValue0(...)`，CAS 写入 `result`。
  - 完成线程进入 `checkNotifyWaiters()`，发现 `waiters == 0`，所以不 `notifyAll()`。
  - 等待线程随后进入 monitor，增加 `waiters` 并调用带超时的 `wait(...)`。
  - 因为通知已经错过，它只能睡到 timeout，表现为“本该立刻返回却异常等待很久”。
- 修复方式：
  - 在 `await0` 的 `synchronized (this)` 块内、`incWaiters()` 和 `wait()` 之前，补一次 `if (isDone()) return true;`。
  - 这遵守了条件变量规则：检查条件和进入等待必须在同一把锁保护下完成。

后续复现命令草案：

只测 `common` 模块即可：

```bash
cd sources/netty-fixed
mvn -pl common -Dtest=DefaultPromiseTest#testSignalRace test
```

buggy 版本补入测试：

```bash
cd sources/netty-buggy
git show 9380fdeb43fb841b567612947bdadad6ba3cddec -- common/src/test/java/io/netty/util/concurrent/DefaultPromiseTest.java | git apply
mvn -pl common -Dtest=DefaultPromiseTest#testSignalRace test
```

实际结果：

- 上游 `testSignalRace` 在本机调度下没有撞中竞态，buggy/fixed 都通过，日志见 `results/netty/netty-buggy.log` 和 `results/netty/netty-fixed.log`。
- 为稳定复现，在两个 Netty worktree 中添加了同一个本地测试 `testDeterministicSignalRace`，只改测试、不改生产代码。
- buggy：`results/netty/netty-buggy-deterministic.log` 失败，等待约 `1000583490 ns`，超过 `500000000 ns` 阈值。
- fixed：`results/netty/netty-fixed-deterministic.log` 通过。

## 4. Linux 内核检测机制调研

上游仓库：<https://github.com/torvalds/linux>

当前状态：

- 已保留 Linux sparse checkout 于 `sources/linux`。
- 当前源码提交：`9ecfb2f7287a967b418ba69f10d45ead0d360593`。
- 已完成三种机制的详细中文讲解：`results/linux/linux-detection-analysis.md`。
- 覆盖机制：
  - lockdep / `CONFIG_PROVE_LOCKING`：通过 `lock_class`、`held_lock`、`lock_list`、`lock_chain` 记录锁依赖图，加边前 BFS 判环，提前报告潜在环形锁依赖。
  - KCSAN / `CONFIG_KCSAN`：通过编译器内存访问插桩、采样 watchpoint 和延迟窗口检测内核数据竞争。
  - `CONFIG_DEBUG_ATOMIC_SLEEP`：通过 `might_sleep()`、`preempt_count`、IRQ 状态和 `current->non_block_count` 检测不可睡眠上下文里的睡眠调用。

## 5. 下一步清单

- 无，已完成实验全部内容。
