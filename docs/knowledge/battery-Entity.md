# 业务实体知识

battery_manager 模块核心业务实体的完整知识定义，涵盖静态概念、动态运行、规则约束与代码映射。路径均相对仓根（`base/powermgr/battery_manager`）。术语释义见 [术语表](glossary.md)，实体运行所依赖的机制见 [技术术语知识](battery-Technology.md)，整体边界见 [业务背景](business-context.md)。

## 电池信息（BatteryInfo）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 电池信息快照 / BatteryInfo |
| 实体定义 | 某一时刻整机电池与充电状态的完整快照，是服务端对外呈现电池业务的唯一数据载体；身份边界：它描述"状态"而非"策略"，不含任何保护动作判定字段 |
| 核心特征 | 17 个业务字段：capacity(%)、voltage(mV)、temperature(0.1℃)、healthState、pluggedType、chargeState、chargeType、pluggedMaxCurrent/MaxVoltage、nowCurrent、curAverage、chargeCounter、totalEnergy/remainEnergy(mAh)、present、technology、uevent；枚举 `BatteryChargeState`/`BatteryHealthState`/`BatteryPluggedType`/`BatteryCapacityLevel`/`ChargeType` 与 `COMMON_EVENT_CODE_*`/`COMMON_EVENT_KEY_*` 事件键（`interfaces/inner_api/native/include/battery_info.h:214-469`） |
| 类型/分类 | 纯数据类（getter/setter 全内联），非 Parcelable；跨进程以 IPC 基本类型出参或 CommonEvent Want 参数传递 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | HDI 上报 `V2_0::BatteryInfo` → `BatteryService::ConvertingEvent` 逐字段写入服务端 `batteryInfo_` →（与 `lastBatteryInfo_` 不同才）经 `BatteryNotify` 分发 → 处理完成后 `lastBatteryInfo_ = batteryInfo_`（`battery_service.cpp:242-369`）。`InitBatteryInfo` 在 HDI 首次就绪时通过 `Get*Inner` 主动拉取构造首份快照 |
| 状态流转 | 单实体持续覆写更新；特殊状态：mock 期间 `isMockUnplugged_` 锁定 pluggedType/maxCurrent/maxVoltage/chargeState，`isMockCapacity_` 锁定 capacity，`isMockUevent_` 锁定 uevent（`ConvertingEvent` 分支）；`operator==` 用于全字段去重，`operator!=` 取反 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 无变化不分发（`lastBatteryInfo_ == batteryInfo_` 时 `HandleBatteryCallbackEvent` 直接返回）；mock 激活期间 HDI 上报被整体忽略；capacity 上限 100（`BATTERY_FULL_CAPACITY`），超出时 `CalculateRemainingChargeTime` 拒绝计算 |
| 并发约束 | 服务端 `batteryInfo_` 读写须持 `mutex_`（shared_mutex）；CommonEvent 参数组装时通过 `g_service->GetCapacityLevel()` 间接读取 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | 由 `BatteryService` 独占持有两份（当前 + 上次）；charger 子模块不复用该类（用 `BatterydInfo`/`PowerSupplyProvider` 原生值） |
| 下游影响 | `BatteryNotify`（事件参数与判定依据）、`BatteryLight::UpdateColor`（chargeState + capacity）、`CalculateRemainingChargeTime`、`HandleCapacity(Ext)`（低电量保护）、NAPI/CAPI/CJ 绑定的属性输出 |
| 平级关联 | HDI `V2_0::BatteryInfo`（上游来源，字段逐一映射）；`ChargeType` 由服务端在 `ConvertingEvent` 中额外经 `GetChargeType()` 拉取补充 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryInfo`（`interfaces/inner_api/native/include/battery_info.h`） |
| 核心方法 | setter `SetCapacity`/`SetPluggedType`/`SetChargeState`/`SetUevent` 等 17 个；getter `GetCapacity`/`GetUevent` 等；比较 `operator==`/`operator!=` |

## 电池系统服务（BatteryService）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 电池系统服务 / BatteryService（SA 3302） |
| 实体定义 | powermgr 进程内的电池业务中枢：持有电池状态、接收 HDI 上报、响应 18 个 IPC 查询/配置请求、编排事件分发与电源保护动作；身份边界：策略的发起者而非执行者（真正开关机由 power_manager 完成） |
| 核心特征 | 继承 `SystemAbility` + `BatterySrvStub`（`services/native/include/battery_service.h:51-52`）；成员：`batteryInfo_`/`lastBatteryInfo_`、`batteryNotify_`、`batteryLight_`、`iBatteryInterface_`（HDI 代理）、`hdiServiceMgr_`/`hdiServStatListener_`、8 个容量阈值、`remainTime_`/`chargeFlag_`、mock 三标志、就绪三原子量（`isBatteryHdiReady_`/`isCommonEventReady_`/`isHdiRemove_`） |
| 类型/分类 | 系统服务（`DECLARE_SYSTEM_ABILITY` + `DelayedSpSingleton`），进程 `powermgr`，`run-on-create: true` |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | `OnStart`：`Init`（配置/通知/定时器/震动/boot 回调）→ `RegisterHdiStatusListener` → `ModuleMgrScan` 加载插件 → `Publish` 注册 SA → 监听 MISCDEVICE/CES/DEVICE_SERVICE_MANAGER 三个 SA（`battery_service.cpp:98-118`）。HDI 上线 → FFRT 任务 `RegisterBatteryHdiCallback` + `SetLowCapacityThreshold` + `InitBatteryInfo`。CES 上线 → 补发首份电池事件。`OnStop`：反注册 HDI/监听器、销毁插件、退订公共事件 |
| 状态流转 | `ready_`（OnStart 成功）→ `isBatteryHdiReady_`（HDI 回调注册且首份快照就绪）→ `isCommonEventReady_`（CES 上线且补发完成）；HDI 下线时 `isHdiRemove_=true` 并清空接口，DEVICE_SERVICE_MANAGER 重新上线后重走注册；`OnRemoveSystemAbility(DEVICE_SERVICE_MANAGER_SA_ID)` 兜底 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 查询走 `Get*Inner`（带权限与 mock 分支）；`GetTotalEnergy`/`GetRemainEnergy`/`GetRemainingChargeTime` 仅系统调用可见（非系统返回哨兵）；`Dump` 需 boot 完成且系统权限；温度越界（≤low 或 ≥high 且 low≠high）即 `ShutDownDevice("TemperatureOutOfRange")`；低电量规则见 Technology 文档"低电量延迟休眠/关机保护机制" |
| 并发约束 | `mutex_`(shared_mutex) 保护 HDI 接口与 `batteryInfo_`；FFRT 队列 `g_queue("battery_service")` 承载延迟/重试任务；`remainTime_` 相关字段在 HDI 回调线程内更新 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | samgr（SA 注册与依赖监听）、HDI ServiceManager |
| 下游影响 | `BatteryNotify`/`BatteryLight`/`BatteryConfig`/`ChargingSound`(经 notify)/`BatteryDump` 均由其编排；`PowerMgrClient`（唤醒/关机/休眠/RunningLock） |
| 平级关联 | 同进程 SA 3301/3303/3308；HDI `battery_interface_service`（数据源）；`BatterySrvClient`（IPC 对端） |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryService`（`services/native/include/battery_service.h`，实现 `services/native/src/battery_service.cpp`） |
| 核心方法 | 生命周期 `OnStart`/`OnStop`/`OnAddSystemAbility`/`OnRemoveSystemAbility`；上报处理 `HandleBatteryCallbackEvent`/`ConvertingEvent`/`HandleBatteryInfo`；查询 `GetCapacity` 等 18 个 IPC 方法 + `Get*Inner`；保护 `HandleTemperature`/`HandleCapacity(Ext)`/`WakeupDevice`；配置 `SetBatteryConfigInner` 等；mock `MockUnplugged`/`MockCapacity`/`MockUevent`/`Reset` |

