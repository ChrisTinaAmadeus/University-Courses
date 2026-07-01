# Lucene：commitLock 与 merge stall 之间的环形等待

## 1. 复现信息

- 修复标题：LUCENE-7570: don't run merges while holding the commitLock to prevent deadlock when merges are stalled and a tragic merge exception strikes
- 回归测试：`TestTragicIndexWriterDeadlock.testDeadlockStalledMerges`

日志文件：

- fixed：`reports/lucene/lucene-fixed.log`
- buggy：`reports/lucene/lucene-buggy.log`
- buggy 预编译：`reports/lucene/lucene-buggy-precompile.log`
- 构建依赖处理：`reports/lucene/lucene-ivy-bootstrap.log`、`reports/lucene/lucene-ivy-sha256.txt`、`reports/lucene/lucene-junit4-ant-local-cache.txt`
- 简要状态：`reports/lucene/lucene-summary.txt`

复现结果：

```text
lucene_fixed_status=0
lucene_buggy_precompile_status=0
lucene_buggy_status=124
```

fixed 版通过；buggy 版进入目标 suite 后一直不返回，被 `timeout` 在 45 秒后结束，退出码 124。这正是回归测试想要证明的卡死现象。

## 2. 背景：提交线程与合并线程

Lucene 的 `IndexWriter` 同时管理三个操作：接收文档写入并 flush segment 到磁盘、commit 将改动持久化、merge 将小 segment 合并成大段以维持搜索效率。这些操作运行在不同的线程上，通过一套锁协议来协调。

这个 bug 涉及三个同步对象，它们之间有一条约定的获取顺序：

- `IndexWriter.commitLock`：一个独立的 `Object` 实例，专门用来保护 commit 操作的原子性。注释明确写了 lock order 是 commitLock 先于 IW。
- `IndexWriter` 自身的 monitor（即 `synchronized` 在 `this` 上）：保护 `IndexWriter` 实例的内部状态。
- `ConcurrentMergeScheduler` 的 monitor（即其 `synchronized` 实例方法上的锁）：保护 merge 调度器的内部状态，包括 merge 线程列表、pending merge 计数等。

源码中的注释很值得注意：

```java
// Used only by commit and prepareCommit, below; lock
// order is commitLock -> IW          // 约定的锁顺序：先拿 commitLock，再拿 IW
private final Object commitLock = new Object();
```

这条注释表明开发者已经意识到这里存在多把锁，也定下了获取顺序。但问题是一种特殊情况打破了这条约定：commit 线程在持有 `commitLock` 时通过调用 `maybeMerge()` 进入了 `ConcurrentMergeScheduler` 的 monitor，而 `ConcurrentMergeScheduler` 内部的 `doStall()` 会在其 monitor 上 `wait()`，这本身不违反 lock order。真正的麻烦在于另一条路径——merge 线程遭遇致命异常后需要 rollback，而 rollback 又需要 `commitLock`，于是形成了一条 `commitLock → CMS monitor → commitLock` 的环。

 Coffman 条件中，第四条是循环等待：T1 等 T2，T2 等 T3，…，Tn 等 T1，形成闭环。这个 bug 正是循环等待的典型案例——ABBA 式环形锁依赖，两把锁的获取顺序在不同线程中不一致导致了闭环。

## 3. buggy 提交路径：持有 commitLock 时调用 maybeMerge

先看 `prepareCommit()` 的入口：

```java
public final long prepareCommit() throws IOException {
  ensureOpen();                                            // 确保 IndexWriter 处于打开状态
  pendingSeqNo = prepareCommitInternal(config.getMergePolicy());
  return pendingSeqNo;
}
```

它直接调用 `prepareCommitInternal`，传入当前的 merge 策略。`prepareCommitInternal` 的主体在 `synchronized(commitLock)` 内部运行：

```java
private long prepareCommitInternal(MergePolicy mergePolicy) throws IOException {
  startCommitTime = System.nanoTime();
  synchronized(commitLock) {                               // 获取 commitLock，整个 commit 过程在此锁保护下
    ...
    boolean anySegmentsFlushed = false;                    // 本次 commit 是否有 segment 被 flush
    ...
    boolean success = false;
    try {
      if (anySegmentsFlushed) {
        maybeMerge(mergePolicy, MergeTrigger.FULL_FLUSH, UNBOUNDED_MAX_MERGE_SEGMENTS);
                                                           // 在持 commitLock 状态下触发 merge！
      }
      startCommit(toCommit);                               // 执行真正的 commit 操作
      success = true;
      ...
    } finally {
      ...
    }
  }
}
```

