# 体素游戏冲刺会话报告（2026-08-29 ~ 08-30，ZCode 会话）——修订版

> 本文件为**历史交付记录 + 截至 voxel c09924e 的权威状态**（按统筹 2026-08-30 修正意见修订；此前版本中的"17 项全绿/crash-safe/两行接线/零测试"等表述不准确，已在本版逐条修正并保留修正对照）。

## 一、权威状态快照

- **voxel 子仓库**：origin/main = `c09924e6fe08b56687c1c58d96dcbd53b30571af`（Harden NPC sight at coordinate boundaries），远端干净克隆 MSVC `/W4 /permissive- /WX` 构建通过，CTest **17/17**。Light、Frustum、Inventory/Crafting、DayNight、GreedyMesher、RegionStore、NPC CPU 行为内核及 LOS/极端坐标加固均已提交。
- **"17 项全绿"的当前口径**：NPC 批次已由协调者独立复审并在远端干净克隆复证，现为权威结果；该批次尚未接入 main、facade 或 VES。
- **vessel 子仓库**：HEAD `86b05c0`，父仓 gitlink 仍为 **`0f59584`**。`src/vm.c` 含未提交的 foreign 实例方法修复，**未完成权威验证、未推送**（详见第四节）。
- **未提交（待协调者处置）**：ves/（GameClock 绑定层）及其测试/生成物；NPC 的 main/facade/VES 运行时接线仍待后续门禁。

## 二、历史交付记录（含现状修正）

### 1. 光照引擎 `light/LightEngine.h` — 已提交（aa94bca）
- 交付时：双通道 BFS、天光 15 纵列移除特例、不透明发光体虚拟移除（on_block_changed 需传 old_state）、跨 section、随机参考实现交叉验证。
- **现状修正**：`relight_sections()` 已改为**安全回退**——无已加载目标返回空，否则 `relight_all()`；one-ring 重算及"15 个测试函数"均为交付时口径，当前测试集已由后续返修重构，以子仓库为准。

### 2. 视锥剔除 `render/Frustum.h` — 已提交（4097dec + 393be7d）
- 数值加固（归一化系数 isfinite、畸形 AABB 保守放行）由并行会话补齐；协调者已验收，origin/main 已更新，**已冻结**。

### 3. 权威流送集成审计 — 结论有效
- 无阻塞发现；轻微项（int↔int32 收窄缺 static_assert、snapshot_for 每次冻结 4096+1536 格）记录在案。

### 4. 区域存档 `persistence/RegionStore.h` — 已提交（9aefa74），**协调者终审通过**
- **最终状态（2026-08-30 第五轮）**：第四轮的 `ReplaceFileW` 方案因官方规定的 1176/1177 部分失败状态被终审阻塞；无 backup 时旧 target 可能已消失，而原 guard 又会删除仍在 staged 名下的新快照，存在数据丢失路径。最终实现改为同一 linked-module-local SRWLOCK：`load_region()` 从 native open 到 handle close 持 shared lock，`save_region()` 仅在最终 `MoveFileExW(MOVEFILE_REPLACE_EXISTING)` 发布时持 exclusive lock；encode/stage/write/flush/close 全在独占锁外，零重试，外部模块/进程/句柄冲突如实返回 `WriteError`。Windows loader 仅分享 read+delete、拒绝原地写入。确定性锁范围用例、4 writers × 25 次零重试生产 reader 门禁、RegionStore 12/12 均通过；远端干净克隆 CTest 16/16。`rename_durable` 默认 false（Windows 成功=false、所有失败=false、POSIX 仅 rename+父目录 fsync 成功=true）。POSIX 分支仍未机器运行（本机无环境）。
- **第四轮结论（历史记录）**：SRWLOCK 只包发布仍无法让 `MoveFileExW` 与本模块持有的 reader handle 共存；临时改用 `ReplaceFileW` 后本地 12/12、17/17，但终审发现上述官方文档化的数据丢失路径，因此第四轮未验收、未提交。
- **第三轮结论（历史记录）**：取消 WriteError 重试后发布冲突确定性复现（阶段=replace，GLE=5，4 写者 ~85-90/100，仅写者 ~28/100）；第三轮交付时 region_store_tests 在 Windows 红、其余 16 项绿。
- **交付时缺陷（统筹指正，属实）**：并发临时文件共用名字、ANSI 路径破坏 Unicode、fflush/fclose 不保证落盘、读取无上界——**不能称为 crash-safe**。原报告的"崩溃安全"表述撤回。
- **返修内容**（本会话核验时已在文件中）：目标目录内唯一临时名（pid+原子计数+随机 nonce，`_O_EXCL` 独占创建+重试）、全程宽字符 API、`fflush`+`FlushFileBuffers`/`fsync` 后才发布、探测尺寸先于读取按 `max_bytes` 拒绝、新增 `ProbeError` 区分"确定不存在/不可判定"。
- 最终口径：本链接模块内的 load/save 由 shared/exclusive SRWLOCK 协调；跨模块、跨进程或不兼容外部句柄不在该锁的覆盖内，发布失败返回 `WriteError`，不自动重试。发布前数据 flush 到设备；Windows 不宣称目录项持久化，POSIX 只有父目录 fsync 成功才置 `rename_durable=true`。

