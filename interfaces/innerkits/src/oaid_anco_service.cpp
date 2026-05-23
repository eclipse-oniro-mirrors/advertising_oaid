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

#include "oaid_anco_service.h"
#include "oaid_common.h"
#include "oaid_service_client.h"

namespace OHOS {
namespace Cloud {

bool AncoService::SetAncoSwitchStatus(int32_t userId, const std::string& bundleName,
    const std::string& uid, int32_t status)
{
    if (userId < 0 || bundleName.empty() || uid.empty()) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Invalid parameter: userId/bundleName/uid invalid");
        return false;
    }

    if (status != SWITCH_ON && status != SWITCH_OFF) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Invalid parameter: status must be 0 or 1");
        return false;
    }

    OAID_HILOGI(OAID_MODULE_SERVICE, "Client SetAncoSwitchStatus called");

    return Cloud::OAIDServiceClient::GetInstance()->SetAncoSwitchStatus(userId, bundleName, uid, status);
}

std::vector<AncoSwitchStatusInfo> AncoService::GetAncoSwitchStatus(int32_t userId,
    const std::string& bundleName, const std::string& uid)
{
    if (userId < 0) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Invalid parameter: userId cannot be negative");
        return {};
    }

    bool hasBundleName = !bundleName.empty();
    bool hasUid = !uid.empty();
    if (hasBundleName != hasUid) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Invalid parameter: bundleName and uid must be both present or both absent");
        return {};
    }

    OAID_HILOGI(OAID_MODULE_SERVICE, "GetAncoSwitchStatus called");

    return Cloud::OAIDServiceClient::GetInstance()->GetAncoSwitchStatus(userId, bundleName, uid);
}

std::vector<AncoAccessRecordInfo> AncoService::GetAncoAccessRecords(int32_t userId,
    const std::string& bundleName, const std::string& uid)
{
    if (userId < 0) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Invalid parameter: userId cannot be negative");
        return {};
    }

    bool hasBundleName = !bundleName.empty();
    bool hasUid = !uid.empty();
    if (hasBundleName != hasUid) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Invalid parameter: bundleName and uid must be both present or both absent");
        return {};
    }

    OAID_HILOGI(OAID_MODULE_SERVICE, "GetAncoAccessRecords called");

    return Cloud::OAIDServiceClient::GetInstance()->GetAncoAccessRecords(userId, bundleName, uid);
}

} // namespace Cloud
} // namespace OHOS