## 电池服务客户端（BatterySrvClient）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 电池服务客户端 SDK / BatterySrvClient |
| 实体定义 | 面向系统组件与各语言绑定的进程内查询入口；身份边界：纯转发者，不缓存业务状态、不做权限判断（权限在服务端） |
| 核心特征 | `DelayedRefSingleton` 单例；成员仅 `proxy_`（IDL 代理）、`deathRecipient_`、`mutex_`（`interfaces/inner_api/native/include/battery_srv_client.h:29-127`） |
| 类型/分类 | inner_api 公共 C++ SDK（`bundle.json` inner_kits：`battery_info.h` + `battery_srv_client.h`） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 各 `Get*` → `Connect()`（首次经 samgr `CheckSystemAbility(3302)` 获取代理、`AddDeathRecipient`）→ `proxy->Get*` → 失败返回哨兵值；服务死亡 → `OnRemoteDied` → `ResetProxy` 清代理，下次调用重连（`frameworks/native/src/battery_srv_client.cpp:35-100`） |
| 状态流转 | 无业务状态；仅代理生命周期（null ↔ connected），由 mutex 保证并发 `Connect` 安全 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 每次查询独立 IPC，无本地缓存与变化通知（通知依赖 CommonEvent 订阅）；配置类接口返回 `BatteryError`，查询类返回值即数据 |
| 并发约束 | 多线程共享单例；`Connect`/`ResetProxy` 均持 `mutex_`；代理为 `sptr` 引用计数安全 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | NAPI（batteryInfo/charger/battery 三模块）、CAPI（`ohbattery_info.cpp`）、CJ FFI、taihe 生成层，及 battery_statistics/thermal_manager 等系统组件 |
| 下游影响 | `IBatterySrv` proxy（IDL 生成物）；IDL 变更需同步其调用面 |
| 平级关联 | 死亡通知实体 `BatterySrvDeathRecipient`（内嵌类） |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatterySrvClient`、内嵌 `BatterySrvClient::BatterySrvDeathRecipient` |
| 核心方法 | `Connect`/`ResetProxy`；查询 `GetCapacity`/`GetChargingStatus`/`GetCapacityLevel`/`GetRemainingChargeTime` 等；配置 `SetBatteryConfig`/`GetBatteryConfig`/`IsBatteryConfigSupported`/`HandleNotification` |

## 电池事件分发器（BatteryNotify）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 电池事件分发器 / BatteryNotify |
| 实体定义 | 把 `BatteryInfo` 变化翻译成系统级公共事件、HiSysEvent 与本地反馈（震动/音效）的专职组件；身份边界：只负责"发"，不持有电池真值（level/plugged 等取自入参或经 `g_service` 查询） |
| 核心特征 | 成员：`lowCapacity_`（构造时读 `soc.low`，默认 20）、`batteryInfoChargeType_`、`lastCapacity_`/`lastPluggedType_`/`lastTemperature_`/`lastHealthState_`/`lastPowerPluggedType_`（去重基线）、`mutex_`（`services/native/include/battery_notify.h:27-65`） |
| 类型/分类 | 服务端内部组件（`std::unique_ptr` 持有于 `BatteryService`） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | `PublishEvents(info)` 总入口：先确认 CES 可用（`IsCommonEventServiceAbilityExist`，失败返回 `ERR_NO_INIT`）→ uevent 非空则转 `HandleUevent` 动作分支 → 否则依次发布 CHANGED/CHANGED_INNER/LOW/OKAY/（无线充电钩子）/CONNECTED/DISCONNECTED/CHARGING/DISCHARGING/CHARGE_TYPE_CHANGED（`battery_notify.cpp:87-130`） |
| 状态流转 | once 标志（`g_batteryLowOnce` 等 6 个全局 bool）实现边沿触发：进入低电置位只发一次、恢复清位；`g_lastCapacityLevel` 控制容量档位参数仅在变化时携带 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | LOW/OKAY 以 `lowCapacity_` 为界互斥边沿；CONNECTED/DISCONNECTED 以 pluggedType 是否有效判定并触发震动/音效；CHARGING/DISCHARGING 以 chargeState ∈ {ENABLE, FULL} 判定；CHARGE_TYPE_CHANGED 以 `batteryInfoChargeType_` 变化判定；INNER 与自定义事件仅限 `POWER_OPTIMIZATION` 权限订阅 |
| 并发约束 | `PublishEvents` 由 HDI 回调线程串行调用；`PublishLowEvent`/`PublishOkayEvent` 段持 `mutex_`；once 全局标志依赖串行语义，禁止并发调用 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | `BatteryService`（唯一调用方与所有者） |
| 下游影响 | CES（事件）、HiSysEvent、`PowerVibrator`、`ChargingSound` 路径（`TriggerChargingSound`→AMS）、`libbattery_notification.z.so`（`HandleNotification`） |
| 平级关联 | `BatteryConfig`（lowCapacity_、popup/notification conf）；HookMgr（`BATTERY_UEVENT_CHECK`、`BATTERY_PUBLISH_EVENT` 两个执行点） |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryNotify`（`services/native/include/battery_notify.h`） |
| 核心方法 | `PublishEvents`、`PublishCustomEvent`、`HandleNotification`、`HandleUevent`、`PublishChangedEvent(Inner)`、`PublishLowEvent`/`PublishOkayEvent`、`PublishPowerConnected/DisconnectedEvent`、`PublishCharging/DischargingEvent`、`PublishChargeTypeChangedEvent` |

