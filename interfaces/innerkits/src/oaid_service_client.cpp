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

#include "oaid_service_client.h"

#include <cinttypes>
#include <mutex>

#include "oaid_common.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "system_ability_load_callback_stub.h"

namespace OHOS {
namespace Cloud {
namespace {
static const int32_t OAID_SYSTME_ID = 6101; // The system component ID of the OAID is 6101.

/**
 * load time out: 10s
 */
static const int8_t LOAD_TIME_OUT = 10;
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

bool OAIDServiceClient::LoadServcie()
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
    OAID_HILOGI(OAID_MODULE_CLIENT, "Begin.");
    if (!LoadServcie()) {
        OAID_HILOGW(OAID_MODULE_CLIENT, "Redo load oaid service.");
        LoadServcie();
    }

    if (oaidServiceProxy_ == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Quit because redoing load oaid service failed.");
        return "";
    }

    auto oaid = oaidServiceProxy_->GetOAID();
    if (oaid == "") {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Get OAID failed.");
        return "";
    }
    OAID_HILOGI(OAID_MODULE_SERVICE, "Get OAID id %{public}zu.", oaid.size());
    return oaid;
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
    OAID_HILOGI(OAID_MODULE_CLIENT, "Load OAID service success.");
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