`maybeMerge()` 被调用的条件是 `anySegmentsFlushed` 为 true——也就是这次 commit 过程中确实有新的 segment 被 flush 出来，Lucene 认为 commit 后应该尽早触发一次 merge 以控制 segment 数量。注意 `maybeMerge(…UNBOUNDED_MAX_MERGE_SEGMENTS…)` 中的 UNBOUNDED 参数意味着此次 merge 可能涉及任意数量的 segment。

`commitInternal()` 的结构类似：

```java
private final long commitInternal(MergePolicy mergePolicy) throws IOException {
  synchronized(commitLock) {                               // 整个 commit 内部逻辑都在 commitLock 保护下
    ...
    if (pendingCommit == null) {
      seqNo = prepareCommitInternal(mergePolicy);          // 内部也会在持 commitLock 时调用 maybeMerge
    } else {
      seqNo = pendingSeqNo;
    }

    finishCommit();
    return seqNo;
  }
}
```

两者都在持有 `commitLock` 的情况下进入了 `maybeMerge()`。而这个调用链往下走，最终会进入 `ConcurrentMergeScheduler`。

### maybeMerge 的调用链

`maybeMerge()` 会经过多层调用，最终到达 `ConcurrentMergeScheduler.merge()`，这是一个 `synchronized` 实例方法。如果当前 merge 线程数已经达到上限、且调用方本身不是 merge 线程，调度器会认为 merge 积压太严重，调用 `maybeStall()` 来暂停调用方线程。

## 4. merge scheduler：调用方线程可能被 stall

`maybeStall()` 和 `doStall()` 是理解这个 bug 的关键：

```java
protected synchronized boolean maybeStall(IndexWriter writer) {
  long startStallTime = 0;
  while (writer.hasPendingMerges() && mergeThreadCount() >= maxMergeCount) {
                                                           // 条件：有积压合并 且 合并线程数已满
    if (mergeThreads.contains(Thread.currentThread())) {
      return false;                                        // 当前线程自身是 merge 线程，不 stall
    }

    startStallTime = System.currentTimeMillis();
    doStall();                                             // 让调用方线程等待
  }
  return true;
}

protected synchronized void doStall() {
  try {
    wait(250);                                             // 在 CMS monitor 上等待 250ms
  } catch (InterruptedException ie) {
    throw new ThreadInterruptedException(ie);
  }
}
```

逐行分析 `maybeStall()` 的逻辑：

1. `while` 循环的条件是两件事同时成立：有 pending merge 等待处理，并且当前 merge 线程数已经达到上限。只要这两个条件不同时成立，循环就不会进入，函数直接返回 true，允许调用方继续。
2. 在循环体内部，先检查当前线程是否本身就是 merge 线程——如果是，直接返回 false，merge 线程不被 stall，继续去做合并工作。
3. 如果当前线程不是 merge 线程，就调用 `doStall()`。
4. `doStall()` 在 `this`（即 `ConcurrentMergeScheduler` 实例）的 monitor 上调用 `wait(250)`，让当前线程等待 250 毫秒后自动醒来，然后重新检查条件。

这里有 Java 监视器的一个关键语义：调用 `wait()` 会释放当前所在的 monitor（即 `ConcurrentMergeScheduler` 实例的锁），让其他线程可以进入该实例的 `synchronized` 方法。但 `wait()` 只释放这一个 monitor，不释放调用者在外层已经持有的任何其他锁。

这个 stall 机制本来是为了性能控制：当前端写入太快而 merge 跟不上时，让提交线程暂停一下，给 merge 线程时间消化积压。但如果这个被暂停的线程手里还攥着 `commitLock`，情况就变了：它现在是一个在 stall 中持有 `commitLock` 的线程，而 `wait()` 只释放 `ConcurrentMergeScheduler` 的 monitor，**不释放** `commitLock`。

## 5. 另一条路径：合并失败走 tragicEvent → rollback

合并线程在执行 merge 时可能遇到不可恢复的异常（比如磁盘满、索引损坏等）。这时 Lucene 的处理流程是调用 `tragicEvent()`：

```java
void tragicEvent(Throwable tragedy, String location) throws IOException {
  ...
  try {
    rollbackInternal();              // 遇到致命错误，回滚整个 IndexWriter
  } catch (Throwable t) {
    ...
  }
}
```