## 电池配置（BatteryConfig）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 电池配置 / BatteryConfig |
| 实体定义 | 部件级电池配置的解析与查询中枢：把 `battery_config.json` 的 light/boot_actions/temperature/soc/charger/wirelesscharger/popup/notification 各节解析为强类型结构；身份边界：只读配置查询，不负责场景化配置的运行时写路径（那是 HDI 透传） |
| 核心特征 | 子结构：`LightConf{beginSoc,endSoc,rgb}`、`CommonEventConf{eventName,uevent,sceneConfigName,sceneConfigEqual,sceneConfigValue}`、`PopupConf{name,action}`、`NotificationConf{name,icon,title,text,bannerFlags,firstButton,secondButton,...}`（`services/native/include/battery_config.h:33-68`）；静态单例 + `mutex_` |
| 类型/分类 | 跨模块共享（服务端 `InitConfig` 与 charger `InitLackPowerCapacity`/LED 均使用） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | `GetInstance().ParseConfig()` → 配置策略 `GetOneCfgFile("etc/battery/battery_config.json")` 定位（vendor 优先于 system，无 config_policy part 时直接 open vendor/system 路径）→ cJSON 解析各节入库 → 各方 `GetInt(key, defVal)`/`GetLightConf()` 等查询 |
| 状态流转 | 一次解析多次查询；键语法 `节.子键`（如 `soc.low`），`SplitKey` 拆分后 `FindConf` 逐级定位，不存在返回默认值 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 默认值以 `services/native/profile/battery_config.json` 为准：light 低[0,10]红/中(10,90]黄/高(90,100]绿、temperature.high 680/low -500、soc.shutdown 1/critical 5/warning 10/low 20/normal 90/high 99/full 100、wirelesscharger 0；`boot_actions.sendcommonevent` 用 `scene_config`（默认 `wireless`≠"0"）决定开机补发；厂商可在 `/vendor/etc/battery/` 覆盖 |
| 并发约束 | 静态 `instance_` + `mutex_`；`ParseConfig` 幂等性由调用方保证（charger 用 `isConfigParse_` 防重复） |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | `BatteryService::InitConfig`（阈值加载）、`BatteryNotify`（lowCapacity_、popup/notification conf）、`BatteryLight`（lightConf）、`ChargerThread`（soc.shutdown、温度阈值） |
| 下游影响 | 间接决定容量档位区间、指示灯颜色、低电广播阈值、关机充电"缺电容量" |
| 平级关联 | 场景化配置（HDI 透传）共用 `scene_config` 键语义但通道不同 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryConfig`（`services/native/include/battery_config.h`，实现 `services/native/src/battery_config.cpp`） |
| 核心方法 | `ParseConfig`、`IsExist`、`GetInt`、`GetLightConf`、`GetWirelessChargerConf`、`GetCommonEventConf`、`GetPopupConf`、`GetNotificationConf` |

## 电池指示灯（BatteryLight）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 电池指示灯 / BatteryLight |
| 实体定义 | 充电状态下按 SOC 区间点亮指定颜色 LED 的执行组件；身份边界：只管"亮什么颜色"，不决策"该不该亮"（由 `UpdateColor` 的 chargeState 判定完成） |
| 核心特征 | 成员：`available_`（是否存在名为 "battery" 的灯）、`lightId_`（miscdevice part 编译时存在）、`lightColor_`（当前颜色）（`services/native/include/battery_light.h:23-41`） |
| 类型/分类 | 服务端值成员（`BatteryService::batteryLight_` 直接内嵌） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | `OnAddSystemAbility(MISCDEVICE_SERVICE_ABILITY_ID)` → `InitLight()` 遍历 `Sensors::GetLightList` 匹配 `lightName == "battery"` → 每次电池变化 `UpdateColor(chargeState, capacity)` 查 lightConf 区间 → 变色时 `TurnOff()` + `TurnOn(rgb)` |
| 状态流转 | 未初始化(available_=false) ↔ 可用；chargeState 为 NONE/BUTT/DISABLE 时强制熄灭并返回 false；同色不重复设置（`lightColor_ == it.rgb` 短路） |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 颜色规则由 `BatteryConfig` light 节决定（默认红/黄/绿三段）；仅在充电（ENABLE/FULL）状态亮灯 |
| 并发约束 | 随 `HandleBatteryInfo` 在 HDI 回调线程内串行调用；`lightColor_` 无锁（依赖串行语义） |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | `BatteryService`（初始化时机与调用） |
| 下游影响 | sensors miscdevice → light HDI（`drivers_interface_light`） |
| 平级关联 | `BatteryConfig::GetLightConf`；charger 子模块另有独立的 `BatteryLed`（`charger/include/battery_led.h`，sysfs 直控，非本类） |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryLight`（`services/native/include/battery_light.h`，实现 `services/native/src/battery_light.cpp`） |
| 核心方法 | `InitLight`、`TurnOn(uint32_t color)`、`TurnOff`、`UpdateColor`、`isAvailable`、`GetLightColor` |

