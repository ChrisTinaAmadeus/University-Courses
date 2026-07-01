# Netty：DefaultPromise 的条件变量丢唤醒

## 1. 复现信息

- 修复标题：Avoid missed signals on a default promise
- 上游回归测试：`DefaultPromiseTest.testSignalRace`
- 本地确定性复现测试：`DefaultPromiseTest.testDeterministicSignalRace`

日志文件：

- 上游测试 buggy：`reports/netty/netty-buggy.log`
- 上游测试 fixed：`reports/netty/netty-fixed.log`
- 确定性测试 buggy：`reports/netty/netty-buggy-deterministic.log`
- 确定性测试 fixed：`reports/netty/netty-fixed-deterministic.log`
- 简要状态：`reports/netty/netty-summary.txt`、`reports/netty/netty-deterministic-summary.txt`

复现结果：

```text
netty_buggy_deterministic_status=1
netty_fixed_deterministic_status=0
```

说明：上游 `testSignalRace` 依赖调度竞争，本机这次恰好没有撞中，两个版本都通过了。为了稳定复现，我在 buggy 和 fixed 两个 worktree 中添加了同一个确定性测试，只改测试代码，不改生产代码。buggy 版等待约 1 秒后失败，fixed 版快速通过。

## 2. Promise 的等待与唤醒机制

Netty 的 `DefaultPromise` 是异步框架中连接生产者与消费者的核心同步对象。一个线程等待 promise 完成（比如等待一个网络操作的结果），另一个线程在操作完成后设置结果并唤醒等待者。这个等待-唤醒协议由三部分组成：一个 `volatile` 结果字段（通过 `AtomicReferenceFieldUpdater` 做 CAS 更新），一个 `waiters` 计数器记录当前有多少线程正在等待，以及 Java 对象监视器的 `synchronized` / `wait()` / `notifyAll()` 机制。

关键设计点是：结果字段的写入使用 CAS 而非 `synchronized`，这样完成线程在大多数情况下不需要获取 monitor 就能快速设置结果；只有检查是否需要通知等待者以及实际执行 `notifyAll()` 时才进入 `synchronized`。但 `waiters` 计数的增减是在等待线程的 `synchronized` 块内进行的——这就构成了一个跨锁边界的协同协议，而 bug 就藏在 CAS 写入与 `synchronized` 登记之间的边界上。

这个场景和课程中讲到的竞争条件紧密相关：程序的正确性取决于等待线程和完成线程在某一时刻的相对完成度。如果完成线程在等待线程登记 `waiters` 之前就完成了 CAS 并检查了 `waiters`，正确性就被破坏了。

## 3. 完成线程：设置结果后通知等待者

buggy 版本中，完成线程通过 `setValue0` 来设置结果：

```java
private boolean setValue0(Object objResult) {
    if (RESULT_UPDATER.compareAndSet(this, null, objResult) ||       // CAS：null → objResult
        RESULT_UPDATER.compareAndSet(this, UNCANCELLABLE, objResult)) {
                                                      // CAS：UNCANCELLABLE → objResult（特殊状态处理）
        checkNotifyWaiters();                         // CAS 成功，负责通知等待者
        return true;
    }
    return false;                                     // CAS 失败，说明已有其他线程先设置了结果
}
```

`RESULT_UPDATER` 是一个 `AtomicReferenceFieldUpdater`，它对 `result` 字段执行 CAS 操作。这和课程中讲到的无锁化编程中的 CAS（Compare And Swap）完全一致——硬件保证比较并交换的原子性，`lock cmpxchgl` 指令锁定总线阻止其他核心访问。第一个 `compareAndSet` 尝试将 `result` 从 `null` 设为 `objResult`（正常完成），第二个尝试从 `UNCANCELLABLE` 设为 `objResult`（处理一种特殊的不可取消状态）。如果其中任何一个 CAS 成功，说明当前线程是第一个设置结果的人，它需要负责通知等待者，于是调用 `checkNotifyWaiters()`。

```java
private synchronized void checkNotifyWaiters() {       // synchronized 在 this (DefaultPromise) 上
    if (waiters > 0) {                                 // 只有确实有人在等才通知
        notifyAll();                                   // 唤醒所有在 this 上 wait() 的线程
    }
}
```

