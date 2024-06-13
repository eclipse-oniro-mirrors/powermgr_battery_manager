/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#include "battery_dump_test.h"
#include <memory>

#define private   public
#define protected public
#include "battery_dump.h"
#include "battery_service.h"
#undef private
#undef protected
#include "power_common.h"

using namespace testing::ext;

namespace OHOS {
namespace PowerMgr {
namespace {
sptr<BatteryService> g_service;
}
void BatteryDumpTest::SetUpTestCase()
{
    g_service = DelayedSpSingleton<BatteryService>::GetInstance();
    g_service->isBootCompleted_ = true;
}

/**
 * @tc.name: BatteryDump001
 * @tc.desc: Dump parameter is -i, Get battery information
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
HWTEST_F(BatteryDumpTest, BatteryDump001, TestSize.Level1)
{
    int32_t fd = 1;
    std::vector<std::u16string> args;
    std::u16string arg = u"-i";
    args.push_back(arg);
    EXPECT_EQ(g_service->Dump(fd, args), ERR_OK);
}

/**
 * @tc.name: BatteryDump002
 * @tc.desc: Dump parameter is -u, MockUnplugged
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
HWTEST_F(BatteryDumpTest, BatteryDump002, TestSize.Level1)
{
    int32_t fd = 1;
    std::vector<std::u16string> args;
    std::u16string arg = u"-u";
    args.push_back(arg);
    EXPECT_EQ(g_service->Dump(fd, args), ERR_OK);
}

/**
 * @tc.name: BatteryDump003
 * @tc.desc: Dump parameter is -r, Reset
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
HWTEST_F(BatteryDumpTest, BatteryDump003, TestSize.Level1)
{
    int32_t fd = 1;
    std::vector<std::u16string> args;
    std::u16string arg = u"-r";
    args.push_back(arg);
    EXPECT_EQ(g_service->Dump(fd, args), ERR_OK);
}

/*
 * @tc.name: BatteryDump005
 * @tc.desc: Test functions Dump
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryDumpTest, BatteryDump005, TestSize.Level1)
{
    int32_t fd = 1;
    std::vector<std::u16string> args;
    std::u16string arg = u"-l";
    args.push_back(arg);
    EXPECT_EQ(g_service->Dump(fd, args), ERR_NO_INIT);
}

/**
 * @tc.name: BatteryDump006
 * @tc.desc: Test functions Dump
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryDumpTest, BatteryDump006, TestSize.Level1)
{
    int32_t fd = 1;
    std::vector<std::u16string> args;
    std::u16string arg = u"-ls";
    args.push_back(arg);
    EXPECT_EQ(g_service->Dump(fd, args), ERR_NO_INIT);
}

/**
 * @tc.name: BatteryDump007
 * @tc.desc: Test functions Dump
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
static HWTEST_F(BatteryDumpTest, BatteryDump007, TestSize.Level1)
{
    int32_t fd = 1;
    std::vector<std::u16string> args;
    EXPECT_EQ(g_service->Dump(fd, args), ERR_OK);
}

/**
 * @tc.name: BatteryDump008
 * @tc.desc: Dump parameter is empty, Get battery information
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
HWTEST_F(BatteryDumpTest, BatteryDump008, TestSize.Level1)
{
    BatteryDump& batteryDump = BatteryDump::GetInstance();
    int32_t fd = 1;
    std::vector<std::u16string> args;
    EXPECT_FALSE(batteryDump.GetBatteryInfo(fd, g_service, args));
}

/**
 * @tc.name: BatteryDump009
 * @tc.desc: Dump parameter is empty, MockUnplugged
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
HWTEST_F(BatteryDumpTest, BatteryDump009, TestSize.Level1)
{
    BatteryDump& batteryDump = BatteryDump::GetInstance();
    int32_t fd = 1;
    std::vector<std::u16string> args;
    EXPECT_FALSE(batteryDump.MockUnplugged(fd, g_service, args));
}

/**
 * @tc.name: BatteryDump010
 * @tc.desc: Dump parameter is empty, Reset
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
HWTEST_F(BatteryDumpTest, BatteryDump010, TestSize.Level1)
{
    BatteryDump& batteryDump = BatteryDump::GetInstance();
    int32_t fd = 1;
    std::vector<std::u16string> args;
    EXPECT_FALSE(batteryDump.Reset(fd, g_service, args));
}

/**
 * @tc.name: BatteryDump011
 * @tc.desc: Test functions Dump, capacity cmd normal
 * @tc.type: FUNC
 * @tc.require: issueI6Z8RB
 */
static HWTEST_F(BatteryDumpTest, BatteryDump011, TestSize.Level1)
{
    int32_t fd = 1;
    std::vector<std::u16string> args;
    std::u16string argParam = u"--capacity";
    std::u16string argCapacity = u"20";
    args.push_back(argParam);
    args.push_back(argCapacity);
    EXPECT_EQ(g_service->Dump(fd, args), ERR_OK);
}

/**
 * @tc.name: BatteryDump012
 * @tc.desc: Test functions Dump, capacity cmd invalid
 * @tc.type: FUNC
 * @tc.require: issueI6Z8RB
 */
static HWTEST_F(BatteryDumpTest, BatteryDump012, TestSize.Level1)
{
    int32_t fd = 1;
    std::vector<std::u16string> args;
    std::u16string arg = u"--capacity";
    args.push_back(arg);
    EXPECT_EQ(g_service->Dump(fd, args), ERR_NO_INIT);
}

/**
 * @tc.name: BatteryDump013
 * @tc.desc: Test functions Dump, capacity out of range
 * @tc.type: FUNC
 * @tc.require: issueI6Z8RB
 */
static HWTEST_F(BatteryDumpTest, BatteryDump013, TestSize.Level1)
{
    int32_t fd = 1;
    std::vector<std::u16string> args;
    std::u16string arg = u"--capacity 200";
    args.push_back(arg);
    EXPECT_EQ(g_service->Dump(fd, args), ERR_NO_INIT);
}

/**
 * @tc.name: BatteryDump014
 * @tc.desc: Dump parameter is empty, Capacity
 * @tc.type: FUNC
 * @tc.require: issueI5YZR1
 */
HWTEST_F(BatteryDumpTest, BatteryDump014, TestSize.Level1)
{
    BatteryDump& batteryDump = BatteryDump::GetInstance();
    int32_t fd = 1;
    std::vector<std::u16string> args;
    EXPECT_FALSE(batteryDump.MockCapacity(fd, g_service, args));
}

/**
 * @tc.name: BatteryDump015
 * @tc.desc: Dump parameter is empty, Uevent
 * @tc.type: FUNC
 */
HWTEST_F(BatteryDumpTest, BatteryDump015, TestSize.Level1)
{
    BatteryDump& batteryDump = BatteryDump::GetInstance();
    int32_t fd = 1;
    std::vector<std::u16string> args;
    EXPECT_FALSE(batteryDump.MockUevent(fd, g_service, args));
}

/**
 * @tc.name: BatteryDump016
 * @tc.desc: Test functions Dump, Uevent cmd normal
 * @tc.type: FUNC
 */
static HWTEST_F(BatteryDumpTest, BatteryDump016, TestSize.Level1)
{
    int32_t fd = 1;
    std::vector<std::u16string> args;
    std::u16string argParam = u"--uevent";
    std::u16string argCapacity = u"BATTERY_UNDER_VOLTAGE=3$sendcommonevent";
    args.push_back(argParam);
    args.push_back(argCapacity);
    EXPECT_EQ(g_service->Dump(fd, args), ERR_OK);
}

/**
 * @tc.name: BatteryDump017
 * @tc.desc: Test functions Dump, Uevent cmd invalid
 * @tc.type: FUNC
 */
static HWTEST_F(BatteryDumpTest, BatteryDump017, TestSize.Level1)
{
    int32_t fd = 1;
    std::vector<std::u16string> args;
    std::u16string arg = u"--uevent";
    args.push_back(arg);
    EXPECT_EQ(g_service->Dump(fd, args), ERR_NO_INIT);
}

} // namespace PowerMgr
} // namespace OHOS
