# voxel_game::streaming — CPU 异步区块流送基础设施

本目录是 C++17、header-only、零新第三方依赖的 CPU 管线。它不引用场景、GPU、
渲染器或 VM；World 令牌维护、主循环调度与实际 GPU 上传由上层接线。

## 输入所有权与状态有效性

`SectionSnapshot` 没有公有构造器、`token/blocks/faces` 也没有公有可写成员。
生产者只可用 `SectionSnapshotBuilder(token)` 填充已检查的 block/face 单元，再以
`std::move(builder).freeze()` 独占转移为 `shared_ptr<const SectionSnapshot>`。普通
调用路径不能保留 `shared_ptr<SectionSnapshot>` 的可写别名；worker 只会看到 const
快照。坐标、`FaceIndex` 与每一个 `BlockState` 都检查，未知枚举值严格拒绝。
未检查的线性索引仅位于 `streaming::detail`，并只由已检查实现调用。

`ChunkToken` 的 generation/revision 使用 64 位 `TokenRevision`；它没有 `++`，只能
`try_bump()`。达到 `UINT64_MAX` 时返回 false 且保持原值，调用方必须把该 section
标为不可再提交或切换 world epoch，不能静默回绕造成 ABA。

## mailbox、线程与 stop cutoff

- owner 线程专有：`try_submit`、`try_pop_result`、`stop`；其他线程返回
  `WrongThread` 且不改变状态。`stats().snapshot()` 与 `state()` 可跨线程读取。
- `try_submit`/`try_pop_result` 只保证**不等待 mailbox mutex**：锁忙返回 `Busy`。
  参数、局部对象或被替换的 callable/snapshot 的析构由调用线程承担，不能宣传为
  任意析构都严格非阻塞。
- 任务 mailbox 的 pop 会在取得锁前移出旧 `out`，因此旧 callable/snapshot 的最后
  一个引用在解锁后销毁；`discard_all` 也在锁外销毁。
- `stop()` 是同步屏障。它先封闭 result mailbox，再把 `Stopping` 置为可观察状态，
  所以观察到 `Stopping` 即代表 result cutoff 已完成。cutoff 前已经发布的结果仍可
  drain；cutoff 后发布一律 `dropped_closed`。随后未领取任务取消、已领取未 admission
  的任务取消、worker join，外层返回后才置 `Stopped`。
- stop 的 owner-thread 析构重入返回 `StopStatus::Reentrant`，不会 join、不会写
  `Stopped`；仅最外层 stop 屏障完成后返回 `Joined`。析构函数执行同一关停与 join，
  但没有状态码。

任务被 mailbox 真正 `Accepted` 后才消耗提交序号；`Busy`、`Full`、`Closed` 都不烧号。
`Config` 仅接受 worker_count `-1`（默认）、`0`、`1..32`；其他值抛
`std::invalid_argument`。两种 mailbox 容量都必须非零，零同样抛异常，绝不静默改为 1。

worker 入口把 task 构造、pop、admission、执行、发布和 loop-tail 都置于异常边界；
已领取任务在基础设施异常时恰好结算一次 pending，线程继续处理后续任务。作业函数
异常以 `MeshResult{ok=false}` 发布；错误文字仅尽力复制。`std::function` 的析构本身
按标准为 noexcept，若用户析构函数违反该规则触发 terminate，任何线程边界都无法恢复。

## pending、stats 与接纳

`pending` 在任务可见前预留，只有 Accepted 保留；每个已接受任务由 owner pop、取消、
cutoff drop 或基础设施失败恰好结算一次，因而不为负。`StreamingStatsSnapshot` 是用
relaxed 原子逐字段读取的**近似瞬时快照**，字段间不是同一线性化时刻，不能把它当成
事务性总账。

`UploadFrameQueue` 是跨帧接纳的持有位置：`enqueue()` 接收已 pop 的结果，
`try_take()` 在 `FrameBudgetExhausted` 时保留队首对象，`begin_frame()` 精确清零本帧
uploads/bytes，再次 `try_take()` 才能接纳。它对 `StaleToken`、`FailedResult`、
`MeshTooLarge`、`SizeOverflow` 明确丢弃；对 Accepted 精确累计上传数和字节数，避免
调用方“pop 后一句下帧重试”却丢失对象。

## 接线顺序

1. World 维护每个 section 的权威 token；加载、块编辑、邻接面变化分别调用对应
   revision 的 `try_bump()`，失败即 fail closed。
2. 主线程以 builder 构造并冻结快照，调用 `try_submit`；`Busy/Full` 可保留 builder
   输入后重试，`Invalid` 是调用错误，`Closed` 停止提交。
3. 主线程把 `try_pop_result` 的结果移动入 `UploadFrameQueue`，按 disposition 做一次
   业务结算，再把 Accepted 的结果交给 GPU。不要对同一个 disposition 重复计数。
4. 销毁 World/GPU 前先调用 `stop()` 并要求 `Joined`；之后才释放被 callable 捕获的
   上层对象。

## 测试与测试接缝

根 CMake 会建立 `voxel_streaming_tests` 并注册到 CTest，设置 20 秒进程级 `TIMEOUT`。
测试只使用 CPU 与 C++17。`Config.task_pop_hook`、`result_cutoff_hook`、
`fault_injector`、`thread_factory`、`initial_sequence` 是确定性测试/重放接缝；生产
接线应保持为空。测试覆盖 frozen 所有权、非法 state、最后引用锁外析构、重入 stop、
cutoff 前 drain/cutoff 后 drop、满 result waiter 唤醒、Busy/Full 不烧序号、异常结算、
revision 饱和与跨帧 deferred 保留。
