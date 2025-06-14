/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef BATTERY_SRV_ERRORS_H
#define BATTERY_SRV_ERRORS_H

namespace OHOS {
namespace PowerMgr {
enum class BatteryError : int32_t {
    ERR_OK = 0,
    ERR_FAILURE = 1,
    ERR_PERMISSION_DENIED = 201,
    ERR_SYSTEM_API_DENIED = 202,
    ERR_PARAM_INVALID = 401,
    ERR_CONNECTION_FAIL = 5100101
};
} // namespace PowerMgr
} // namespace OHOS

#endif // BATTERY_SRV_ERRORS_H
