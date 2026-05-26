/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_CLOUD_OAID_BROKER_CLIENT_H
#define OHOS_CLOUD_OAID_BROKER_CLIENT_H

#include <string>
#include <vector>
#include <mutex>

#include "oaid_anco_service.h"


namespace OHOS {
namespace Cloud {
class OAIDBrokerClient {
public:
    static std::vector<bool> RequestAuthorization(const std::string packageName, const std::string uid);
    static bool WriteAuthorization(const std::string packageName, const std::string uid, bool status);
    static std::string GetAncoOaid(const std::string packageName, const std::string uid, bool flag);
private:
    static bool GetGlobalSwitch(const int32_t userId);
    static int32_t GetUserId();
};

} // namespace Cloud
} // namespace OHOS

#endif // OHOS_CLOUD_OAID_BROKER_CLIENT_H