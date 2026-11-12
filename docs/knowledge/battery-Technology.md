# 技术术语知识

battery_manager 模块核心技术框架和机制的知识定义，涵盖价值认知、原理、使用方式和代码关联。路径均相对仓根（`base/powermgr/battery_manager`）。涉及的数据与持有者见 [业务实体知识](battery-Entity.md)，术语释义见 [术语表](glossary.md)，分层与约束总览见 [架构设计及约束](architecture.md)。

## HDI 电池事件上报与自动重订阅机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 电池数据源头在驱动，用户态若各自轮询 sysfs 会产生重复 IO 与状态不一致；HDI 服务（用户态驱动服务）可能随驱动升级重启，重启后若不重新注册回调，电池上报将永久中断 |
| 技术目标 | 单一真相源（数据只从 HDI 进来）+ 断线自愈（HDI 重启后自动恢复订阅并全量刷新状态） |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `BatteryService::RegisterHdiStatusListener` 经 HDI ServiceManager 注册 `HdiServiceStatusListener`（`ServStatListenerStub` 子类，回调转发）；收到 `battery_interface_service` 的 `SERVIE_STATUS_START` → FFRT 任务：`RegisterBatteryHdiCallback`（new `BatteryCallback` 并 `iBatteryInterface_->Register(callback)`，再把 `HandleBatteryCallbackEvent` 绑定到 `BatteryCallback::RegisterBatteryEvent` 静态回调）→ `SetLowCapacityThreshold`（特性开关开启时向 HDI 下发 `low_battery_thers`）→ `InitBatteryInfo` 全量拉取；`SERVIE_STATUS_STOP` → `UnRegister` 并置空接口。注册失败按 `RETRY_TIME`=1000ms 延迟重试（FFRT） |
| 关键流程 | 驱动变化 → HDI `BatteryCallback::Update(event)` → 静态 `eventCb_` → `BatteryService::HandleBatteryCallbackEvent`：mock 激活直接丢弃；`ConvertingEvent` 写入 `batteryInfo_`；与 `lastBatteryInfo_` 全等则去重返回；否则 `HandleBatteryInfo` 完整分发。另有 SA 层兜底：`OnRemoveSystemAbility(DEVICE_SERVICE_MANAGER_SA_ID)` 置 `isHdiRemove_`，该 SA 重新上线时重走注册（`battery_service.cpp:145-155`） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 无对外 API；服务内部触发。`OnStart` 首次注册；HDI 上下线与设备服务管理 SA 变化自动驱动重订阅。测试可经 hidumper mock（见"HIDump 调试与 Mock 机制"）模拟事件链路 |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryCallback`（`services/native/include/battery_callback.h`）、`HdiServiceStatusListener`（`services/native/include/hdi_service_status_listener.h`）、`BatteryService` |
| 核心成员 | `BatteryCallback::eventCb_`（静态）、`iBatteryInterface_`、`hdiServiceMgr_`、`hdiServStatListener_`、`isBatteryHdiReady_`/`isHdiRemove_`、`g_queue` |
| 核心方法 | `BatteryCallback::Update`/`RegisterBatteryEvent`；`BatteryService::RegisterHdiStatusListener`/`RegisterBatteryHdiCallback`/`HandleBatteryCallbackEvent`/`ConvertingEvent`/`InitBatteryInfo`（`services/native/src/battery_service.cpp:195-418`） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 回调链路同步执行，禁止阻塞（AGENTS.md 禁止事项）；HDI 代理版本 ≥ `libbattery_proxy_2.0.z.so`（`sa_profile/3302.json`） |
| 并发约束 | `RegisterBatteryHdiCallback` 写锁、`Get*Inner` 读锁；重订阅任务在 FFRT 队列串行；重复 `Update` 依赖 `lastBatteryInfo_` 去重，多事件并发到达需保序 |
| 安全约束 | 无权限要求（服务内部）；mock 状态激活期间上报被忽略，需 `-r` 复位恢复 |

## 电池公共事件分发机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 应用与系统组件需要感知电池变化（刷新 UI、触发省电策略），逐一拉取或自建监听会造成重复 IPC 与状态漂移；驱动级 uevent 还承载关机/重启/弹窗等扩展指令，需要统一解释 |
| 技术目标 | 一处变化、多方订阅；边沿事件不重复打扰；厂商 uevent 指令有受控出口 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `BatteryNotify::PublishEvents` 分两组通道：(1) 常规事件族——`BATTERY_CHANGED`（全字段 Want 参数，每次变化都发）、`BATTERY_CHANGED_INNER`（补充 maxCurrent/maxVoltage/nowCurrent/chargeCounter，订阅需 `POWER_OPTIMIZATION`）、LOW/OKAY、CONNECTED/DISCONNECTED、CHARGING/DISCHARGING、CHARGE_TYPE_CHANGED（后六类 once 边沿触发：全局 bool 标志 + 反向条件复位）；(2) uevent 动作通道——uevent 含 `$act` 时 `HandleUevent` 解释：`shutdown`/`reboot` → PowerMgrClient 电源动作；`sendcommonevent` → 补发 CHANGED；`sendcustomevent`（前缀 `usual.event.battery`）→ `PublishCustomEvent`（先过 `BATTERY_UEVENT_CHECK` 钩子可否决）；`sendpopup` → 事件 + `HandleNotification` 弹窗。发布前先 `IsCommonEventServiceAbilityExist` 探测 CES，未就绪返回 `ERR_NO_INIT` |
| 关键流程 | 状态变化时同时写 HiSysEvent "CHANGED"（仅 capacity/pluggedType/temperature/healthState 任一变化时）；CAPACITY_LEVEL 参数仅在档位变化时携带（`g_lastCapacityLevel` 去重）；无线充电开关开启时执行 `BATTERY_PUBLISH_EVENT` 钩子（`PublishEventContext{pluggedType,lastPluggedType,wirelessChargerEnable}`） |
| 补发机制 | 开机首轮：`AddBootCommonEvents` 依 `boot_actions.sendcommonevent` 配置（默认场景 `wireless`≠"0"，即无线充电时）在 uevent 尾拼 `$sendcommonevent`；CES 上线后 `OnAddSystemAbility` 用当前快照补发一次 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 应用侧订阅 `@ohos.commonEvent` 的 `usual.event.BATTERY_*`；系统侧可调 `BatterySrvClient::HandleNotification(ueventName)` 触发弹窗链路（服务端需系统权限 + `POWER_MANAGER`） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryNotify`（`services/native/include/battery_notify.h`） |
| 核心成员 | `lowCapacity_`、`lastCapacity_`/`lastPluggedType_`/`lastTemperature_`/`lastHealthState_`/`lastPowerPluggedType_`/`batteryInfoChargeType_`、`g_batteryLowOnce` 等 6 个 once 标志、`g_lastCapacityLevel` |
| 核心方法 | `PublishEvents`、`HandleUevent`、`PublishChangedEvent(Inner)`、`PublishLow/OkayEvent`、`PublishPowerConnected/DisconnectedEvent`、`PublishCharging/DischargingEvent`、`PublishChargeTypeChangedEvent`、`PublishCustomEvent`、`HandleNotification`（`services/native/src/battery_notify.cpp`） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | CES 未就绪时发布失败返回 `ERR_NO_INIT`（服务启动早期依赖 `OnAddSystemAbility(COMMON_EVENT_SERVICE_ID)` 补发）；INNER/自定义事件仅限 `POWER_OPTIMIZATION` 订阅者 |
| 并发约束 | once 去重依赖 HDI 回调串行调用；`PublishLow/OkayEvent` 段持 `mutex_`；禁止在别处并发调用 Publish 系列破坏边沿语义 |
| 安全约束 | uevent 动作白名单外的动作拒绝执行仅记日志；弹窗链路服务端双重权限校验 |

