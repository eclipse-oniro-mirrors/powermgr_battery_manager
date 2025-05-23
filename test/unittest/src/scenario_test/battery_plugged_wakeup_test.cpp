/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "battery_plugged_wakeup_test.h"

#include <csignal>
#include <iostream>

#include "battery_info.h"
#include "battery_service.h"
#include "power_mgr_client.h"
#include "test_utils.h"

using namespace testing::ext;
using namespace OHOS::PowerMgr;
using namespace OHOS::HDI::Battery;
using namespace OHOS;
using namespace std;

namespace {
bool g_isMock = false;
static sptr<BatteryService> g_service;
const std::string MOCK_BATTERY_PATH = "/data/service/el0/battery/";
} // namespace

void BatteryPluggedWakeupTest::SetUpTestCase()
{
    g_service = DelayedSpSingleton<BatteryService>::GetInstance();
    g_service->OnStart();
    g_isMock = TestUtils::IsMock();
    GTEST_LOG_(INFO) << " g_isMock: " << g_isMock;
}

void BatteryPluggedWakeupTest::TearDownTestCase()
{
    g_isMock = false;
    TestUtils::ResetOnline();
}

void BatteryPluggedWakeupTest::SetUp() {}

void BatteryPluggedWakeupTest::TearDown() {}

namespace {
/**
 * @tc.name: BatteryPluggedWakeupTest001
 * @tc.desc: Test function of wakeup_device when USB plugged.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryPluggedWakeupTest, BatteryPluggedWakeupTest001, TestSize.Level0)
{
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest001 function start!");
    if (g_isMock) {
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "0");
        g_service->Reset();
        auto& powerMgrClient = OHOS::PowerMgr::PowerMgrClient::GetInstance();
        powerMgrClient.SuspendDevice();
        bool isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_FALSE(isScreenOnNow);
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "1");
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/type", "USB");
        g_service->Reset();
        isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_TRUE(isScreenOnNow);
    }
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest001 function end!");
}

/**
 * @tc.name: BatteryPluggedWakeupTest002
 * @tc.desc: Test function of wakeup_device when USB unplugged.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryPluggedWakeupTest, BatteryPluggedWakeupTest002, TestSize.Level0)
{
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest002 function start!");
    if (g_isMock) {
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "1");
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/type", "USB");
        g_service->Reset();
        auto& powerMgrClient = OHOS::PowerMgr::PowerMgrClient::GetInstance();
        powerMgrClient.SuspendDevice();
        bool isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_FALSE(isScreenOnNow);
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "0");
        g_service->Reset();
        isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_TRUE(isScreenOnNow);
    }
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest002 function end!");
}

/**
 * @tc.name: BatteryPluggedWakeupTest003
 * @tc.desc: Test function of wakeup_device when WIRELESS plugged.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryPluggedWakeupTest, BatteryPluggedWakeupTest003, TestSize.Level0)
{
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest003 function start!");
    if (g_isMock) {
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "0");
        g_service->Reset();
        auto& powerMgrClient = OHOS::PowerMgr::PowerMgrClient::GetInstance();
        powerMgrClient.SuspendDevice();
        bool isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_FALSE(isScreenOnNow);
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "1");
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/type", "Wireless");
        g_service->Reset();
        isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_TRUE(isScreenOnNow);
    }
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest003 function end!");
}

/**
 * @tc.name: BatteryPluggedWakeupTest004
 * @tc.desc: Test function of wakeup_device when WIRELESS unplugged.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryPluggedWakeupTest, BatteryPluggedWakeupTest004, TestSize.Level0)
{
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest004 function start!");
    if (g_isMock) {
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "1");
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/type", "Wireless");
        g_service->Reset();
        auto& powerMgrClient = OHOS::PowerMgr::PowerMgrClient::GetInstance();
        powerMgrClient.SuspendDevice();
        bool isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_FALSE(isScreenOnNow);
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "0");
        g_service->Reset();
        isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_TRUE(isScreenOnNow);
    }
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest004 function end!");
}

/**
 * @tc.name: BatteryPluggedWakeupTest005
 * @tc.desc: Test function of wakeup_device when AC plugged.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryPluggedWakeupTest, BatteryPluggedWakeupTest005, TestSize.Level0)
{
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest005 function start!");
    if (g_isMock) {
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "0");
        g_service->Reset();
        auto& powerMgrClient = OHOS::PowerMgr::PowerMgrClient::GetInstance();
        powerMgrClient.SuspendDevice();
        bool isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_FALSE(isScreenOnNow);
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "1");
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/type", "Mains");
        g_service->Reset();
        isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_TRUE(isScreenOnNow);
    }
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest005 function end!");
}

/**
 * @tc.name: BatteryPluggedWakeupTest006
 * @tc.desc: Test function of wakeup_device when AC unplugged.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryPluggedWakeupTest, BatteryPluggedWakeupTest006, TestSize.Level0)
{
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest006 function start!");
    if (g_isMock) {
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "1");
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/type", "Mains");
        g_service->Reset();
        auto& powerMgrClient = OHOS::PowerMgr::PowerMgrClient::GetInstance();
        powerMgrClient.SuspendDevice();
        bool isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_FALSE(isScreenOnNow);
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "0");
        g_service->Reset();
        isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_TRUE(isScreenOnNow);
    }
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest006 function end!");
}

/**
 * @tc.name: BatteryPluggedWakeupTest007
 * @tc.desc: Test function of wakeup_device when Unknown plugged.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryPluggedWakeupTest, BatteryPluggedWakeupTest007, TestSize.Level0)
{
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest007 function start!");
    if (g_isMock) {
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "0");
        g_service->Reset();
        auto& powerMgrClient = OHOS::PowerMgr::PowerMgrClient::GetInstance();
        powerMgrClient.SuspendDevice();
        bool isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_FALSE(isScreenOnNow);
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "1");
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/type", "Unknown");
        g_service->Reset();
        isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_FALSE(isScreenOnNow);
    }
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest007 function end!");
}

/**
 * @tc.name: BatteryPluggedWakeupTest008
 * @tc.desc: Test function of wakeup_device when Unknown unplugged.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryPluggedWakeupTest, BatteryPluggedWakeupTest008, TestSize.Level0)
{
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest008 function start!");
    if (g_isMock) {
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "1");
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/type", "Unknown");
        g_service->Reset();
        auto& powerMgrClient = OHOS::PowerMgr::PowerMgrClient::GetInstance();
        powerMgrClient.SuspendDevice();
        bool isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_FALSE(isScreenOnNow);
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "0");
        g_service->Reset();
        isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_FALSE(isScreenOnNow);
    }
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest008 function end!");
}

/**
 * @tc.name: BatteryPluggedWakeupTest009
 * @tc.desc: the situation of PLUGGED_TYPE_NONE to PLUGGED_TYPE_NONE.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryPluggedWakeupTest, BatteryPluggedWakeupTest009, TestSize.Level0)
{
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest009 function start!");
    if (g_isMock) {
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "0");
        g_service->Reset();
        auto& powerMgrClient = OHOS::PowerMgr::PowerMgrClient::GetInstance();
        powerMgrClient.SuspendDevice();
        bool isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_FALSE(isScreenOnNow);
        TestUtils::WriteMock(MOCK_BATTERY_PATH + "/ohos_charger/online", "0");
        g_service->Reset();
        isScreenOnNow = powerMgrClient.IsScreenOn();
        EXPECT_FALSE(isScreenOnNow);
    }
    BATTERY_HILOGI(LABEL_TEST, "BatteryPluggedWakeupTest009 function end!");
}
}