`checkNotifyWaiters()` 是 `synchronized` 的（锁在 `this` 上）。它检查 `waiters` 计数器，只有当确实有线程在等待时才调用 `notifyAll()`。这个优化的意图是避免无条件调 `notifyAll()` 带来的性能开销。问题在于 `waiters` 为 0 有两种可能的含义：一种是真的没有线程在等，另一种是等待线程还没来得及把自己登记进去。这个检查发生在 `synchronized` 块内，但 CAS 写入结果的操作在 `synchronized` 块外，两者之间存在一个没有同步保护的空隙。

## 4. 等待线程：buggy 版 await0

等待线程通过 `await0()` 等待 promise 完成：

```java
private boolean await0(long timeoutNanos, boolean interruptable) throws InterruptedException {
    if (isDone()) {                 // (A) 锁外第一次快速检查，无锁开销
        return true;
    }

    if (timeoutNanos <= 0) {
        return isDone();
    }

    if (interruptable && Thread.interrupted()) {
        throw new InterruptedException(toString());
    }

    checkDeadLock();

    long startTime = System.nanoTime();   // 记录开始时间，用于超时计算
    long waitTime = timeoutNanos;
    boolean interrupted = false;
    try {
        for (;;) {
            synchronized (this) {         // (B) 进入 DefaultPromise 的 monitor
                incWaiters();             // (C) waiters++，登记为等待者
                try {
                    wait(waitTime / 1000000, (int) (waitTime % 1000000));
                                          // (D) 释放 monitor 并进入等待集
                } finally {
                    decWaiters();         // (E) waiters--，无论正常/异常都恢复计数
                }
            }
            if (isDone()) {               // (F) 从 wait 返回后检查条件
                return true;
            } else {
                waitTime = timeoutNanos - (System.nanoTime() - startTime);
                                          // 计算剩余等待时间
                if (waitTime <= 0) {
                    return isDone();      // (G) 超时，最后一次检查条件后返回
                }
            }
        }
    } finally {
        if (interrupted) {
            Thread.currentThread().interrupt(); // 恢复中断状态
        }
    }
}
```

逐段分析：

- **(A) 锁外 `isDone()` 检查**：在获取 monitor 之前先快速检查一次 promise 是否已经完成。如果已完成，直接返回 true，避免了获取锁的开销。这是性能优化，但也是 bug 的关键——这次检查没有锁保护。
- **(B) 进入 `synchronized (this)`**：获取 `DefaultPromise` 实例的 monitor。在此之前，线程可能经历了任意长的调度延迟（被 OS 切走、cache miss、CPU 被其他线程占用等）。
- **(C) `incWaiters()`**：在 monitor 内部将 `waiters` 计数器加 1。这个操作是线程安全的，因为它被 `synchronized` 保护。但从 (A) 的 `isDone()` 检查失败到这一步，中间有任意长度的空窗期。
- **(D) `wait()`**：在 monitor 上等待，释放 monitor 并进入等待集。被 `notifyAll()` 或超时唤醒后会重新获取 monitor 然后返回。
- **(E) `decWaiters()`**：在 `finally` 块中将 `waiters` 计数器减 1。无论 `wait()` 是正常返回还是被中断，`waiters` 都会被正确恢复。
- **(F) wait 后 `isDone()` 检查**：从 `wait()` 返回后检查 promise 是否已完成。如果完成就返回 true。
- **(G) 超时处理**：计算剩余等待时间，如果超时就再检查一次 `isDone()` 然后返回。

关键的空隙在 (A) 和 (C) 之间：`isDone()` 返回 false 到 `incWaiters()` 执行完毕之间，`waiters` 仍然为 0，而完成线程恰好在这个窗口内检查 `waiters` 并决定不调用 `notifyAll()`。

## 5. 丢唤醒的交错过程

这个 bug 是条件变量模式中的 lost wakeup 问题。课程中讲到信号量的 P 操作在 s=0 时会挂起线程直到 s 变为非零——但如果 V 操作在 P 操作判定 s=0 之前就执行了，而 P 还没开始等待，V 的唤醒就丢失了。这里的机制是相似的。具体的交错顺序如下：

