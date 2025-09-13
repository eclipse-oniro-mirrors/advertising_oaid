/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "oaid_service_client.h"

#include <cinttypes>
#include <mutex>

#include "oaid_common.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "system_ability_load_callback_stub.h"

using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Cloud {
namespace {
static const int32_t OAID_SYSTME_ID = 6101; // The system component ID of the OAID is 6101.

static const std::string OAID_ALLZERO_STR = "00000000-0000-0000-0000-000000000000";

static const std::string OAID_TRACKING_CONSENT_PERMISSION = "ohos.permission.APP_TRACKING_CONSENT";

/**
 * load time out: 10s
 */
static const int8_t LOAD_TIME_OUT = 10;

static const int8_t RESET_OAID_DEFAULT_CODE = 0;
} // namespace

std::mutex OAIDServiceClient::instanceLock_;
sptr<OAIDServiceClient> OAIDServiceClient::instance_;

class OAIDServiceLoadCallback : public SystemAbilityLoadCallbackStub {
public:
    void OnLoadSystemAbilitySuccess(int32_t systemAbilityId, const sptr<IRemoteObject>& remoteObject) override
    {
        if (systemAbilityId != OAID_SYSTME_ID) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "Start systemAbility is not oaid service: %{public}d.", systemAbilityId);
            return;
        }

        OAIDServiceClient::GetInstance()->LoadServerSuccess(remoteObject);
    }

    void OnLoadSystemAbilityFail(int32_t systemAbilityId) override
    {
        if (systemAbilityId != OAID_SYSTME_ID) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "Start systemAbility is not oaid service: %{public}d.", systemAbilityId);
            return;
        }

        OAIDServiceClient::GetInstance()->LoadServerFail();
    }
};

OAIDServiceClient::OAIDServiceClient()
{
    deathRecipient_ = new (std::nothrow) OAIDSaDeathRecipient();
}

OAIDServiceClient::~OAIDServiceClient()
{
    if (oaidServiceProxy_ != nullptr) {
        auto remoteObject = oaidServiceProxy_->AsObject();
        if (remoteObject != nullptr && deathRecipient_ != nullptr) {
            remoteObject->RemoveDeathRecipient(deathRecipient_);
        }
    }
}

sptr<OAIDServiceClient> OAIDServiceClient::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> autoLock(instanceLock_);
        if (instance_ == nullptr) {
            instance_ = new OAIDServiceClient;
        }
    }
    return instance_;
}

bool OAIDServiceClient::LoadService()
{
    if (loadServiceReady_) {
        return true;
    }

    std::lock_guard<std::mutex> lock(loadServiceLock_);
    if (loadServiceReady_) {
        return true;
    }

    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Getting SystemAbilityManager failed.");
        return false;
    }

    sptr<OAIDServiceLoadCallback> loadCallback = new (std::nothrow) OAIDServiceLoadCallback();
    if (loadCallback == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "New  OAIDServiceLoadCallback failed.");
        return false;
    }

    int32_t result = systemAbilityManager->LoadSystemAbility(OAID_SYSTME_ID, loadCallback);
    if (result != ERR_OK) {
        OAID_HILOGE(
            OAID_MODULE_CLIENT, "LoadSystemAbility %{public}d failed, result: %{public}d.", OAID_SYSTME_ID, result);
        return false;
    }

    std::unique_lock<std::mutex> conditionLock(loadServiceConditionLock_);
    auto waitStatus = loadServiceCondition_.wait_for(
        conditionLock, std::chrono::seconds(LOAD_TIME_OUT), [this]() { return loadServiceReady_; });
    if (!waitStatus) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "LoadSystemAbility timeout.");
        return false;
    }

    return true;
}

std::string OAIDServiceClient::GetOAID()
{
    if (!CheckPermission(OAID_TRACKING_CONSENT_PERMISSION)) {
        OAID_HILOGW(
            OAID_MODULE_SERVICE, "get oaid not granted the app tracking permission");
        return OAID_ALLZERO_STR;
    }

    if (!LoadService()) {
        OAID_HILOGW(OAID_MODULE_CLIENT, "Redo load oaid service.");
        LoadService();
    }

    std::unique_lock<std::mutex> lock(getOaidProxyMutex_);
    if (oaidServiceProxy_ == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Quit because redoing load oaid service failed.");
        return OAID_ALLZERO_STR;
    }

    auto oaid = oaidServiceProxy_->GetOAID();
    if (oaid == "") {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Get OAID failed.");
        return OAID_ALLZERO_STR;
    }
    return oaid;
}