## 场景化电池配置透传机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 充电策略（如无线充电开关、快充使能、温控阈值）由驱动/厂商实现且形态各异，框架无法穷举语义 |
| 技术目标 | 用统一的 `sceneName` 字符串命名空间把配置读写请求安全地透传给 HDI，框架只管权限与通道 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 三方法链路：`SetBatteryConfig(sceneName, value)` / `GetBatteryConfig(sceneName) → result` / `IsBatteryConfigSupported(sceneName) → bool`。服务端 `*Inner` 先做权限校验（Set 需系统 + `POWER_OPTIMIZATION`；Get/IsSupported 需系统），再持读锁调用同名 HDI 接口，HDI 返回值映射为 `BatteryError::ERR_OK/ERR_FAILURE`；IPC 层错误码经 `batteryErr` 出参回传，方法本身恒返回 `ERR_OK`（IPC 送达语义与业务语义分离） |
| 关键流程 | 已知场景键：`wireless`（开机补发判定）、`low_battery_thers`（HDI 侧低电量阈值，`SetLowCapacityThreshold` 在 HDI 就绪时自动下发 `soc.shutdown` 值） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | `BatterySrvClient::SetBatteryConfig/GetBatteryConfig/IsBatteryConfigSupported`（C++ inner_api）；ArkTS 侧对应系统 API；新增场景需 HDI 侧先行支持并用 IsBatteryConfigSupported 探测 |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryService`（`services/native/src/battery_service.cpp:720-781`）、`BatterySrvClient` |
| 核心成员 | `iBatteryInterface_`（透传目标） |
| 核心方法 | `SetBatteryConfigInner`/`GetBatteryConfigInner`/`IsBatteryConfigSupportedInner`/`SetLowCapacityThreshold`；IDL 定义 `services/zidl/IBatterySrv.idl:32-34` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | HDI 不在线时返回 `ERR_FAILURE`；配置语义完全由 HDI/厂商定义，框架不校验取值 |
| 并发约束 | 读锁内调用 HDI（HDI 调用自身不可长时间阻塞） |
| 安全约束 | 权限校验必须在服务端（AGENTS.md 架构不变量）；非系统调用收到 `ERR_SYSTEM_API_DENIED(202)` |

## 电量档位计算机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | UI 提示与省电策略需要离散的电量语义（"低电""严重低电"），直接比较百分比会让各消费者各自为政 |
| 技术目标 | 按可配置阈值把连续 SOC 映射为统一档位枚举，并保证阈值配置自检 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `GetCapacityLevelInner` 取当前容量后按区间链判定：`(INVALID, shutdown]→SHUTDOWN`、`(shutdown, critical]→CRITICAL`、`(critical, warning]→WARNING`、`(warning, low]→LOW`、`(low, normal]→NORMAL`、`(normal, high]→HIGH`、`(high, full]→FULL`（左开右闭，`CapacityLevelCompare`）；进入前先校验阈值链单调递增（`shutdown ≤ 0` 或逆序时记 `capacityThreshold err` 日志，但继续按区间判定）；默认阈值 1/5/10/20/90/99/100 来自 `BatteryConfig` 的 `soc.*` |
| 关键流程 | 查询入口 `GetCapacityLevel` → `GetCapacityLevelInner` → `GetCapacityInner`（mock 时返回 mock 值，否则实时读 HDI） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | `BatterySrvClient::GetCapacityLevel()`；`BATTERY_CHANGED` 事件的 `capacityLevel` 参数（仅档位变化时携带） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryService`；枚举 `BatteryCapacityLevel`（`interfaces/inner_api/native/include/battery_info.h:133-178`） |
| 核心成员 | `shutdownCapacityThreshold_` … `fullCapacityThreshold_` 8 个阈值（`InitConfig` 加载） |
| 核心方法 | `GetCapacityLevelInner`、`CapacityLevelCompare`、`IsCapacityLevelDefined`（`battery_service.cpp:1014-1054`） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 阈值未配置（`INVALID_BATT_INT_VALUE`）时对应区间判定为 false，最终返回 `LEVEL_NONE` |
| 并发约束 | 阈值仅启动期加载，运行期只读 |
| 安全约束 | 无权限要求（公共信息） |

