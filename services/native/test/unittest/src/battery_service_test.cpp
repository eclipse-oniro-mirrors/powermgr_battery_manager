/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "battery_service_test.h"

#ifdef GTEST
#define private   public
#define protected public
#endif

#include <fcntl.h>
#include <memory>
#include <string>

#include "battery_info.h"
#include "battery_log.h"
#include "battery_service.h"
#include "common_event_data.h"
#include "common_event_subscriber.h"
#include "common_event_support.h"
#include "securec.h"
#include "test_utils.h"

using namespace testing::ext;
using namespace OHOS::PowerMgr;
using namespace OHOS::EventFwk;
using namespace OHOS::HDI::Battery;
using namespace OHOS;
using namespace std;

namespace {
sptr<BatteryService> g_service;
bool g_isMock = TestUtils::IsMock();
const std::string MOCK_BATTERY_PATH = "/data/service/el0/battery/";
}

int32_t BatteryServiceTest::ReadFile(const char* path, char* buf, size_t size)
{
    int32_t fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        return -1;
    }

    size_t readSize = read(fd, buf, size);
    close(fd);
    return readSize;
}

int32_t BatteryServiceTest::WriteFile(const char* path, char* buf, size_t size)
{
    int32_t fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        return -1;
    }

    size_t writeSize = write(fd, buf, size);
    close(fd);
    return writeSize;
}