int32_t OAIDServiceClient::ResetOAID()
{
    if (!LoadService()) {
        OAID_HILOGW(OAID_MODULE_CLIENT, "Redo load oaid service.");
        LoadService();
    }
    std::unique_lock<std::mutex> lock(getOaidProxyMutex_);
    if (oaidServiceProxy_ == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Quit because redoing load oaid service failed.");
        return RESET_OAID_DEFAULT_CODE;
    }

    int32_t resetResult = oaidServiceProxy_->ResetOAID();
    OAID_HILOGI(OAID_MODULE_SERVICE, "End.resetResult = %{public}d", resetResult);

    return resetResult;
}

bool OAIDServiceClient::CheckPermission(const std::string &permissionName)
{
    // Verify the invoker's permission.
    AccessTokenID callingToken = IPCSkeleton::GetCallingTokenID();
    ATokenTypeEnum callingType = AccessTokenKit::GetTokenTypeFlag(callingToken);
    OAID_HILOGD(OAID_MODULE_SERVICE, "callingToken = %{public}u", callingToken);
    ErrCode result = TypePermissionState::PERMISSION_DENIED;
    if (callingType == TOKEN_INVALID) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "callingToken is invalid");
        return false;
    } else {
        result = AccessTokenKit::VerifyAccessToken(callingToken, permissionName);
    }
    if (result == TypePermissionState::PERMISSION_DENIED) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "the caller not granted the app tracking permission");
        return false;
    }
    return true;
}

int32_t OAIDServiceClient::RegisterObserver(const sptr<IRemoteConfigObserver>& observer)
{
    if (!LoadService()) {
        OAID_HILOGW(OAID_MODULE_CLIENT, "Redo load oaid service.");
        LoadService();
    }

    if (oaidServiceProxy_ == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Quit because redoing load oaid service failed.");
        return RESET_OAID_DEFAULT_CODE;
    }

    int32_t resetResult = oaidServiceProxy_->RegisterObserver(observer);
    OAID_HILOGI(OAID_MODULE_SERVICE, "End.resetResult = %{public}d", resetResult);

    return resetResult;
}

void OAIDServiceClient::OnRemoteSaDied(const wptr<IRemoteObject>& remote)
{
    OAID_HILOGE(OAID_MODULE_CLIENT, "OnRemoteSaDied");
    std::unique_lock<std::mutex> lock(loadServiceConditionLock_);
    if (oaidServiceProxy_ != nullptr) {
        auto remoteObject = oaidServiceProxy_->AsObject();
        if (remoteObject != nullptr && deathRecipient_ != nullptr) {
            remoteObject->RemoveDeathRecipient(deathRecipient_);
        }
        oaidServiceProxy_ = nullptr;
    }
    loadServiceReady_ = false;
}

void OAIDServiceClient::LoadServerSuccess(const sptr<IRemoteObject>& remoteObject)
{
    std::unique_lock<std::mutex> lock(loadServiceConditionLock_);
    if (remoteObject == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Load OAID service is null.");
        return;
    }

    if (deathRecipient_ != nullptr) {
        remoteObject->AddDeathRecipient(deathRecipient_);
    }
    oaidServiceProxy_ = iface_cast<IOAIDService>(remoteObject);
    loadServiceReady_ = true;
    loadServiceCondition_.notify_one();
}

void OAIDServiceClient::LoadServerFail()
{
    std::unique_lock<std::mutex> lock(loadServiceConditionLock_);
    loadServiceReady_ = false;
    OAID_HILOGE(OAID_MODULE_CLIENT, "Load OAID service fail.");
}

OAIDSaDeathRecipient::OAIDSaDeathRecipient() {}

void OAIDSaDeathRecipient::OnRemoteDied(const wptr<IRemoteObject>& object)
{
    OAID_HILOGW(OAID_MODULE_CLIENT, "Remote systemAbility died.");
    OAIDServiceClient::GetInstance()->OnRemoteSaDied(object);
}
} // namespace Cloud
} // namespace OHOS