## 剩余充电时间估算机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 用户需要"还有多久充满"的估计，但充电功率随温度/SOC 曲线变化，精确建模需驱动深度参与 |
| 技术目标 | 不依赖驱动模型的轻量在线估算：用充电过程中的容量-时间采样外推 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `CalculateRemainingChargeTime(capacity, chargeState)`：非 ENABLE 状态清零（`remainTime_=0`、`chargeFlag_=false`）；首次进入充电记录基线（`lastCapacity_`、`lastTime_`，CLOCK_MONOTONIC 毫秒）；容量若回落则下调基线防跳变；当增量 ≥1% 时计算 `onceTime = Δt/Δcapacity`，外推 `remainTime_ = (100 - capacity) × onceTime` 并滚动更新基线。查询侧 `GetRemainingChargeTimeInner` 需系统权限 |
| 关键流程 | 每次电池变化（`HandleBatteryInfo`）调用一次；结果经 IPC `GetRemainingChargeTime` 输出（失败/未充电返回 `INVALID_REMAINING_CHARGE_TIME_VALUE(-1)`） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | `BatterySrvClient::GetRemainingChargeTime()`（系统权限）；应用侧对应 `@ohos.batteryInfo` 系统能力 |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryService` |
| 核心成员 | `remainTime_`、`chargeFlag_`、`lastCapacity_`、`lastTime_` |
| 核心方法 | `CalculateRemainingChargeTime`、`GetRemainingChargeTimeInner`（`battery_service.cpp:974-1012`） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | capacity > 100 时拒绝计算；充满前最后 1% 的估算偏保守；仅充电态有效 |
| 并发约束 | 字段在 HDI 回调线程更新、IPC 线程读取，无锁（int64 原子性由平台保证，容忍单次读到旧值） |
| 安全约束 | 查询需系统权限，非系统返回 -1 |

## 低电量延迟休眠/关机保护机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 电量耗尽前若不干预会硬掉电，损坏数据；但瞬时跌破阈值（如负载尖峰时的电量抖动）不应立即关机；等待期内系统可能先进入休眠（S3/ULSR）打断关机任务 |
| 技术目标 | 容忍抖动的延迟保护：60 秒缓冲期内可被充电/回升取消，任务执行不被休眠打断，且极简省电模式下让位于系统策略 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 两种实现按特性开关编译：**基础版 `HandleCapacity`**（默认）——`capacity ≤ shutdown阈值` 且无有效任务且在场未充电时，FFRT 延迟 60000ms（`SHUTDOWN_DELAY_TIME_MS`）执行 `ShutDownDevice("LowCapacity")`（极简省电模式 `IsInExtremePowerSaveMode`——EXTREME 模式或蓬莱 minisys——则放弃）；充电时取消任务。**扩展版 `HandleCapacityExt`**（`battery_manager_feature_set_low_capacity_threshold` 开启）——创建条件增加"容量下降"路径（`capacity < lastCapacity` 也触发）；执行时按 `const.power.enable_s4` 决定 `Hibernate(false)` 或 ShutDown；创建任务时同步创建 RunningLock "ShutDownGuard"（`RUNNINGLOCK_BACKGROUND_TASK`，`Lock(90000ms)`）防休眠打断；取消条件为容量回升或开始充电 |
| 关键流程 | 休眠联动：订阅 `COMMON_EVENT_ENTER/EXIT_HIBERNATE`，收到即 `CancelTimer(TIMER_ID_DELAY_HIBERNATE)` 并 `UnlockShutdownGuard`（系统已休眠说明保护让位）；定时器为全局 `g_ffrtTimer`（FFRTTimer）。HDI 侧同步：`SetLowCapacityThreshold` 把 `soc.shutdown` 下发为 `low_battery_thers`，让驱动层提前降频/告警 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 无对外 API；完全由 HDI 上报驱动。行为开关：`batterymgr.gni` 的 `battery_manager_feature_set_low_capacity_threshold` + 系统参数 `const.power.enable_s4` |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryService`、`BatteryCommonEventSubscriber`（`services/native/include/battery_service.h:208-214`） |
| 核心成员 | `g_lowCapacityShutdownHandle`（基础版 FFRT 句柄）、`g_ffrtTimer`、`g_shutdownGuard`、`shutdownGuardMutex_`、`isHibernateEnable_`、`shutdownCapacityThreshold_` |
| 核心方法 | `HandleCapacity`/`HandleCapacityExt`、`CheckIfCreateHibernateTask`/`CheckIfClearHibernateTask`、`DoHibernateOrShutdown`、`IsDelayHibernateTimerValid`、`CreateShutdownGuard`/`LockShutdownGuard`/`UnlockShutdownGuard`、`SubscribeCommonEvent`/`UnSubscribeCommonEvent`、`IsInExtremePowerSaveMode`（`battery_service.cpp:510-676`） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 基础版依赖 boot 完成（`Dump` 亦受 `isBootCompleted_` 门控）；扩展版 hibernate 依赖 S4 能力 |
| 并发约束 | `shutdownGuardMutex_` 保护 guard 操作；定时器任务在 FFRT 队列执行，锁/解锁需与取消路径互斥 |
| 安全约束 | 关机属不可逆动作，触发条件三重判定（阈值+在场+未充电/容量下降），取消路径充分（充电/回升/休眠事件） |