`tragicEvent` 需要回滚整个 `IndexWriter` 的状态，而 `rollbackInternal` 又需要获取 `commitLock`：

```java
private void rollbackInternal() throws IOException {
  synchronized(commitLock) {         // 回滚操作需要 commitLock 来保证原子性
    rollbackInternalNoCommit();
  }
}
```

这条路径本身在正常流程下是没问题的：merge 线程不持有 `commitLock`，直接获取即可。但如果此时 `commitLock` 正被一个在 stall 中的 commit 线程持有，merge 线程就会被阻塞在 `synchronized(commitLock)` 上。

## 6. 死锁的交错过程

回归测试精准地构造了以下场景来触发 ABBA 环形等待：

1. commit 线程调用 `commitInternal()`，进入 `synchronized(commitLock)` 块，成功拿到 `commitLock`（这是锁 A）。
2. commit 过程中 flush 了新的 segment，在持有 `commitLock` 的状态下调用 `maybeMerge()`，一路进入 `ConcurrentMergeScheduler.merge()`，这里是 `synchronized` 实例方法，线程进入 `ConcurrentMergeScheduler` 的 monitor（这是锁 B）。
3. merge 积压达到上限，commit 线程进入 `maybeStall()` → `doStall()` → `wait(250)`。此时，commit 线程释放了锁 B（`ConcurrentMergeScheduler` 的 monitor，因为 `wait()` 会释放它），但仍然持有锁 A（`commitLock`，因为 `wait()` 只释放它所在的那个 monitor）。
4. 后台 merge 线程发生致命异常，进入 `tragicEvent()` → `rollbackInternal()`，需要获取 `synchronized(commitLock)`——也就是锁 A。
5. 此时：commit 线程持有锁 A，在 stall 中等待 merge 完成（需要 merge 线程来消化积压）；merge 线程等待锁 A 来做 rollback。形成 A→B→A 的环。

用 Coffman 条件来逐条对照：

- **互斥**：`commitLock` 和 CMS monitor 都是互斥的，同一时间只能被一个线程持有。✓
- **持有并等待**：commit 线程持有 `commitLock` 的同时，在 stall 中等待 merge 线程完成工作。✓
- **不可剥夺**：Java 的 `synchronized` 锁不能被外部强制释放。✓
- **循环等待**：commit 线程持有 commitLock 等 CMS→merge 完成，merge 线程需要 commitLock 做 rollback。形成 commit→merge→commit 的闭环。✓

四个条件全部满足，死锁是确定性的。这个 bug 特别隐蔽的地方在于：在锁的持有范围太大时，一个看似无关的 `wait()` 操作造成了跨锁层级的阻塞。Java 的 `wait()` 机制只释放调用 `wait()` 所在的那个 monitor，这个设计本身是正确且必要的，但在这里恰好因为外层锁 `commitLock` 的范围过大而构成了陷阱。

## 7. fixed 版本：把 maybeMerge 移出 commitLock

修复的思路是在 `commitLock` 临界区内只判断是否需要 merge（设置一个标志），在释放 `commitLock` 之后再去真正执行 merge：

```java
private long prepareCommitInternal(boolean[] doMaybeMerge) throws IOException {
                                                           // doMaybeMerge 是长度为1的数组，用作 out-参数
  startCommitTime = System.nanoTime();
  synchronized(commitLock) {
    ...
    if (anySegmentsFlushed) {
      doMaybeMerge[0] = true;                              // 只设标志，不在此处真正执行 merge
    }
    startCommit(toCommit);                                 // commit 操作仍在锁内完成
    ...
  }
}
```

函数签名变了：新增了一个 `boolean[] doMaybeMerge` 参数，它是一个长度为 1 的数组，用来在 `synchronized` 块内外传递一个布尔标志。之所以用数组而不是直接返回 boolean，是因为函数原本需要返回 `long` 类型的 seqNo，Java 不支持多返回值，而用数组做 out-parameter 是 Lucene 这个历史时期的常见做法。

上层调用方 `prepareCommit()` 和 `commitInternal()` 的模式变为先在锁内确定是否需要 merge，再到锁外执行：

