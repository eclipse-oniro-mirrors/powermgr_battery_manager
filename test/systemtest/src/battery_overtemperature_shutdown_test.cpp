/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "battery_overtemperature_shutdown_test.h"

#include <csignal>
#include <iostream>
#include "utils/hdf_log.h"

#include "battery_service.h"
#include "power_supply_provider.h"
#include "battery_thread_test.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::PowerMgr;
using namespace OHOS::HDI::Battery::V1_1;
using namespace std;

static sptr<BatteryService> g_service;

namespace BatteryOvertemperatureShutdownTest {
void BatteryOvertemperatureShutdownTest::SetUpTestCase(void)
{
    g_service = DelayedSpSingleton<BatteryService>::GetInstance();
    g_service->OnStart();
}

void BatteryOvertemperatureShutdownTest::TearDownTestCase(void)
{
    g_service->OnStop();
    DelayedSpSingleton<BatteryService>::DestroyInstance();
}

/**
 * @tc.name: BatteryST_003
 * @tc.desc: Test functions temperature in normal range in BatteryHostServiceStub
 * @tc.type: FUNC
 */
HWTEST_F (BatteryOvertemperatureShutdownTest, BatteryST_003, TestSize.Level1)
{
    HDF_LOGD("%{public}s: enter. BatteryST_003 start.", __func__);
    g_service->InitConfig();

    g_service->HandleTemperature(599);
    HDF_LOGD("%{public}s: enter. BatteryST_003 end.", __func__);
}

/**
 * @tc.name: BatteryST_004
 * @tc.desc: Test functions high temperature shutdown in BatteryHostServiceStub
 * @tc.type: FUNC
 */
HWTEST_F (BatteryOvertemperatureShutdownTest, BatteryST_004, TestSize.Level1)
{
    HDF_LOGD("%{public}s: enter. BatteryST_004 start.", __func__);
    g_service->InitConfig();
    if (false) {
        g_service->HandleTemperature(600);
    }
    HDF_LOGD("%{public}s: enter. BatteryST_004 end.", __func__);
}
}