## 充电音效按需加载机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 充电提示音依赖播放器/媒体库等重资源，常驻链接会显著增加 powermgr 进程内存与启动时间；且 dlopen 卸载部分 OHOS 共享库会引发崩溃（符号已释放） |
| 技术目标 | 插入充电器时才拉起播放路径，控制资源占用并规避卸载陷阱 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `BATTERY_MANAGER_ENABLE_CHARGING_SOUND` 宏下两条路径：(1) 扩展能力路径——`TriggerChargingSound(isStart)` 经 AMS `StartExtensionAbility/StopExtensionAbility` 拉起 `com.ohos.powerdialog` 的 `ChargeSoundPlayerExtension`，音频路径由配置策略解析（`GetOneCfgFile("resource/media/audio/ui/PowerConnected.ogg")`，回退 `/vendor/etc/battery/PowerConnected.ogg`）；(2) 进程内辅助路径 `StartChargingSoundFunc`——`AntiMemLeak` 先以 `RTLD_NODELETE` 预热 libphonenumber（规避已知泄漏），`dlopen("libcharging_sound.z.so", RTLD_NOLOAD→RTLD_LAZY)` 后 `dlsym` 取 `ChargingSoundStart/IsPlaying/ChargingSoundRelease`，ffrt 提交播放循环（最长 2000ms、100ms 轮询停止位），完成后 `dlclose`；`g_released/g_stopping` 原子量防重入。boot 未完成不播放 |
| 关键流程 | `PublishPowerConnectedEvent` 触发 start（伴随 `StartVibrator` 场景 `start_charge`）；`PublishPowerDisconnectedEvent` 触发 stop |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 无对外 API。产品启用需置 `battery_manager_feature_enable_charging_sound = true` 并部署音频资源与 power_dialog 应用 |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryNotify`（`TriggerChargingSound`/`GetChargingSoundPath`）、匿名命名空间 `StartChargingSoundFunc`/`AntiMemLeak`（`services/native/src/battery_notify.cpp:346-522`） |
| 核心成员 | `g_stopping`、`g_released`、`MAX_PLAY_TIME_MS(2000)`、`TICK_INTERVAL_MS(100)` |
| 核心方法 | `TriggerChargingSound`、`GetChargingSoundPath`、`StartChargingSoundFunc` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | power_dialog 应用与音频资源必须存在；boot 完成前静默跳过 |
| 并发约束 | `g_released` CAS 保证单实例播放；重 dlopen 已知崩溃库（libmedia_client 等）禁止 dlclose（源码注释明确列出） |
| 安全约束 | 播放属用户体验非安全路径；失败仅记日志不影响事件发布结果 |

## 插件与钩子扩展机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 厂商需要在不修改框架代码的情况下定制电池行为（如无线充电场景的专属事件、自定义 uevent 过滤），且扩展不能拖慢主流程 |
| 技术目标 | 固定执行点的进程内钩子 + 目录扫描式动态插件，实现解耦扩展 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | **插件**：`OnStart` 时 `ModuleMgrScan(BATTERY_PLUGIN_AUTORUN_PATH)`（64 位 `/system/lib64/batteryplugin/autorun`，否则 `/system/lib/batteryplugin/autorun`）加载目录内动态库，`OnStop` 时 `ModuleMgrDestroy`。**钩子**：`utils/hookmgr` 提供 `GetBatteryHookMgr()`；本仓定义 `BatteryHookStage`：`BATTERY_UEVENT_CHECK`（`PublishCustomEvent` 前，插件可置 `checkResult=false` 否决发布，`UEVENT_CHECK_INFO` 载荷）、`BATTERY_PUBLISH_EVENT`（无线充电开关开启时的发布上下文，`PublishEventContext` 载荷）；执行用 `HookMgrExecute` |
| 关键流程 | 自定义事件：HDI uevent `$sendcustomevent` → 钩子校验 → 发布；无线充电：`PublishEvents` 末段（`BATTERY_MANAGER_ENABLE_WIRELESS_CHARGE`）注入上下文执行钩子 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 厂商插件库实现约定符号后放入 autorun 目录；跨仓消费者可链接 `utils/hookmgr:battery_hookmgr`（inner_kits，头文件 `battery_hookmgr.h`，导出受 `libbattery_hookmgr.map` 约束） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryHookMgr` 阶段枚举与载荷结构（`utils/hookmgr/include/battery_hookmgr.h:27-42`） |
| 核心成员 | `g_moduleMgr`（`battery_service.cpp:51`）；枚举上限 `BATTERY_HOOK_STAGE_MAX = 1000` |
| 核心方法 | `GetBatteryHookMgr`、`HookMgrExecute`（调用点 `battery_notify.cpp:115-116, 629-630`）、`ModuleMgrScan`/`ModuleMgrDestroy`（`battery_service.cpp:109, 437-440`） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 插件目录属系统镜像，新增需镜像构建配合；钩子在事件发布路径上执行，必须轻量 |
| 并发约束 | 钩子随 Publish 链路串行执行；插件加载发生在 OnStart 单线程阶段 |
| 安全约束 | 插件以 powermgr 进程权限运行，属系统信任域；导出符号变更影响 power_manager 等下游仓（跨仓 inner_kit） |