```java
public final long prepareCommit() throws IOException {
  ensureOpen();
  boolean[] doMaybeMerge = new boolean[1];                 // out-参数，用于从 prepareCommitInternal 传回标志
  pendingSeqNo = prepareCommitInternal(doMaybeMerge);      // 锁内只设标志，不执行 merge
  if (doMaybeMerge[0]) {                                   // 释放 commitLock 之后
    maybeMerge(config.getMergePolicy(), MergeTrigger.FULL_FLUSH, UNBOUNDED_MAX_MERGE_SEGMENTS);
                                                           // 在锁外执行真正的 merge
  }
  return pendingSeqNo;
}

private final long commitInternal(MergePolicy mergePolicy) throws IOException {
  boolean[] doMaybeMerge = new boolean[1];
  long seqNo;

  synchronized(commitLock) {                               // commitLock 临界区：只包含状态修改和 finishCommit
    ...
    if (pendingCommit == null) {
      seqNo = prepareCommitInternal(doMaybeMerge);         // 锁内只设标志
    } else {
      seqNo = pendingSeqNo;
    }
    finishCommit();                                        // 完成 commit
  }                                                        // 临界区结束，commitLock 已释放

  if (doMaybeMerge[0]) {                                   // 在锁外执行 merge
    maybeMerge(mergePolicy, MergeTrigger.FULL_FLUSH, UNBOUNDED_MAX_MERGE_SEGMENTS);
  }

  return seqNo;
}
```

比较 buggy 和 fixed 两个版本的 `commitInternal`：

- buggy 版：`synchronized(commitLock)` 包围了 `prepareCommitInternal`（内含 `maybeMerge`）和 `finishCommit`。
- fixed 版：`synchronized(commitLock)` 只包围了 `prepareCommitInternal`（不含 `maybeMerge`，只设置标志）和 `finishCommit`。`maybeMerge` 被移到了 `synchronized` 块之后、`return` 之前。

这个改动意味着即使 commit 线程在 `maybeMerge()` 里被 stall，它也已经不持有 `commitLock`。同时，merge 线程如果在此时遭遇 tragedy 需要 rollback，可以顺利获取 `commitLock`，不会被一个正在 stall 的线程阻塞。从 Coffman 条件的角度看，修复打掉了循环等待这个条件——commit 线程不再在持有 commitLock 时等待 merge 线程，锁依赖图从 `commitLock → CMS → commitLock` 的环形变成了 `commitLock → …（释放）→ CMS → …` 的线性。

## 8. 复现日志里的证据

fixed 版日志 `reports/lucene/lucene-fixed.log`：

```text
OK      0.09s | TestTragicIndexWriterDeadlock.testDeadlockStalledMerges
BUILD SUCCESSFUL
exit_code=0
```

buggy 版日志 `reports/lucene/lucene-buggy.log`：

```text
Suite: org.apache.lucene.index.TestTragicIndexWriterDeadlock
exit_code=124
result: timed out / killed, expected for buggy deadlock reproduction
```

buggy 版已经进入目标测试 suite（`Suite: org.apache.lucene.index.TestTragicIndexWriterDeadlock`），说明 `compile-test` 阶段成功了，但测试执行后没有正常返回，最终被外层 `timeout` 在 45 秒后杀掉，退出码 124。由于这是死锁，测试进程内部的两个线程互相等待对方持有的锁，没有任何线程能继续推进，测试框架永远收不到结果。这正是 ABBA 死锁在自动化测试中的典型行为。

## 9. 小结

Lucene 这个 bug 的核心问题是 `commitLock` 的持有范围过大，覆盖了一个可能阻塞并等待其他线程的 merge 调度路径。Java 的 `wait()` 只释放当前 monitor、不释放外层锁的语义，使得在 `commitLock` 内进入 `ConcurrentMergeScheduler` 的 stall 等待时，`commitLock` 被无意中带入了等待区间，而 merge 线程的异常恢复路径又需要 `commitLock`，于是构成 ABBA 环。

修复的策略是缩短 `commitLock` 的持有范围：在锁内只做判断和状态修改，把可能阻塞的 `maybeMerge()` 移到锁外执行。这个模式在并发编程中很常见——把 I/O、网络调用、可能阻塞的子系统交互等操作尽量放到锁外，锁只保护必要的状态读写。从 lockdep 的视角看，这个修复相当于打掉了锁依赖图中从 `CMS monitor` 回到 `commitLock` 的边——merge 线程的 rollback 路径不再需要和 commit 线程的 stall 路径共享同一条锁获取顺序，依赖图从有环变为无环。
