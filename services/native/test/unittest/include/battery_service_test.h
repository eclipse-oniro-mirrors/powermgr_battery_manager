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

#ifndef BATTERY_SERVICE_TEST_H
#define BATTERY_SERVICE_TEST_H

#include <gtest/gtest.h>

namespace OHOS {
namespace PowerMgr {
class BatteryServiceTest : public testing::Test {
public:
    int32_t ReadFile(const char* path, char* buf, size_t size);
    int32_t WriteFile(const char* path, char* buf, size_t size);
};
} // namespace PowerMgr
} // namespace OHOS
#endif // BATTERY_SERVICE_TEST_H