## HIDump 调试与 Mock 机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 电池问题复现依赖物理插拔/电量耗尽，自动化测试与现场调试需要不改驱动即可伪造状态 |
| 技术目标 | 提供受权限保护的调试入口：查看真实状态、模拟拔出/容量/uevent，并可复位回真实模式 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `BatteryService::Dump(fd, args)` 门控：`isBootCompleted_` + `Permission::IsSystem()`；`BatteryDump` 单例解析参数：`-h` 帮助、`-i` 输出容量/档位/充电状态/健康/电压/温度/电流等；非用户版本（`#ifndef BATTERY_USER_VERSION`）另有 `-u`（`MockUnplugged`）、`--capacity N`（0-100，`MockCapacity`）、`--uevent S`（`MockUevent`）、`-r`（`Reset`）。Mock 实现：置对应标志 → 主动 `GetBatteryInfo` 全量拉取 → `ConvertingEvent`（此时其他字段仍写真实值）→ 改写目标字段（拔出时 pluggedType=NONE、电流电压清零、chargeState=NONE）→ `HandleBatteryInfo` 走完整分发链路（因此下游事件/灯光/保护逻辑全部被触发）。此后 HDI 真实上报因 mock 标志在 `HandleBatteryCallbackEvent` 被忽略，`ConvertingEvent` 内对应字段也不再被覆盖 |
| 关键流程 | `Reset` 清三标志并重新全量同步 + 销毁 `g_ffrtTimer`；`DumpCurrentTime` 输出时间戳便于对账 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | `hidumper -s 3302 [-h|-i|-u|-r|--capacity N|--uevent S]`（后四项仅非用户版本）；测试仓 `test/systemtest`、`test/fuzztest`（17 个 fuzzer，含 `setbatteryconfig/getbatteryconfig/isbatteryconfigsupported`）覆盖相关面 |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryDump`（`services/native/include/battery_dump.h`，实现 `services/native/src/battery_dump.cpp`）、`BatteryService` |
| 核心成员 | `isMockUnplugged_`/`isMockCapacity_`/`isMockUevent_` |
| 核心方法 | `BatteryService::Dump`/`MockUnplugged`/`MockCapacity`/`MockUevent`/`Reset`（`battery_service.cpp:1056-1159`）；`BatteryDump::GetBatteryInfo`/`MockUnplugged`/`MockCapacity`/`MockUevent`/`Reset`/`DumpBatteryHelp` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 仅 boot 完成后可用；mock 命令不进用户版本镜像 |
| 并发约束 | Mock 系列持读锁修改标志（与 `Reset` 互斥性依赖调用频率低的前提） |
| 安全约束 | 系统权限 + 非用户版本双重门槛；mock 拔出会触发唤醒/事件等真实副作用，测试环境专用 |

## 关机充电独立运行机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 设备关机插入充电器时需要展示充电 UI 并支持长按开机，但此时 SystemAbility 层系统服务（samgr/CES/通知等）均未运行，开机态架构完全不可用 |
| 技术目标 | 以最小依赖集（sysfs + epoll + ui_lite + MMI input）实现关机充电的用户交互与安全保护 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | **事件驱动**：`BatteryThread::LoopingThreadEntry` epoll 同时监听 uevent socket（`NETLINK_KOBJECT_UEVENT`，仅 `SUBSYSTEM=power_supply`）与 timerfd；uevent 到来立即 `UpdateBatteryInfo`，否则按 `UpdateEpollInterval(chargeState)` 调整的周期执行 `CycleMatters`。**数据直读**：`PowerSupplyProvider` 遍历 `/sys/class/power_supply`（`TraversalNode`/`CheckSubfolderNode`）缓存 capacity/temperature/charge_state 文件路径，`Parse*` 系列读文件转数值（charge_state 字符串经 `ChargeStateEnumConverter` 映射枚举，含 RESERVED）。**UI 栈**：`ChargerAnimation` 按 `animation.json` 声明渲染（UIImageView 62 帧 @60ms + 电量 UILabel + 低电两种提示）；显示设备经 `charger/src/dev/` 适配（DRM 优先，fbdev 回退）。**保护与开机**：温度越界或"容量高于缺电阈值却未充电"→ `DoReboot("shutdown")`；长按电源键 2000ms → 震动 75ms → `ClearPowerOffChargeFlag`（dlopen `libpower_charge_ext.z.so` 清充电标志）→ `DoReboot("")` 正常开机 |
| 关键流程 | 状态机每轮 `HandleStates`：充电状态变化→亮屏刷动画；电源键状态机（`g_keys` 记录按下时间戳，`keyWait_` 控制最长等待）；背光 10s 超时熄屏并停动画 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 无运行时 API。产品启用需 `battery_manager_feature_enable_charger = true`（默认 false），由 init 按 `power_manager_feature_poweroff_charge` 决定拉起时机；`battery_manager_feature_support_battery_cli` 的 `battery-shell` CLI（`tools/ohos-batteryManager`，独立 npm 包）提供开机态调试查看 |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `ChargerThread`（`charger/include/charger_thread.h`）、`BatteryThread`（`charger/include/battery_thread.h`）、`PowerSupplyProvider`（`charger/include/power_supply_provider.h`）、`ChargerAnimation`、`BatteryLed`/`BatteryBacklight`/`BatteryVibrate` |
| 核心成员 | `epFd_`/`ueventFd_`/`epollInterval_`/`callbacks_`（基类）；`capacity_`/`chargeState_`/`lackPowerCapacity_`/`keyWait_`/`backlightWait_`/`g_keys[]` |
| 核心方法 | `ChargerThread::Run/Init/CycleMatters/UpdateBatteryInfo/HandleStates/HandlePowerKey/UpdateAnimation/HandleTemperature/HandleCapacity`；`PowerSupplyProvider::InitPowerSupplySysfs/ParseCapacity/ParseTemperature/ParseChargeState`；入口 `charger/src/charger.cpp:23` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | samgr/CES/SA 不可用，任何新增依赖必须能在 init 后早期环境运行；HDI 驱动服务可用（LED 经 `ILightInterface`、背光经 `IDisplayComposerInterface`、输入经 MMI），显示另依赖 drivers_interface_display 与 graphic 栈裁剪配置（gni 的 part 检测） |
| 并发约束 | 主循环单线程；MMI 回调线程写 `g_keys`（去抖：同状态忽略）；`animation_`/`lackPowerCapacity_` 为静态成员 |
| 安全约束 | 开机/关机决策仅依赖本地 sysfs 值与配置阈值；`ClearPowerOffChargeFlag` 经 `RTLD_NODELETE` 动态加载，失败则跳过（仍执行重启） |

## 电池事件通知（弹窗/通知中心）机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 部分 uevent 场景（如充电器不兼容提醒）需要以系统通知或弹窗形式触达用户，而通知的构建与本地化属应用层能力 |
| 技术目标 | 框架解析配置并按需加载通知实现库，把"何时通知"与"如何通知"解耦 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `BatteryNotify::HandleNotification(ueventName)`（`BATTERY_SUPPORT_NOTIFICATION` 宏）：查 `BatteryConfig::GetPopupConf()`（`uevent → [PopupConf{name, action}]`）→ `dlopen("libbattery_notification.z.so", RTLD_LAZY|RTLD_NODELETE)` 取 `HandleNotification(name, action, notificationConfMap)` 符号 → 逐项执行（action 区分按钮行为）。通知库位于 `services/native/notification/`：`IBatteryNotification` 接口（`CreateBaseStyle`/`SetActionButton`/`PublishNotification`/`CancelNotification`），`NotificationCenter`/`NotificationManager`/`NotificationDecorator`/`button_event`/`notification_locale`（i18n）组合实现，文案与按钮来自 `NotificationConf` |
| 关键流程 | 触发源有二：uevent `$sendpopup` 动作；产品型号 uevent（`RVS_ADAPTER_PRODUCT_TYPE=3/4/5`）的 `sendcustomevent` 分支附带调用 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 系统侧 `BatterySrvClient::HandleNotification(ueventName)`（服务端系统权限 + `POWER_MANAGER`）；配置新增在 `battery_config.json` 的 popup/notification 节 |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryNotify`、`IBatteryNotification` 及 notification 目录实现（`services/native/notification/`） |
| 核心成员 | `BatteryConfig::popupConfig_`、`notificationConfMap_` |
| 核心方法 | `BatteryNotify::HandleNotification`（`battery_notify.cpp:653-687`）；配置解析 `BatteryConfig::ParsePopupConf`/`ParseNotificationConf`/`SaveNotificationConfToMap` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | `BATTERY_SUPPORT_NOTIFICATION` 关闭时函数体为空恒返回 true；通知依赖分布式通知服务在线 |
| 并发约束 | dlopen/dlclose 每次调用执行，`RTLD_NODELETE` 规避重加载崩溃 |
| 安全约束 | 双重权限（系统 + `POWER_MANAGER`）；通知文案来自受保护配置文件 |
