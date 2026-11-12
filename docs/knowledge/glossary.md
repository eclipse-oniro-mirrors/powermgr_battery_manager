# 术语表

## 电池服务（BatteryService / SA 3302）
本部件的系统服务端，进程 `powermgr`，库 `libbatteryservice.z.so`，SA ID `POWER_MANAGER_BATT_SERVICE_ID`（3302）。持有唯一电池状态并对外提供查询与事件分发。区别于客户端 `BatterySrvClient`：服务端是状态持有者，客户端只是转发者。代码：`services/native/include/battery_service.h`。

## 电池信息数据模型（BatteryInfo）
服务端内存中的电池状态快照（容量、电压、温度、健康、充电状态、电流、技术类型、uevent 等 17 个字段），带 `INVALID_*` 哨兵默认值与 `COMMON_EVENT_KEY_*` 事件参数键。注意它不是 Parcelable——跨进程经 IPC 时使用基本类型出参、经公共事件时以 Want 参数传递。代码：`interfaces/inner_api/native/include/battery_info.h`。

## IPC 接口（IBatterySrv / IDL）
客户端与服务端之间的 18 方法接口契约，源文件 `services/zidl/IBatterySrv.idl`，proxy/stub 由构建工具生成。`[ipccode N]` 码值是 ABI 一部分，禁止复用。区别于 HDI：IDL 定义进程间（应用↔服务）调用，HDI 定义服务↔驱动调用。

## HDI 电池接口（IBatteryInterface / battery_interface_service）
`OHOS::HDI::Battery::V2_0::IBatteryInterface`，电池数据唯一真相源，服务名 `battery_interface_service`，代理库 `libbattery_proxy_2.0.z.so`（`sa_profile/3302.json` 的 `min_hdi_proxy_version`）。本仓只消费不维护（归 `drivers_interface_battery` 仓）。

## 充电状态（BatteryChargeState / ChargeState）
电池是否在充电：NONE(0 未接电源)、ENABLE(1 充电中)、DISABLE(2 接电源但未充电)、FULL(3 充满)。`IsCharging` 判定仅认 ENABLE。代码：`battery_info.h` 枚举 `BatteryChargeState`。与"充电类型"（PluggedType）不同：前者描述电池行为，后者描述电源来源。

## 充电类型（BatteryPluggedType / PluggedType）
电源来源：NONE(0)、AC(1)、USB(2)、WIRELESS(3)。插拔判断（`IsPlugged`/`IsUnplugged`）与 POWER_CONNECTED/DISCONNECTED 事件以它为准。代码：`battery_info.h` 枚举 `BatteryPluggedType`。

## 充电档位（ChargeType）
充电能力的档位划分（有线/无线 × 普通/快充/超级快充），来自 HDI `GetChargeType`，变化时发布 `usual.event.CHARGE_TYPE_CHANGED`。区别于 PluggedType（来源）与 ChargeState（行为）。代码：`battery_info.h` 枚举 `ChargeType`。

## 容量档位（BatteryCapacityLevel）
按 SOC 阈值划分的电量区间：NONE、FULL、HIGH、NORMAL、LOW、WARNING、CRITICAL、SHUTDOWN、RESERVED。阈值来自配置 `soc.shutdown/critical/warning/low/normal/high/full`（默认 1/5/10/20/90/99/100），区间为左开右闭。计算见 `BatteryService::GetCapacityLevelInner`。注意：AGENTS.md 术语表中该枚举为 5 档旧描述，当前代码为上述 9 值枚举。

## 健康状态（BatteryHealthState）
电池健康诊断值：UNKNOWN、GOOD、OVERHEAT、OVERVOLTAGE、COLD、DEAD（代码实际 6 值；AGENTS.md 中的 UNSPECIFIED_FAILURE 在当前 `battery_info.h` 中不存在）。

## 电池公共事件（usual.event.BATTERY_*）
`BatteryNotify` 发布的系统级广播：BATTERY_CHANGED（通用变化）、BATTERY_CHANGED_INNER（带电流细节、需 `ohos.permission.POWER_OPTIMIZATION` 订阅）、BATTERY_LOW/BATTERY_OKAY（低电/恢复，边沿触发）、CHARGING/DISCHARGING、POWER_CONNECTED/DISCONNECTED、CHARGE_TYPE_CHANGED，以及 `usual.event.battery*` 前缀自定义事件。与"HiSysEvent"不同：CommonEvent 面向业务订阅，HiSysEvent 面向 DFX 统计。

## uevent 动作（uevent act）
HDI uevent 字符串中以 `$` 分隔的尾部指令，由 `BatteryNotify::HandleUevent` 解释执行：`shutdown`（关机，reason "POWEROFF_CHARGE_DISABLE"）、`reboot`、`sendcommonevent`（发 BATTERY_CHANGED）、`sendcustomevent`（发 battery 前缀自定义事件）、`sendpopup`（发事件并触发通知弹窗）。是驱动/厂商区域向框架传递扩展指令的通道。

## 场景化电池配置（sceneName / SetBatteryConfig）
按场景名读写字符串配置的机制，服务端做系统权限校验后透传 HDI（`BatteryService::SetBatteryConfigInner` 等）。`sceneName` 是配置命名空间，典型键如 `wireless`（配置 `boot_actions` 用它决定开机是否补发事件）、`low_battery_thers`（低电量阈值下发 HDI）。