1. **等待线程** 进入 `await0`，在 (A) 位置执行锁外的 `isDone()`，结果是 `false`。
2. **等待线程** 还没有进入 `synchronized (this)` 块，也还没有执行 `incWaiters()`。此时它可能在操作系统的运行队列里被切走，也可能在 JIT 编译后的代码流水线里延迟了几个周期。
3. **完成线程** 调用 `setSuccess()` → `setValue0()`，CAS 成功写入 result。
4. **完成线程** 进入 `checkNotifyWaiters()`。因为此时等待线程还没有执行 `incWaiters()`，所以 `waiters == 0`，条件不成立，不调用 `notifyAll()`。
5. **完成线程** 从 `checkNotifyWaiters()` 返回，完成整个 `setValue0` 调用。通知的机会已经在此刻错过了。
6. **等待线程** 这时才被调度回来，进入 `synchronized (this)`（步骤 B），执行 `incWaiters()`（步骤 C）把 `waiters` 加 1，然后调用 `wait(timeout)`（步骤 D）。
7. 由于 `notifyAll()` 已经在步骤 4 被跳过，且 promise 只会被完成一次不会再次调用，等待线程只能睡到 timeout 结束。醒来后 (F) 检查 `isDone()` 返回 true，但比最优路径多等了整个超时时间。

这个 bug 的表现形式是异常地慢——本该在微秒级别返回的操作，却吃满了整个超时（可能是数百毫秒甚至秒级）。在生产环境中，这会导致使用 Netty 的服务端或客户端出现偶发的请求延迟尖刺，而且在监控上很难和真实的网络延迟区分开。

### 为什么这个窗口这么窄却仍然能命中

从 (A) `isDone()` 返回 false 到 (C) `incWaiters()` 执行完毕之间，在源代码层面只有进入 `synchronized (this)` 这一道操作。但在实际的 JVM 执行中，这中间涉及 monitor 获取——当有竞争时，monitor 获取可能是重量级的，需要经过 OS 的 pthread mutex 配合、线程排队、上下文切换等，耗时可达微秒甚至毫秒级。而 monitor 获取之前的锁外检查正是为了避开这条慢路径而做的优化。这个优化把一次关键的 `isDone()` 检查放在了锁外，使得在锁外检查通过到锁内正式登记之间，完成线程的 `notifyAll()` 被无声地跳过了。

## 6. 确定性测试如何把握这个瞬间

上游的 `testSignalRace` 通过创建大量 promise/线程对反复尝试来制造竞争，能否命中取决于 OS 的线程调度，是一个概率性测试。在本机的 JVM 和 OS 调度行为下它没有命中。为了让这个 bug 能够每次稳定复现，我在测试中覆写了 `isDone()`，用两个 `CountDownLatch` 来精确控制时序：

```java
final CountDownLatch firstIsDone = new CountDownLatch(1);    // 等待线程已进入第一次 isDone()
final CountDownLatch successSet = new CountDownLatch(1);     // 完成线程已 setSuccess
final AtomicInteger isDoneCalls = new AtomicInteger();       // 计数 isDone 被调用次数
final DefaultPromise<Void> promise = new DefaultPromise<Void>(executor) {
    @Override
    public boolean isDone() {
        boolean done = super.isDone();                       // 调用原始 isDone 获取真实结果
        if (isDoneCalls.getAndIncrement() == 0) {            // 仅第一次调用时执行拦截
            firstIsDone.countDown();                         // 通知：等待线程已到达 (A) 位置
            assertTrue(successSet.await(5, TimeUnit.SECONDS));
                                                             // 在此阻塞，等待完成线程先执行
        }
        return done;
    }
};
```

时序控制逻辑：

1. 覆写的 `isDone()` 在第一次被调用时（`isDoneCalls == 0`），先 `countDown` 释放 `firstIsDone` 信号，通知启动线程「等待线程已经走到了锁外 `isDone()` 检查这一步」。
2. 然后 `isDone()` 通过 `successSet.await()` 阻塞自己，等待启动线程那边的完成线程先把 `setSuccess()` 执行完。这样保证了等待线程在锁外检查 `isDone()` 返回 false 之后、进入 `synchronized` 之前，被强行暂停。
3. 启动线程在等待线程被暂停后，启动完成线程调用 `setSuccess(null)`。因为等待线程还没进入 `synchronized`、`waiters` 还是 0，`checkNotifyWaiters()` 不会调用 `notifyAll()`。
4. 完成线程设置完结果后，启动线程通过 `successSet.countDown()` 释放 `isDone()` 中的阻塞，让等待线程继续进入 `synchronized` → `incWaiters()` → `wait()`。
5. 等待线程进入 `wait(timeout)`，因为通知已经错过，只能睡满超时。

