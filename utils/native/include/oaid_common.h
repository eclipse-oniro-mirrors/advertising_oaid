/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_CLOUD_OAID_COMMON_H
#define OHOS_CLOUD_OAID_COMMON_H
#include <map>
#include "oaid_hilog_wreapper.h"
#include "errors.h"

namespace OHOS {
namespace Cloud {
#define OAID_SERVICE_NAME "OAIDService"

enum OAIDError : int32_t {
    ERR_OK = 0,
    ERR_INVALID_PARAM = 401,
    ERR_SYSYTEM_ERROR = 17200001,
};

static std::map<int32_t, std::string> oaidErrCodeMsgMap = {
    {ERR_INVALID_PARAM, "Invalid input parameter."},
    {ERR_SYSYTEM_ERROR, "System internal error."}
};
}  // namespace Cloud
}  // namespace OHOS
#endif  // OHOS_CLOUD_OAID_COMMON_H