## 低电量保护（HandleCapacity / HandleCapacityExt）
容量 ≤ `soc.shutdown` 且未充电时启动 60 秒延迟任务：普通构建走 `ShutDownDevice("LowCapacity")`；开启 `battery_manager_feature_set_low_capacity_threshold` 的构建走 `HandleCapacityExt`——按 `const.power.enable_s4` 决定 Hibernate 或 ShutDown，并用 RunningLock "ShutDownGuard" 防休眠打断。充电或容量回升取消任务。

## 关机充电模式（charger）
关机状态下插入充电器时由 init 拉起的独立可执行程序（`charger/src/charger.cpp`），含充电动画、LED、背光、震动与电源键长按开机（2 秒，`REBOOT_TIME`），直读 sysfs，与开机服务互斥。`battery_manager_feature_enable_charger` 控制是否构建。

## 电量供应源（PowerSupplyProvider / sysfs）
charger 子模块的电池数据访问器，遍历 `/sys/class/power_supply` 节点读取 capacity/temperature/charge_state 文件。区别于开机态：开机态由 HDI 提供，关机态无 HDI 服务只能直读 sysfs。代码：`charger/include/power_supply_provider.h`。

## 钩子管理器（HookMgr / BatteryHookStage）
`utils/hookmgr` 提供的固定执行点插拔机制（跨仓共享，power_manager 也用）。本仓定义两个阶段：`BATTERY_UEVENT_CHECK`（自定义事件发布前可否决）、`BATTERY_PUBLISH_EVENT`（无线充电场景事件处理）。获取入口 `GetBatteryHookMgr()`。区别于插件（ModuleMgr）：钩子是在固定点的同进程逻辑，插件是从 `batteryplugin/autorun` 目录扫描加载的动态库。

## 插件目录（batteryplugin/autorun）
服务启动时 `ModuleMgrScan("/system/lib{,64}/batteryplugin/autorun")` 扫描并自动加载的插件动态库路径，用于厂商扩展（如无线充电场景处理），`OnStop` 时 `ModuleMgrDestroy`。

## 充电音效（ChargingSound / ChargeSoundPlayerExtension）
插入充电器时播放提示音的机制（`battery_manager_feature_enable_charging_sound`，默认关）：服务端经 AMS 启动 `com.ohos.powerdialog` 应用的 `ChargeSoundPlayerExtension` 扩展，音频路径由配置策略解析（默认 `/vendor/etc/battery/PowerConnected.ogg`）；进程内另有按需 `dlopen libcharging_sound.z.so` 的播放辅助路径。

## 充电震动（PowerVibrator / start_charge）
插入充电器时按 `etc/battery/battery_vibrator.json` 配置触发的马达震动，场景名 `start_charge`。代码：`utils/native`（`power_vibrator`）。

## 死亡通知（DeathRecipient / BatterySrvDeathRecipient）
客户端为 SA 代理挂载的 `IRemoteObject::DeathRecipient`，服务死亡时 `OnRemoteDied` → `ResetProxy` 置空代理，下次调用自动重连。保证客户端自愈而不崩溃。

## HDI 上下线监听（HdiServiceStatusListener）
继承 HDI `ServStatListenerStub` 的监听器，`OnReceive` 回调转发给 `StatusCallback`；用于 battery_interface_service 重启后重新注册电池回调并全量刷新状态，防止上报中断（AGENTS.md 项目特定陷阱"HDI 重启时序"）。

## HiSysEvent 电池域（BATTERY domain）
`batterymgr.yaml` 声明的 DFX 事件集合（如 STATISTIC 类型 "CHANGED"），由 `PublishChangedEvent` 在状态变化时写入，供 HiView 观测。

## 电池信息调试（BatteryDump / hidumper -s 3302）
`hidumper` 调试入口：`-h` 帮助、`-i` 信息；非用户版本另有 `-u`（模拟拔出）、`--capacity N`、`--uevent S`、`-r`（复位）。mock 标志（`isMockUnplugged_` 等）会让 HDI 后续上报被忽略直到复位。代码：`services/native/src/battery_dump.cpp`。

## 哨兵值（INVALID_BATT_*）
`battery_info.h` 定义的失败占位值：`INVALID_BATT_INT_VALUE(-1)`、`INVALID_BATT_TEMP_VALUE(100)`、`INVALID_REMAINING_CHARGE_TIME_VALUE(-1)`、`INVALID_STRING_VALUE("Invalid")`、`INVALID_BATT_BOOL_VALUE(true)`。客户端所有查询失败均返回哨兵而非异常。

## 系统能力开关（declare_args / 特性开关）
`batterymgr.gni` 的 7 个 GN 参数：`set_low_capacity_threshold`、`enable_charger`、`enable_charging_sound`、`support_notification`、`support_notification_string`、`enable_wireless_charge`、`support_battery_cli`（默认全 false），开启后注入对应 `BATTERY_MANAGER_*` 宏并需同步 `bundle.json` features 列表（现列 6 个）。

## 多 SA 共进程（powermgr 进程）
SA 3301（power）、3302（battery）、3303（thermal）、3308（display）共享 `powermgr` 进程；任一 SA 崩溃或阻塞影响全部（AGENTS.md 项目特定陷阱）。运行约束见 `sa_profile/3302.json`。

## 共享指针（sptr / wptr）
OpenHarmony `refbase.h` 引用计数指针体系（非 `std::shared_ptr`），IPC 代理、HDI 接口、单例持有普遍使用 `sptr`，弱引用用 `wptr`（如死亡通知回调参数）。