/**
 * @tc.name: BatteryService000
 * @tc.desc: Test functions DelayedSpSingleton GetInstance
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService000, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService000 start.");
    g_service = DelayedSpSingleton<BatteryService>::GetInstance();
    EXPECT_TRUE(g_service != nullptr);
    g_service->OnStart();
    g_service = nullptr;
    g_service = DelayedSpSingleton<BatteryService>::GetInstance();
    EXPECT_TRUE(g_service != nullptr);
    BATTERY_HILOGD(LABEL_TEST, "BatteryService000 end.");
}

/**
 * @tc.name: BatteryService001
 * @tc.desc: Test functions RegisterBatteryHdiCallback
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService001, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService001 start.");
    EXPECT_TRUE(g_service->RegisterBatteryHdiCallback());
    BATTERY_HILOGD(LABEL_TEST, "BatteryService001 end.");
}

/**
 * @tc.name: BatteryService003
 * @tc.desc: Test functions DestoryBatteryDialog
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService003, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService003 start.");
    EXPECT_TRUE(g_service->DestoryBatteryDialog());
    BATTERY_HILOGD(LABEL_TEST, "BatteryService003 end.");
}

/**
 * @tc.name: BatteryService004
 * @tc.desc: Test functions GetVoltage
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService004, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService004 start.");
    char buf[128];
    memset_s(buf, sizeof(buf), '\0', sizeof(buf));
    int32_t readSize = ReadFile("/data/service/el0/battery/battery/voltage_now", buf, sizeof(buf));
    EXPECT_TRUE(readSize > 0);

    int32_t voltage = g_service->GetVoltage();
    EXPECT_TRUE(voltage == std::stoi(buf));
    BATTERY_HILOGD(LABEL_TEST, "BatteryService004 end.");
}

/**
 * @tc.name: BatteryService005
 * @tc.desc: Test functions GetPresent
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService005, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService005 start.");
    char buf[128];
    int32_t readSize = ReadFile("/data/service/el0/battery/battery/present", buf, sizeof(buf));
    EXPECT_TRUE(readSize > 0);
    char present[] = "1";
    EXPECT_TRUE(WriteFile("/data/service/el0/battery/battery/present", present, sizeof(present)));
    EXPECT_TRUE(g_service->GetPresent());
    WriteFile("/data/service/el0/battery/battery/present", buf, readSize);
    BATTERY_HILOGD(LABEL_TEST, "BatteryService005 end.");
}

/**
 * @tc.name: BatteryService006
 * @tc.desc: Test functions GetBatteryTemperature
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService006, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService006 start.");

    char buf[128];
    memset_s(buf, sizeof(buf), '\0', sizeof(buf));
    int32_t readSize = ReadFile("/data/service/el0/battery/battery/temp", buf, sizeof(buf));
    EXPECT_TRUE(readSize > 0);

    BATTERY_HILOGD(LABEL_TEST, "Battery temperature is %{public}s", buf);
    int32_t temp = g_service->GetBatteryTemperature();
    EXPECT_TRUE(temp == std::stoi(buf));
    BATTERY_HILOGD(LABEL_TEST, "BatteryService006 end.");
}

/**
 * @tc.name: BatteryService012
 * @tc.desc: Test functions RegisterHdiStatusListener
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService007, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService007 start.");
    EXPECT_TRUE(g_service->RegisterHdiStatusListener());
    BATTERY_HILOGD(LABEL_TEST, "BatteryService007 end.");
}

/**
 * @tc.name: BatteryService008
 * @tc.desc: Test functions OnStart and OnStop and OnAddSystemAbility
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService008, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService008 start.");
    g_service->OnStart();
    g_service->OnStart();
    g_service->OnStop();
    g_service->OnStop();
    int32_t systemAbilityId = 3602; // MISCDEVICE_SERVICE_ABILITY_ID
    g_service->OnAddSystemAbility(systemAbilityId, std::string("BatteryService008"));
    g_service->OnAddSystemAbility(0, std::string("BatteryService008"));
    g_service->ready_ = true;
    g_service->OnStart();
    EXPECT_TRUE(g_service->ready_);
    g_service->OnStop();
    EXPECT_FALSE(g_service->ready_);

    g_service->OnStart();
    g_service->iBatteryInterface_ = V1_1::IBatteryInterface::Get();
    BATTERY_HILOGD(LABEL_TEST, "BatteryService008 end.");
}

/**
 * @tc.name: BatteryService009
 * @tc.desc: Test functions MockUnplugged, with parameter is false
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService009, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService009 start.");
    g_service->MockUnplugged(false);
    EXPECT_EQ(g_service->IsMockUnplugged(), false);
    BATTERY_HILOGD(LABEL_TEST, "BatteryService009 end.");
}

/**
 * @tc.name: BatteryService010
 * @tc.desc: Test functions GetRemainEnergy
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService010, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService010 start.");
    EXPECT_NE(g_service->GetRemainEnergy(), ERR_NO_INIT);
    BATTERY_HILOGD(LABEL_TEST, "BatteryService010 end.");
}

/**
 * @tc.name: BatteryService011
 * @tc.desc: Test functions GetNowCurrent
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService011, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService011 start.");
    EXPECT_NE(g_service->GetNowCurrent(), ERR_NO_INIT);
    BATTERY_HILOGD(LABEL_TEST, "BatteryService011 end.");
}

/**
 * @tc.name: BatteryService012
 * @tc.desc: Test functions GetTechnology
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService012, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService012 start.");
    EXPECT_FALSE(g_service->GetTechnology().empty());
    BATTERY_HILOGD(LABEL_TEST, "BatteryService012 end.");
}

/**
 * @tc.name: BatteryService013
 * @tc.desc: Test functions GetPluggedType
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService013, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService013 start.");
    BatteryPluggedType pluggedType = g_service->GetPluggedType();
    EXPECT_TRUE(
        pluggedType >= BatteryPluggedType::PLUGGED_TYPE_NONE && pluggedType <= BatteryPluggedType::PLUGGED_TYPE_BUTT);
    BATTERY_HILOGD(LABEL_TEST, "BatteryService013 end.");
}

/**
 * @tc.name: BatteryService014
 * @tc.desc: Test functions GetCurrentAverage
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService014, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService014 start.");
    EXPECT_NE(g_service->GetCurrentAverage(), ERR_NO_INIT);
    BATTERY_HILOGD(LABEL_TEST, "BatteryService014 end.");
}

/**
 * @tc.name: BatteryService015
 * @tc.desc: Test functions GetHealthStatus
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService015, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService015 start.");
    BatteryHealthState healthState = g_service->GetHealthStatus();
    EXPECT_TRUE(healthState >= BatteryHealthState::HEALTH_STATE_UNKNOWN &&
        healthState <= BatteryHealthState::HEALTH_STATE_BUTT);
    BATTERY_HILOGD(LABEL_TEST, "BatteryService015 end.");
}

/**
 * @tc.name: BatteryService016
 * @tc.desc: Test functions GetChargingStatus
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService016, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService016 start.");
    BatteryChargeState chargeState = g_service->GetChargingStatus();
    EXPECT_TRUE(
        chargeState >= BatteryChargeState::CHARGE_STATE_NONE && chargeState <= BatteryChargeState::CHARGE_STATE_BUTT);
    BATTERY_HILOGD(LABEL_TEST, "BatteryService016 end.");
}

/**
 * @tc.name: BatteryService017
 * @tc.desc: Test functions GetTotalEnergy
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryServiceTest, BatteryService017, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService017 start.");
    EXPECT_NE(g_service->GetTotalEnergy(), ERR_NO_INIT);
    BATTERY_HILOGD(LABEL_TEST, "BatteryService017 end.");
}

/**
 * @tc.name: BatteryService018
 * @tc.desc: Test functions HandleBatteryCallbackEvent
 * @tc.type: FUNC
 */
static HWTEST_F(BatteryServiceTest, BatteryService018, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService018 start.");
    V1_1::BatteryInfo event;
    event.capacity = 90; // Prevent shutdown
    EXPECT_EQ(g_service->HandleBatteryCallbackEvent(event), ERR_OK);

    int32_t fd = 1;
    std::vector<std::u16string> args;
    std::u16string arg = u"-u";
    args.push_back(arg);
    EXPECT_EQ(g_service->Dump(fd, args), ERR_OK);

    EXPECT_EQ(g_service->HandleBatteryCallbackEvent(event), ERR_OK);

    args.clear();
    arg = u"-r";
    args.push_back(arg);
    EXPECT_EQ(g_service->Dump(fd, args), ERR_OK);

    EXPECT_EQ(g_service->HandleBatteryCallbackEvent(event), ERR_OK);
    EXPECT_EQ(g_service->HandleBatteryCallbackEvent(event), ERR_OK);

    BATTERY_HILOGD(LABEL_TEST, "BatteryService018 end.");
}