buggy 版结果来自 `reports/netty/netty-buggy-deterministic.log`：

```text
Expected: a value less than <500000000L>
     but: <1000583490L> was greater than <500000000L>
```

1000583490 纳秒 ≈ 1 秒，而阈值是 500000000 纳秒 = 0.5 秒。等待线程睡了大约 1 秒才返回，而不是在 promise 完成后立即返回，证明通知确实被错过了。

## 7. 修复方式：在锁内再检查一次条件

fixed 版本的修复只改了 `await0` 中一个地方——在 `synchronized (this)` 块内部、`incWaiters()` 和 `wait()` 之前，补了一次 `isDone()` 检查：

```java
for (;;) {
    synchronized (this) {
        if (isDone()) {           // 关键修复：锁内再检查一次条件
            return true;          // 条件已满足，不进入 wait，直接返回
        }
        incWaiters();             // 确认条件不满足后才登记为等待者
        try {
            wait(waitTime / 1000000, (int) (waitTime % 1000000));
        } finally {
            decWaiters();
        }
    }
    ...
}
```

对比 buggy 版的 `synchronized` 块内容，fixed 版新增了开头的 `if (isDone()) { return true; }`。这个不到一行的改动背后的原理，是条件变量模式的核心约束。

条件变量正确的使用范式（也是课程中 P/V 操作的思想延伸）是：

```text
synchronized (lock) {
    while (!condition) {    // 在锁内检查条件
        lock.wait();        // 条件不满足才等待
    }
}
```

即条件检查和 `wait()` 必须在同一个 `synchronized` 块内完成。buggy 版的形式等价于：

```text
if (condition) return;      // 锁外检查（快速路径）
synchronized (lock) {
    lock.wait();            // 进入了 wait 但没在锁内检查条件
}
```

锁外的那次快速检查本身没有错——它是一种合理的优化，让已经完成的 promise 可以零锁开销返回。但它的存在不意味着锁内的检查可以省略。fixed 版保留了锁外检查作为快速路径，同时在锁内加入再次检查，形成了双重检查的完整模式。

修复后的完整时序分析：

- 如果完成线程在等待线程进入 `synchronized (this)` 之前就完成了 promise，等待线程进入 `synchronized (this)` 后会执行锁内 `isDone()` 检查，发现结果已经为 true，直接返回 true，不会走到 `incWaiters()` 和 `wait()`。通知是否发生已经不重要了，因为条件本身就是「promise 是否完成」。
- 如果完成线程在等待线程进入 `synchronized (this)` 之后才完成 promise，CAS 的顺序一定晚于 `incWaiters()`，所以 `checkNotifyWaiters()` 会看到 `waiters > 0`，执行 `notifyAll()`，等待线程被正常唤醒。

无论哪种时序，等待线程都能在 promise 完成后尽快返回，不需要睡满超时。

## 8. 小结

Netty 这个 bug 展示的是条件变量丢唤醒在真实工业代码中的表现：`isDone()` 检查和 `wait()` 进入不在同一个 monitor 的保护下，导致完成线程的 `notifyAll()` 在等待线程登记 `waiters` 之前到达，被无声地跳过。修复加的是一行 `if (isDone()) return true;`——放在 `synchronized (this)` 内部、`incWaiters()` 之前——使得等待线程在真正 `wait()` 之前，得到最后一次在锁保护下确认条件的机会。这行代码闭合了从锁外 `isDone()` 检查到锁内 `incWaiters()` 之间的竞态窗口。

从并发理论的角度来看，条件变量的谓词检查和 `wait()` 必须在同一把锁的保护下构成原子操作——这同样是 POSIX `pthread_cond_wait` 和 Java `Object.wait()` 的共同设计要求。这个 bug 的根源，就是在一次为了性能而做的锁外快速路径优化中，这个原子性被打破了。修复通过补回锁内检查恢复了「检查-等待」的原子性，而保留了锁外检查作为快速路径，兼顾了正确性和性能。
