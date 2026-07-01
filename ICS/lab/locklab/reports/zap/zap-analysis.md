# zap：BufferedWriteSyncer 里的 Stop / flush 死锁

## 1. 复现信息

- 修复标题：Fix deadlock when Stop and flush race (#1430)
- 回归测试：`TestBufferWriter/stop_race_with_flush`

日志文件：

- buggy：`reports/zap/zap-buggy.log`
- fixed：`reports/zap/zap-fixed.log`
- 简要状态：`reports/zap/zap-summary.txt`

复现结果：

```text
zap_buggy_status=1
zap_fixed_status=0
```

buggy 版在 3 秒测试超时后打印了 goroutine 栈。关键现场很清楚：一个 goroutine 在 `Stop()` 中等待 `<-s.done`，另一个 goroutine 在 `flushLoop()` 中调用 `Sync()`，卡在 `s.mu.Lock()`。

## 2. Bug 位置

zap 的 `BufferedWriteSyncer` 是一个带后台刷盘协程的缓冲写入器，它把日志先写进 `bufio.Writer`，再由后台 `flushLoop` 定时调用 `Sync()` 刷出去。整个写入器内部状态由一把互斥锁 `s.mu` 保护，用来保证对共享变量的互斥访问：

```go
mu          sync.Mutex    // 互斥锁，保护下方所有内部状态
initialized bool          // 写入器是否已完成初始化
stopped     bool          // 写入器是否已进入停止流程
writer      *bufio.Writer // 缓冲写入器，日志先攒在内存缓冲区
ticker      *time.Ticker  // 定时器，周期性触发 flushLoop 刷盘
stop        chan struct{} // 关闭此 channel 来通知 flushLoop 退出
done        chan struct{} // flushLoop 退出后关闭此 channel 表示已安全退出
```

其中 `stop` 和 `done` 两个 channel 形成了一套协程间的握手协议：主协程通过关闭 `stop` 来通知 `flushLoop` 退出，`flushLoop` 退出时通过 `defer close(s.done)` 来告知主协程自己已经安全退出。这个握手机制本身是合理的，但它和 `s.mu` 之间的交互出了问题。

### Sync：刷盘前先拿锁

```go
func (s *BufferedWriteSyncer) Sync() error {
    s.mu.Lock()          // P 操作：获取互斥锁
    defer s.mu.Unlock()  // V 操作：函数退出时释放互斥锁

    var err error
    if s.initialized {
        err = s.writer.Flush()  // 将 bufio.Writer 缓冲区内数据刷到下游
    }

    return multierr.Append(err, s.WS.Sync()) // 合并可能的错误，同时刷下游
}
```

`Sync()` 的逻辑很直接：拿锁，如果已经初始化就把 `bufio.Writer` 缓冲区里的数据刷到下游 `WriteSyncer`，然后释放锁。值得注意的细节是，`s.mu` 在整个 flush 和下游 `WS.Sync()` 期间都被持有——如果写入下游涉及系统调用或网络 I/O，持锁时间可能并不短。但这不是本次 bug 的直接原因，关键在于这把锁被 `Stop()` 以一种更危险的时序持有了。

### flushLoop：ticker 到点后调用 Sync

```go
func (s *BufferedWriteSyncer) flushLoop() {
    defer close(s.done)  // 无论以何种方式退出循环，最后一定关闭 done channel

    for {
        select {
        case <-s.ticker.C:  // ticker 定时到期，触发一次周期性刷盘
            _ = s.Sync()    // Sync 内部第一行就是 s.mu.Lock()
        case <-s.stop:      // stop channel 被关闭，收到退出信号
            return
        }
    }
}
```

`flushLoop` 是后台 goroutine 的主循环，它在两个 channel 上做 `select`：要么 ticker 到期触发一次 `Sync()` 刷盘，要么 `stop` channel 被关闭通知它退出。函数开头有 `defer close(s.done)`，保证无论从哪个分支退出循环，都会关闭 `done` channel 来通知等待者。这里的关键点是 `case <-s.ticker.C` 分支里调用了 `s.Sync()`，而 `Sync()` 内部第一行就是 `s.mu.Lock()`。

### buggy 版 Stop：持锁等待后台协程退出

```go
func (s *BufferedWriteSyncer) Stop() (err error) {
    var stopped bool              // 记录是否本次调用真正执行了 stop

    func() {                      // 匿名函数限定临界区范围
        s.mu.Lock()               // P 操作：进入临界区
        defer s.mu.Unlock()       // 确保无论以何种路径离开都释放锁

        if !s.initialized {       // 未初始化则无需停止
            return
        }

        stopped = s.stopped       // 读取当前是否已停止
        if stopped {              // 已停止过，防止重复 stop
            return
        }
        s.stopped = true          // 标记为已停止

        s.ticker.Stop()           // 停掉定时器，阻止新的 tick 事件
        close(s.stop)             // 关闭 stop channel，通知 flushLoop 退出
        <-s.done                  // 阻塞等待 flushLoop 真正退出并关闭 done
    }()

    if !stopped {                 // 如果是本次才停止的，做最终刷盘
        err = s.Sync()            // 注意：此时已经释放了 s.mu！
    }

    return err
}
```

`Stop()` 用一个匿名函数来限定临界区的范围——从 `s.mu.Lock()` 到 `defer s.mu.Unlock()`，这个匿名函数内的所有操作都在持锁状态下进行。逐行看这个临界区做了些什么：

1. 先检查 `s.initialized`，如果根本没初始化就直接返回，临界区内什么也不做。
2. 检查 `s.stopped`，如果已经 stop 过就跳过后续逻辑，这是防止重复 stop 的防护。
3. 设置 `s.stopped = true`，标记写入器已进入停止流程。
4. `s.ticker.Stop()` 停掉定时器，防止 ticker 继续触发新的 `Sync()` 调用。
5. `close(s.stop)` 关闭 `stop` channel，这会让 `flushLoop` 的 `select` 语句从 `case <-s.stop` 分支返回。
6. `<-s.done` 阻塞等待 `flushLoop` 真正退出——只有当 `flushLoop` 的 `defer close(s.done)` 执行后，这里才能继续。

问题的核心在第 6 步：`<-s.done` 在 `s.mu` 仍然被持有的情况下执行。而 `flushLoop` 在收到 `stop` 信号并退出之前，可能正处于 `case <-s.ticker.C` 分支中正在执行 `Sync()`，而 `Sync()` 需要获取 `s.mu`。

## 3. 死锁的交错过程

触发这个 bug 需要 ticker 信号和 `Stop()` 调用恰好交错——具体来说，`Stop()` 在 `flushLoop` 已经从 `s.ticker.C` 读到信号、但还没有从 `Sync()` 返回的这段时间窗口内拿到了 `s.mu`。一旦这个交错发生：

1. 用户线程调用 `Stop()`，进入匿名函数的临界区，成功拿到 `s.mu`。
2. `Stop()` 按顺序执行：检查 `initialized` 和 `stopped` → 设置 `s.stopped = true` → 调用 `s.ticker.Stop()` 停掉定时器 → 调用 `close(s.stop)` 通知 `flushLoop` 退出。
3. `Stop()` 继续在临界区内执行 `<-s.done`，开始阻塞等待 `flushLoop` 退出。
4. 但是，在 `Stop()` 拿到 `s.mu` 之前的一瞬间，`flushLoop` 已经从 `s.ticker.C` 读到了一个 tick 信号，进入了 `case <-s.ticker.C` 分支，开始调用 `s.Sync()`。
5. `Sync()` 的第一条语句就是 `s.mu.Lock()`，但此时 `s.mu` 被 `Stop()` 持有，所以 `flushLoop` 卡在获取锁这一步。
6. 此时形成互等：`Stop()` 在等 `flushLoop` 关闭 `s.done` 来解除 `<-s.done` 的阻塞；`flushLoop` 在等 `Stop()` 释放 `s.mu` 来完成 `Sync()` 并返回循环、收到 stop 信号、执行 `defer close(s.done)`。

用进度图来构想：两个 goroutine 各自进入了自己的临界区，但它们的临界区并不像互斥锁设计的那样互不重叠。`Stop()` 的临界区跨越了 `Lock` 到 `Unlock`，其中包含了 `<-s.done` 这个阻塞操作；`Sync()` 要进入临界区必须先过 `Lock()`。当两个 goroutine 同时处于 `Stop` 的 `<-s.done` 和 `Sync` 的 `Lock()` 时，它们就落入了不安全区域（unsafe region），两条轨迹都无法前进。

从 Coffman 条件来看这个 bug：

- **互斥**：`s.mu` 是互斥锁，同一时刻只能被一个 goroutine 持有。✓ 满足。
- **持有并等待**：`Stop()` 持有 `s.mu` 的同时在等 `<-s.done`。✓ 满足。
- **不可剥夺**：Go 的 `sync.Mutex` 不能被外部强制释放，只能由持有者主动 `Unlock`。✓ 满足。
- **循环等待**：`Stop()` 等 `flushLoop` 关闭 `done`，`flushLoop` 等 `Stop()` 释放 `s.mu`。形成 `Stop → done → flushLoop → mu → Stop` 的闭环。✓ 满足。

四个条件同时满足，死锁在逻辑上就是必然的。实践中是否触发取决于两个 goroutine 的调度时序——如果 ticker 在 `Stop()` 的 `s.ticker.Stop()` 之后才触发，`flushLoop` 就走 `case <-s.stop` 正常退出。这就是为什么这个 bug 是偶发的。

### 为什么 Go 的 race detector 没有直接报错

`sync.Mutex` 的 Lock/Unlock 操作和 channel 的 send/receive/close 都是 Go 运行时内部良好同步的操作。但这个 bug 涉及的是两个不同 goroutine 之间的一对相反方向的依赖关系：channel 接收等锁释放，锁释放等 channel 关闭。这是应用层的逻辑死锁，go test -race 无法检测到它。这也是 lockdep 那种运行时锁依赖图判环机制在有向图层面存在价值的原因——如果有类似工具，这种 AB→BA 环可以被提前预警。

## 4. 复现日志里的证据

`reports/zap/zap-buggy.log` 中的关键内容：

```text
goroutine 66 [chan receive]:
go.uber.org/zap/zapcore.(*BufferedWriteSyncer).Stop.func1(...)
    buffered_write_syncer.go:210

goroutine 67 [sync.Mutex.Lock]:
go.uber.org/zap/zapcore.(*BufferedWriteSyncer).Sync(...)
    buffered_write_syncer.go:159
go.uber.org/zap/zapcore.(*BufferedWriteSyncer).flushLoop(...)
    buffered_write_syncer.go:181
```

两个 goroutine 的状态完全印证了上面的分析：goroutine 66 是 `Stop()` 所在的 goroutine，卡在 channel receive 操作 `<-s.done`；goroutine 67 是 `flushLoop`，卡在 `Sync()` 里的 `Mutex.Lock`。goroutine 67 的调用栈清晰地展示了路径：`flushLoop` → `Sync` → `Lock`。两者互为对方等待的资源。

## 5. 修复方式：把等待移出临界区

fixed 版本没有改变后台刷盘模型，也没有引入额外的锁，只做了一件事：把 `<-s.done` 从 `s.mu` 的保护范围里移出去。

```go
func (s *BufferedWriteSyncer) Stop() (err error) {
    stopped := func() bool {      // 匿名函数现在返回是否真正执行了 stop
        s.mu.Lock()               // P 操作
        defer s.mu.Unlock()       // V 操作，临界区在此结束

        if !s.initialized {
            return false
        }

        if s.stopped {
            return false
        }
        s.stopped = true

        s.ticker.Stop()           // 停定时器，必须在持锁状态下完成
        close(s.stop)             // 通知 flushLoop 退出
        return true               // 标志：本次确实执行了 stop
    }()                           // 临界区结束，s.mu 已释放

    if !stopped {                 // 如果之前已经 stop 过，直接返回
        return
    }

    <-s.done                      // 在锁外等待 flushLoop 退出
    return s.Sync()               // 在锁外做最终刷盘，Sync 内部自行拿锁
}
```

逐行对比修复前后的变化：

1. 匿名函数现在返回一个 `bool` 值，表示是否真正执行了 stop 操作。在 buggy 版里，这个信息是通过外层变量 `stopped` 传递的，匿名函数内和外层后续逻辑共享了这个变量——虽然在这个 bug 中这不是直接问题，但修复顺便让数据流更清晰。
2. 临界区现在只包含必须持锁的操作：检查状态、修改 `s.stopped`、停 ticker、关闭 `stop` channel。`<-s.done` 和最后的 `Sync()` 调用都被移到了锁外。
3. `<-s.done` 现在在释放 `s.mu` 之后执行。这意味着即使 `flushLoop` 此时正卡在 `Sync()` 里等 `s.mu`，一旦 `Stop()` 的临界区结束释放了锁，`flushLoop` 就能拿到 `s.mu`、完成 `Sync()`、返回循环，然后看到 `stop` channel 已经关闭（或尚未关闭但马上会通过下一次循环选中），从 `case <-s.stop` 退出，最后执行 `defer close(s.done)` 解除 `<-s.done` 的阻塞。
4. 最后的 `s.Sync()` 在 `<-s.done` 之后、不持任何锁的情况下调用，确保 `flushLoop` 已经完全退出后再做最终刷盘——此时没有任何竞争风险，因为 `flushLoop` 已经不存在。

从 Coffman 条件的角度看，修复打掉了「持有并等待」这个条件：`<-s.done` 被移出临界区，意味着 `s.mu` 的持有者不再在持锁的同时等待另一个资源。持有并等待条件被破坏后，即使其他三个条件仍然满足，死锁也不再成立。

## 6. 小结

这个 bug 的核心教训是临界区应当尽可能短，尤其不要在持有互斥锁时等待另一个 goroutine 或回调完成——你无法保证那个被等待的执行体不会需要同一把锁。修复的改动很小：只是把 `<-s.done` 从 `s.mu.Lock()` / `defer s.mu.Unlock()` 的范围内挪到了释放锁之后，但效果是把一条原本可能双向依赖的等锁链切成了两个互不重叠的阶段。从更宏观的视角看，这是一个锁顺序问题：buggy 版隐含的锁顺序要求在 `s.mu` 被持有时执行 `<-s.done`，而 `<-s.done` 的前提是 `flushLoop` 完成，`flushLoop` 完成的前提又是拿 `s.mu`——三者形成环。fixed 版把 `<-s.done` 移出锁范围，打掉了从 `done` 回到 `mu` 的依赖边，锁依赖图重新变成无环。