### 5. 贪婪网格化 `voxel/GreedyMesher.h` — 已提交（d26e2c7）
- 覆盖等价、绕序法线、tile 单位 UV 契约、光照烘焙变体（合并键=方块+光照）。

### 6. 物品栏 + 合成 — 已提交（83c3ce9，含后续会话六项终审修复）

### 7. NPC AI `ai/` + 昼夜 `runtime/DayNightCycle.h` — DayNight **已提交**（b066754，含溢出安全返修）；NPC CPU 行为内核及 LOS/极端坐标加固 **已提交**（c09924e，尚未接入运行时）

### 8. VES 绑定层 `ves/`（GameClock）— 未提交，**接线未完成**（见第五节）

## 三、修正对照表（统筹指正 → 原报告措辞）

| 指正 | 原报告措辞 | 修正 |
|---|---|---|
| 权威 = c09924e，干净克隆 17/17 | "ctest 17/17 全绿" | 该数字此前仅为含 NPC 草稿的脏树口径；NPC 提交及边界加固后已由远端干净克隆复证 |
| relight_sections 已改安全回退 | "单环重算（含正确性论证）" | 交付时口径，现状以子仓库为准 |
| RegionStore 有四项阻塞 | "崩溃安全/crash-safe" | 撤回；返修历史+现契约如实记录 |
| vessel 修复未提交未权威验证，gitlink=0f59584 | "修复完整落盘收官" | 落盘≠验收；状态见第四节 |
| vessel 已有 ~30 个 Catch2 测试文件 | "仓库对 VM 零测试" | 撤回；缺的是 foreign 覆盖+CTest 注册 |
| geometry 有实例方法实际消费者 | "生产代码没有消费者" | 撤回；正确表述：损坏路径未被**现有自动化测试**覆盖 |
| GameClock 接线非两行 | "协调者两行注册" | 撤回；完整清单见第五节 |

## 四、vessel 修复状态（准确版）

- `src/vm.c` 未提交 diff（+39/-3，含所有者后续改动）：① `invoke_from_class` METHOD_FOREIGN 分支 api_stack 基准补接收者槽；② 同分支补 `ret = true`；③ `call_value` OBJ_CLASS 对 foreign 类改走元类合成构造器（OP_FOREIGN_CONSTRUCT）。
- 本会话验证（非权威）：实例风格驱动 12/12；**引擎自身 maths 模块**（Float2 多 init→this.set_x 等实例调用）6 检查 12/12；静态+句柄 GameClock 16/16 不受影响。全套 ves_test 回归被并行会话在制的 cax Replayer.cpp 阻塞，**未跑**。
- **未完成**：所有者冻结、审查、提交、推送、父仓 gitlink 更新（现为 0f59584）。
- **既有测试事实**：vessel test/ 已有 ~30 个 Catch2 文件（assignment/block/bool/class/closure/...）；真正缺口 = **foreign callback / 实例方法的自动化覆盖与 CTest 注册**。

## 五、GameClock VES 绑定完整接线清单（替代"两行"说法）

1. `src/editor/ves_host.cpp`：include gameclock.ves.inc + MODULE_REGISTRY 条目——**该文件当前有其他会话改动，暂不能并发进入**。
2. 构建：`wrap_GameClock.cpp` 需加入 editor 与 ves_test 目标的编译与链接（依赖 vessel + voxel_headers）。
3. 测试注册：驱动入 test/ves + CTest 接线。
4. **未过审查项**：参数转换（ves_tonumber 的取整/负值/NaN 语义）与句柄校验（ves_toforeign 对非 GameClock 值的行为）——加固排在 vessel 修复合入之后。

## 六、后续顺序（按统筹调整后）

1. ✅ RegionStore 返修（9aefa74；远端干净克隆 16/16，见第二节 4）。
2. ✅ 本报告修订（当前文件）。
3. ✅ NPC CPU 行为内核、LOS 与极端坐标回归（c09924e；远端干净克隆 17/17，尚未接入 main/facade/VES）。
4. **[等待门禁]** vessel vm.c 所有者冻结后，为**现有 Catch2 套件**补 foreign 实例方法回归（不碰 vm.c）。最少覆盖：无参/带参 receiver 槽位、返回值与栈平衡、构造/静态调用负向回归、Release 配置、`ves_free_vm()`。
5. **[等待门禁]** vessel 修复提交、推送、父仓 gitlink 更新后：加固 GameClock（参数转换+句柄校验+完整接线）；之后才扩展 Inventory/Crafting/NPC 绑定。
6. **冻结**：Cutout/Translucent、音频、biome、M0 指针锁定——不扩范围、不进共享生产渲染区。
