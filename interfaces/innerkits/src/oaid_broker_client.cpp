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

#include <future>
#include <chrono>
#include "oaid_broker_client.h"
#include "oaid_service_client.h"
#include "os_account_manager.h"

namespace OHOS {
namespace Cloud {
namespace {
    static const std::string OAID_ALLZERO_STR = "00000000-0000-0000-0000-000000000000";
    static const std::string VALID_UID_SUFFIX = "10078";
    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 10000; // 10秒超时
}

std::vector<bool> OAIDBrokerClient::RequestAuthorization(const std::string packageName, const std::string uid)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "RequestAuthorization packageName = %{public}s uid = %{public}s",
        packageName.c_str(), uid.c_str());
    int32_t userId = GetUserId();
    bool globalSwitch = GetGlobalSwitch(userId);
    std::vector<AncoSwitchStatusInfo> appSwitch;
    auto future = std::async(std::launch::async, [&]() {
        return Cloud::OAIDServiceClient::GetInstance()->GetAncoSwitchStatus(userId, packageName, uid);
    });
    if (future.wait_for(std::chrono::milliseconds(DEFAULT_TIMEOUT_MS)) == std::future_status::timeout) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "RequestAuthorization GetAncoSwitchStatus timeout");
        return {};
    } else {
        appSwitch = future.get();
    }
    if (!appSwitch.empty()) {
    int32_t statVal = appSwitch[0].status;
    OAID_HILOGI(OAID_MODULE_SERVICE, "RequestAuthorization appSwitch appSwitch[0].status = %{public}d", statVal);
        return {globalSwitch, statVal, false};
    }
    OAID_HILOGI(OAID_MODULE_SERVICE, "RequestAuthorization appSwitch empty");
    return {globalSwitch, false, true};
}

bool OAIDBrokerClient::WriteAuthorization(const std::string packageName, const std::string uid, bool status)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "WriteAuthorization packageName = %{public}s uid = %{public}s status = %{public}d",
        packageName.c_str(), uid.c_str(), status);
    int32_t userId = GetUserId();
    bool result = false;
    auto future = std::async(std::launch::async, [&]() {
        return Cloud::OAIDServiceClient::GetInstance()->SetAncoSwitchStatus(userId, packageName, uid, status);
    });
    if (future.wait_for(std::chrono::milliseconds(DEFAULT_TIMEOUT_MS)) == std::future_status::timeout) {
        return false;
        OAID_HILOGE(OAID_MODULE_SERVICE, "WriteAuthorization SetAncoSwitchStatus timeout");
    } else {
        result = future.get();
    }
    return result;
}

std::string OAIDBrokerClient::GetAncoOaid(const std::string packageName, const std::string uid, bool flag)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "GetAncoOaid packageName = %{public}s uid = %{public}s flag = %{public}d",
        packageName.c_str(), uid.c_str(), flag);
    int32_t userId = GetUserId();
    bool globalSwitch = GetGlobalSwitch(userId);
    if (!flag) {
        std::vector<AncoSwitchStatusInfo> appSwitch;
        auto futureSwitch = std::async(std::launch::async, [&]() {
            return Cloud::OAIDServiceClient::GetInstance()->GetAncoSwitchStatus(userId, packageName, uid);
        });
        if (futureSwitch.wait_for(std::chrono::milliseconds(DEFAULT_TIMEOUT_MS)) == std::future_status::timeout) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "GetAncoOaid GetAncoSwitchStatus timeout");
            return OAID_ALLZERO_STR;
        } else {
            appSwitch = futureSwitch.get();
        }
        if (globalSwitch) {
            if (appSwitch.empty() || appSwitch[0].status) {
                OAID_HILOGI(OAID_MODULE_SERVICE, "GetAncoOaid enter globalSwitch = true return 0");
                return OAID_ALLZERO_STR;
            }
        } else {
            if (!appSwitch.empty() && appSwitch[0].status) {
                OAID_HILOGI(OAID_MODULE_SERVICE, "GetAncoOaid enter globalSwitch = false return 0");
                return OAID_ALLZERO_STR;
            }
        }
    }
    std::string val = OAID_ALLZERO_STR;
    auto futureOaid = std::async(std::launch::async, [&]() {
        return Cloud::OAIDServiceClient::GetInstance()->GetAncoOAID();
    });
    if (futureOaid.wait_for(std::chrono::milliseconds(DEFAULT_TIMEOUT_MS)) == std::future_status::timeout) {
        OAID_HILOGW(OAID_MODULE_SERVICE, "GetAncoOaid timeout, return allzero");
        return OAID_ALLZERO_STR;
    } else {
        val = futureOaid.get();
    }
    bool temp = val != OAID_ALLZERO_STR && !flag;
    OAID_HILOGI(OAID_MODULE_SERVICE, "GetAncoOaid enter end temp = %{public}d flag = %{public}d", temp, flag);
    if (val != OAID_ALLZERO_STR && !flag) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "GetAncoOaid enter insert Record");
        Cloud::OAIDServiceClient::GetInstance()->InsertAccessRecord(userId, packageName, uid);
    }
    return val;
}

bool OAIDBrokerClient::GetGlobalSwitch(const int32_t userId)
{
    uint32_t status;
    std::int32_t ret = Security::AccessToken::AccessTokenKit::GetPermissionRequestToggleStatus(
        "ohos.permission.APP_TRACKING_CONSENT", status, userId);
    OAID_HILOGI(OAID_MODULE_SERVICE, "GetGlobalSwitch ret=%{public}d status=%{public}d", ret, status);
    return status;
}

int32_t OAIDBrokerClient::GetUserId()
{
    //  获取当前userId
    std::int32_t userId;
    const std::int32_t finalUserId = 100;
    std::int32_t ret = AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(userId);
    OAID_HILOGI(OAID_MODULE_SERVICE, "Get AccountSA UserId ret=%{public}d", ret);
    if (ret != 0) {
        userId = finalUserId;
    }
    OAID_HILOGI(OAID_MODULE_SERVICE, "userId=%{public}d", userId);
    return userId;
}
} // namespace Cloud
} // namespace OHOS