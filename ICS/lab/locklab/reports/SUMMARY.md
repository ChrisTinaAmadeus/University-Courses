# Lock Lab 复现汇总

生成时间：2026-06-19

## 本地工具链

全部安装在 `.lab-tools/`，未使用系统包管理器。环境变量通过 `source ./lab-env.sh` 加载。

- Go: go1.26.4 linux/amd64
- Java: Temurin OpenJDK 1.8.0_492
- Ant: Apache Ant 1.10.14
- Maven: Apache Maven 3.9.9

Java/Ant 命令均设置 `HOME=.lab-tools/home`、`ANT_OPTS=-Duser.home=.lab-tools/home`，避免污染真实 home 目录。

三个用户态 bug 各建立了 buggy / fixed 两个 git worktree（`sources/<project>-buggy` 和 `sources/<project>-fixed`），复现时无需反复 checkout。Linux 内核源码通过 sparse checkout 放在 `sources/linux`。`sources/` 已加入 `.gitignore`。

## zap

- Buggy: `b15585bc7a2b383592004f75df35fa2088db5481`
- Fixed: `3ffa0c00594205c2c623aa9493847217982b558d`（Fix deadlock when Stop and flush race #1430）
- 测试: `TestBufferWriter/stop_race_with_flush`

从 fixed commit 提取测试补到 buggy worktree，只补测试不补源码。

- Buggy 结果: 超时，goroutine 栈显示 Stop() 等 `<-s.done`，flushLoop() 卡在 Sync() 获取 `s.mu`
- Fixed 结果: 通过

```bash
source ./lab-env.sh
cd sources/zap-buggy
go test ./zapcore -run '^TestBufferWriter$/^stop_race_with_flush$' -count=10000 -timeout=3s
cd sources/zap-fixed
go test ./zapcore -run '^TestBufferWriter$/^stop_race_with_flush$' -count=1000 -timeout=5s
```

## Netty

- Buggy: `ff51dfce032e55b7bd0f46a69a2578771075257e`
- Fixed: `9380fdeb43fb841b567612947bdadad6ba3cddec`（Avoid missed signals on a default promise）
- 上游测试: `DefaultPromiseTest.testSignalRace`
- 本地确定性测试: `DefaultPromiseTest.testDeterministicSignalRace`

上游 `testSignalRace` 本机未中竞态，两个版本均通过。为稳定复现，在 buggy/fixed 两个 worktree 各添加了同一个确定性测试，通过覆写 `isDone()` 配合 `CountDownLatch` 精确制造丢唤醒窗口，只改测试不改生产代码。

- Buggy 确定性: 等待约 1000583490ns，超过 500000000ns 阈值，失败
- Fixed 确定性: 通过

```bash
source ./lab-env.sh
cd sources/netty-buggy && mvn -pl common -Dtest=DefaultPromiseTest#testDeterministicSignalRace test
cd sources/netty-fixed && mvn -pl common -Dtest=DefaultPromiseTest#testDeterministicSignalRace test
```

## Lucene

- Buggy: `72537fd2cd8caf1d85c0ec0ffac52b167f6982b2`
- Fixed: `ea3f8363319955c589eb3a7df59a031621852d3e`（LUCENE-7570: don't run merges while holding the commitLock）
- 测试: `TestTragicIndexWriterDeadlock.testDeadlockStalledMerges`

构建调整：

- 历史 `http://repo1.maven.org` bootstrap URL 已失效，手动下载 Ivy 2.3.0 到本地
- 在 `lucene/default-nested-ivy-settings.xml` 中添加 HTTPS Maven Central resolver
- junit4-ant 2.4.0 预加载到本地 Ivy 缓存，避免走旧 HTTP resolver

- Buggy 结果: 进入目标 suite 后卡死，45s 被 timeout 结束，exit code 124
- Fixed 结果: OK

```bash
source ./lab-env.sh
# fixed
cd sources/lucene-solr-fixed/lucene/core
HOME="$LAB_HOME" ANT_OPTS="-Duser.home=$LAB_HOME" \
  ant test -Dtestcase=TestTragicIndexWriterDeadlock -Dtests.method=testDeadlockStalledMerges
# buggy（先 compile-test，再 timeout 跑防止卡死无限等）
cd sources/lucene-solr-buggy/lucene/core
HOME="$LAB_HOME" ANT_OPTS="-Duser.home=$LAB_HOME" ant compile-test
timeout --kill-after=5s 45s env HOME="$LAB_HOME" ANT_OPTS="-Duser.home=$LAB_HOME" \
  ant test -Dtestcase=TestTragicIndexWriterDeadlock -Dtests.method=testDeadlockStalledMerges
```

## Linux 内核检测机制

- 上游: torvalds/linux，本地 `sources/linux`（sparse checkout）
- 源码提交: `9ecfb2f7287a967b418ba69f10d45ead0d360593`
- 研究机制: lockdep、KCSAN、CONFIG_DEBUG_ATOMIC_SLEEP
- 文档: `reports/linux/linux-detection-analysis.md`

覆盖源码路径：

- lockdep: `include/linux/lockdep.h`, `include/linux/lockdep_types.h`, `kernel/locking/lockdep.c`
- KCSAN: `include/linux/kcsan.h`, `include/linux/kcsan-checks.h`, `kernel/kcsan/core.c`, `kernel/kcsan/report.c`
- atomic sleep debug: `include/linux/kernel.h`, `include/linux/sched.h`, `include/linux/preempt.h`, `kernel/sched/core.c`

## 日志与截图索引

| Bug | 分析文档 | Buggy 日志 | Fixed 日志 | Buggy 截图 | Fixed 截图 |
|-----|----------|-----------|-----------|-----------|-----------|
| zap | `zap/zap-analysis.md` | `zap/zap-buggy.log` | `zap/zap-fixed.log` | `zap/zap_buggy.png` | `zap/zap_fixed.png` |
| Netty | `netty/netty-analysis.md` | `netty/netty-buggy-deterministic.log` | `netty/netty-fixed-deterministic.log` | `netty/netty_buggy.png` | `netty/netty_fixed.png` |
| Lucene | `lucene/lucene-analysis.md` | `lucene/lucene-buggy.log` | `lucene/lucene-fixed.log` | `lucene/lucene_buggy.png` | `lucene/lucene_fixed.png` |
| Linux | `linux/linux-detection-analysis.md` | — | — | — | — |