/**
 * @tc.name: BatteryService019
 * @tc.desc: Test functions OnStop
 * @tc.type: FUNC
 */
static HWTEST_F(BatteryServiceTest, BatteryService019, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService019 start.");
    g_service->ready_ = true;
    g_service->OnStop();
    EXPECT_FALSE(g_service->ready_);

    g_service->ready_ = true;
    g_service->iBatteryInterface_ = nullptr;
    g_service->hdiServiceMgr_ = nullptr;
    g_service->OnStop();
    EXPECT_FALSE(g_service->ready_);
    g_service->OnStart();

    BATTERY_HILOGD(LABEL_TEST, "BatteryService019 end.");
}

/**
 * @tc.name: BatteryService020
 * @tc.desc: Test functions HandlePopupEvent
 * @tc.type: FUNC
 */
static HWTEST_F(BatteryServiceTest, BatteryService020, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService020 start.");

    g_service->isLowPower_ = false;
    int32_t capacity1 = g_service->warnCapacity_ - 1;
    g_service->HandlePopupEvent(capacity1);
    EXPECT_TRUE(g_service->isLowPower_);
    g_service->HandlePopupEvent(capacity1);
    EXPECT_TRUE(g_service->isLowPower_);

    g_service->isLowPower_ = true;
    int32_t capacity2 = g_service->warnCapacity_ + 1;
    g_service->HandlePopupEvent(capacity2);
    EXPECT_FALSE(g_service->isLowPower_);
    g_service->HandlePopupEvent(capacity2);
    EXPECT_FALSE(g_service->isLowPower_);

    BATTERY_HILOGD(LABEL_TEST, "BatteryService020 end.");
}

/**
 * @tc.name: BatteryService021
 * @tc.desc: Test functions ChangePath
 * @tc.type: FUNC
 */
static HWTEST_F(BatteryServiceTest, BatteryService021, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService021 start.");
    g_service->iBatteryInterface_ = nullptr;
    g_service->ChangePath("/data/service/el0/battery");

    g_service->iBatteryInterface_ = V1_1::IBatteryInterface::Get();
    g_service->ChangePath("/data/service/el0/battery");

    if (!g_isMock) {
        g_service->ChangePath("/sys/class/power_supply");
    }
    BATTERY_HILOGD(LABEL_TEST, "BatteryService021 end.");
}

/**
 * @tc.name: BatteryService022
 * @tc.desc: Test functions CalculateRemainingChargeTime
 * @tc.type: FUNC
 */
static HWTEST_F(BatteryServiceTest, BatteryService022, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService022 start.");
    g_service->CalculateRemainingChargeTime(101, BatteryChargeState::CHARGE_STATE_DISABLE);
    EXPECT_FALSE(g_service->chargeFlag_);

    g_service->chargeFlag_ = true;
    g_service->lastCapacity_ = 50;
    g_service->CalculateRemainingChargeTime(30, BatteryChargeState::CHARGE_STATE_ENABLE);
    EXPECT_EQ(g_service->lastCapacity_, 30);

    g_service->lastCapacity_ = 50;
    g_service->chargeFlag_ = true;
    g_service->CalculateRemainingChargeTime(51, BatteryChargeState::CHARGE_STATE_ENABLE);

        BATTERY_HILOGD(LABEL_TEST, "BatteryService022 end.");
}

/**
 * @tc.name: BatteryService023
 * @tc.desc: Test functions GetBatteryLevel
 * @tc.type: FUNC
 */
static HWTEST_F(BatteryServiceTest, BatteryService023, TestSize.Level1)
{
    BATTERY_HILOGD(LABEL_TEST, "BatteryService023 start.");
    if (g_isMock) {
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/battery/capacity", "4");
        auto level = g_service->GetBatteryLevel();
        EXPECT_EQ(level, BatteryLevel::LEVEL_CRITICAL);

        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/battery/capacity", "20");
        level = g_service->GetBatteryLevel();
        EXPECT_EQ(level, BatteryLevel::LEVEL_LOW);

        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/battery/capacity", "90");
        level = g_service->GetBatteryLevel();
        EXPECT_EQ(level, BatteryLevel::LEVEL_NORMAL);

        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/battery/capacity", "99");
        level = g_service->GetBatteryLevel();
        EXPECT_EQ(level, BatteryLevel::LEVEL_HIGH);

        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/battery/capacity", "100");
        level = g_service->GetBatteryLevel();
        EXPECT_EQ(level, BatteryLevel::LEVEL_FULL);

        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/battery/capacity", "200");
        level = g_service->GetBatteryLevel();
        EXPECT_EQ(level, BatteryLevel::LEVEL_NONE);

        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/battery/capacity", "50");
    }
    BATTERY_HILOGD(LABEL_TEST, "BatteryService023 end.");
}