## 关机充电线程（ChargerThread / charger）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 关机充电线程 / ChargerThread（charger 可执行程序） |
| 实体定义 | 关机充电模式的业务主体：epoll 主循环内轮询/事件驱动地读取电池 sysfs，驱动充电动画、LED、背光、震动，并处理电源键长按开机与温度/缺电保护；身份边界：不依赖 SystemAbility 层开机系统服务（samgr/CES/通知均未运行），硬件访问走 sysfs 与 HDI 驱动接口 |
| 核心特征 | 继承 `BatteryThread`（`charger/include/charger_thread.h:31-72`）；协作者 `provider_`（PowerSupplyProvider）、`vibrate_`、`backlight_`、`led_`、静态 `animation_`；状态 `capacity_`、`chargeState_`、`keyWait_`、`backlightWait_`、`lackPowerCapacity_`；输入监听 `ChargerThreadInputMonitor`（MMI） |
| 类型/分类 | 独立进程 `charger`（`ohos_executable`，`battery_manager_feature_enable_charger` 控制构建），由 init 在关机充电场景拉起 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | `main`（`charger/src/charger.cpp:23`）→ `StartThread` → 子线程 `LoopingThreadEntry`：epoll 监听 ueventFd（`/sys/class/power_supply` 变化）+ timerFd → 每轮 `CycleMatters` → `UpdateBatteryInfo`（provider 解析 temperature/capacity/chargeState）→ `HandleStates`（充电状态变化亮屏刷动画、电源键状态机、10s 无操作熄屏） |
| 状态流转 | 充电状态变化置 `isChargeStateChanged_` → 亮屏并 `UpdateAnimation`；容量 ≤ `lackPowerCapacity_`（`soc.shutdown`）分充电中（提示"电池电量低"）/未充电（提示"请连接电源"）两种动画；长按电源键 2s → 震动 75ms → `ClearPowerOffChargeFlag`（`libpower_charge_ext.z.so`）→ `DoReboot("")` 开机；温度越界或"有电未充电"（capacity > lackPower 且 DISABLE/NONE）→ `DoReboot("shutdown")` |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 背光超时 `BACKLIGHT_OFF_TIME_MS`=10000ms；长按 `REBOOT_TIME`=2000ms；`lackPowerCapacity_` 取 `soc.shutdown`（默认 1）；动画 UI 组件由 `charger/sa_profile/animation.json` 声明（UIImageView 62 帧循环 + UILabel） |
| 并发约束 | 单线程状态机（epoll 循环）；按键状态经全局 `g_keys[KEY_MAX+1]` 由 MMI 回调线程写入、循环线程读取（`SetKeyState` 去抖：同状态忽略） |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | init（拉起与 `DoReboot`）、MMI input（仅电源键消费）、ui_lite/graphic 栈、`charger/src/dev/`（DRM/fbdev 显示适配） |
| 下游影响 | 电池 sysfs 只读（`PowerSupplyProvider`）；LED/背光经 HDI（`charger/src/battery_led.cpp` 用 `ILightInterface`、`battery_backlight.cpp` 用 `V1_2::IDisplayComposerInterface`，背光亮度常量 128）；震动经 sysfs（`/sys/class/leds/vibrator/*`） |
| 平级关联 | 与开机态 `BatteryService` 互斥：同一份 `BatteryConfig` 解析逻辑（`charger/src/battery_config.cpp` 独立编译副本）但进程不同；`BatteryLed`/`BatteryBacklight` 对应开机态 `BatteryLight`/显示服务 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `ChargerThread`、`ChargerThreadInputMonitor`（`charger/include/charger_thread.h`）；基类 `BatteryThread`（`charger/include/battery_thread.h`）；数据源 `PowerSupplyProvider`（`charger/include/power_supply_provider.h`） |
| 核心方法 | `Run`/`Init`、`CycleMatters`、`UpdateBatteryInfo`、`HandleStates`/`HandleChargingState`/`HandleScreenState`/`HandlePowerKeyState`/`HandlePowerKey`、`UpdateAnimation`、`HandleTemperature`/`HandleCapacity`、`InitLackPowerCapacity`/`InitBatteryFileSystem` |
