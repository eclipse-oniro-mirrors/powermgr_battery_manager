# 编码铁律

以下规则提炼自本仓现有实现、AGENTS.md 约束边界与 OpenHarmony 系统部件通用要求，适用于本仓全部生产代码（`services/`、`frameworks/`、`charger/`、`utils/`、`interfaces/`）。规则对应的架构背景见 [架构设计及约束](architecture.md)，涉及实体的语义见 [业务实体知识](battery-Entity.md)。

## 并发模型

- **禁止**在 HDI 回调链路（`BatteryCallback::Update` → `HandleBatteryCallbackEvent` → `PublishEvents`）中执行阻塞 IO、同步等待或长耗时计算，**采用**纯内存状态更新 + FFRT 异步任务（参考 `battery_service.cpp` 的 `FFRTUtils::SubmitDelayTask`、`g_queue` 队列）。依据：AGENTS.md 禁止事项"HDI 回调阻塞会丢失事件"；实现中仅延迟关机/休眠被投递 FFRT。
- **禁止**在未持锁情况下读写 `BatteryService` 的 `batteryInfo_`、`iBatteryInterface_`，**采用**`std::shared_lock<std::shared_mutex>`（读）/`std::lock_guard<std::shared_mutex>`（写）包裹（见 `battery_service.cpp` 各 `Get*Inner`、`RegisterBatteryHdiCallback`）。`BatteryNotify` 的 once 去重状态与 `BatteryConfig` 单例同理使用各自 mutex。
- **禁止**在 `powermgr` 进程内做长耗时同步操作（同进程还有 SA 3301/3303/3308），**采用**FFRT 任务、延迟定时器（`FFRTTimer`）或异步 `dlopen` 按需加载。
- **禁止**无超时的永久等待，**采用**带超时资源（如 `RunningLock::Lock(SHUTDOWN_GUARD_TIMEOUT_MS)`，90s）避免泄漏。

## 错误处理

- **禁止**吞掉 IPC/HDI 失败继续返回"正常值"，**采用**既定哨兵值与错误码语义：
  - 客户端查询失败返回 `INVALID_BATT_INT_VALUE(-1)`、`INVALID_BATT_TEMP_VALUE(100)`、`*_BUTT` 枚举、`INVALID_STRING_VALUE("Invalid")`（`battery_info.h:23-28`，`battery_srv_client.cpp` 各方法）。
  - 业务错误统一用 `BatteryError`（`interfaces/inner_api/native/include/battery_srv_errors.h`）：`ERR_FAILURE(1)`、`ERR_PERMISSION_DENIED(201)`、`ERR_SYSTEM_API_DENIED(202)`、`ERR_PARAM_INVALID(401)`、`ERR_CONNECTION_FAIL(5100101)`、`ERR_HANDLE_NOTIFICATION_FAIL(5100102)`。
  - HDI 接口指针判空是每个 `Get*Inner` 的第一道分支（`iBatteryInterface_ == nullptr` 时返回缓存值或哨兵）。
- **禁止**用异常/崩溃表达可预期失败（服务端代码无 C++ 异常），**采用**返回码 + `BATTERY_HILOGE/W` 日志留痕。
- **禁止**删除日志、HiSysEvent、XCollie 看护以"通过测试"（AGENTS.md 禁止事项）；每个 IPC 入口必须保留 `BatteryXCollie` 构造。

## API 与 ABI 稳定性

- **禁止**复用或调整 `services/zidl/IBatterySrv.idl` 中已有 `[ipccode N]` 码值，**采用**新接口追加新码值（当前 18 个方法；AGENTS.md 表述为 17，以 IDL 实际内容为准）。
- **禁止**在 `BatteryInfo` 中间插入字段或调整既有枚举值顺序，**采用**末尾追加（跨进程/NDK 布局 ABI；`BatteryPluggedType`、`BatteryChargeState`、`BatteryHealthState`、`BatteryCapacityLevel`、`ChargeType` 消费者众多）。
- **禁止**直接修改生成产物：`frameworks/ets/taihe/`（改 `.taihe` 源后重生成）、IDL 生成的 `IBatterySrv_proxy/stub`（改 `.idl` 后重生成）、`tools/ohos-batteryManager` 需独立发版。
- **禁止**绕过 `*.map` 版本脚本（`frameworks/capi/libohbattery_info.map`、`interfaces/inner_api/libbatterysrv_client.map`、`utils/hookmgr/libbattery_hookmgr.map`、`services/libbattery_notification.map` 等）新增导出符号或降级既有符号可见性。
- 公共 API 变更必须同步四套绑定（C/NAPI/CJ/taihe）+ `*.map`，并评估 NDK ABI（AGENTS.md"是否需要同步修改其他层"检查项）。

## 权限与安全

- **禁止**依赖客户端做权限校验，**采用**服务端入口统一校验：
  - `SetBatteryConfigInner`：`Permission::IsSystem()` + 原生权限 `ohos.permission.POWER_OPTIMIZATION`（`battery_service.cpp:720-725`）。
  - `GetBatteryConfigInner`/`IsBatteryConfigSupportedInner`/`GetTotalEnergyInner`/`GetRemainEnergyInner`/`GetRemainingChargeTimeInner`：`Permission::IsSystem()`。
  - `HandleNotificationInner`：`IsSystem()` + `ohos.permission.POWER_MANAGER`。
  - `Dump`：`isBootCompleted_` + `IsSystem()`，mock 类命令仅在非用户版本（`#ifndef BATTERY_USER_VERSION`）编译。
- **禁止**在 uevent/配置解析中直接信任外部字符串做危险操作；uevent 动作仅匹配白名单动作词（shutdown/reboot/sendcommonevent/sendcustomevent/sendpopup，`battery_notify.cpp:132-165`），未知动作记日志拒绝。
- `dlopen` 第三方/伴随库必须 `RTLD_NODELETE` 或确保进程常驻依赖（`battery_notify.cpp:354-369` 对 libphonenumber、libmedia_client 的崩溃教训注释）。

## 配置与特性开关

- **禁止**只改 `batterymgr.gni` 的 `declare_args()` 默认值而漏改 `defines` 宏或 `bundle.json` 的 `features` 列表（"双写"陷阱，AGENTS.md 项目特定陷阱；当前 gni 有 7 个开关，bundle.json features 列出 6 个，`battery_manager_feature_support_notification_string` 未列入，属现状差异）。
- **禁止**在代码中硬编码 SOC/温度阈值，**采用**`BatteryConfig` 键（`soc.shutdown/critical/warning/low/normal/high/full`、`temperature.high/low`），默认值放在 `services/native/profile/battery_config.json`，允许 vendor 覆盖（配置策略 `GetOneCfgFile("etc/battery/battery_config.json")`）。
- 新增 HiSysEvent 事件必须先在 `batterymgr.yaml` 声明（AGENTS.md DFX 路由）。
- 新增场景化配置项必须以 `sceneName` 命名空间区分，并确认 HDI 侧支持（`IsBatteryConfigSupported` 探测）。

## 关机充电子模块

- **禁止**在 `charger/` 中引用 `services/native/` 的开机服务类或链接 `libbatteryservice.z.so`（关机时不可用）。
- **禁止**交叉引用开机服务资源；设备差异通过 `charger/include/dev/`、`charger/src/dev/` 适配层（DRM/fbdev）隔离。
- **禁止**在 charger 主循环中做与充电 UI/保护无关的长任务，循环周期由 `UpdateWaitInterval`/`UpdateEpollInterval` 控制，保持按键响应。
