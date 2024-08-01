/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "oaid_service_stub.h"
#include <singleton.h>
#include "bundle_mgr_helper.h"
#include "bundle_mgr_client.h"
#include "accesstoken_kit.h"
#include "privacy_kit.h"
#include "tokenid_kit.h"
#include "oaid_common.h"
#include "oaid_service_define.h"
#include "oaid_service.h"
#include "oaid_service_ipc_interface_code.h"
#include "config_policy_utils.h"
#include "iservice_registry.h"
#include "oaid_remote_config_observer_stub.h"
#include "oaid_remote_config_observer_proxy.h"
#include "oaid_observer_manager.h"
#include "iservice_registry.h"

using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Cloud {
using namespace OHOS::HiviewDFX;
OAIDServiceStub::OAIDServiceStub()
{}

OAIDServiceStub::~OAIDServiceStub()
{}

bool OAIDServiceStub::CheckPermission(const std::string &permissionName)
{
    // Verify the invoker's permission.
    AccessTokenID firstCallToken = IPCSkeleton::GetFirstTokenID();
    AccessTokenID callingToken = IPCSkeleton::GetCallingTokenID();
    ATokenTypeEnum callingType = AccessTokenKit::GetTokenTypeFlag(callingToken);

    ErrCode result = TypePermissionState::PERMISSION_DENIED;

    if (firstCallToken == 0) {
        if (callingType == TOKEN_INVALID) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "callingToken is invalid");
            return false;
        } else {
            result = AccessTokenKit::VerifyAccessToken(callingToken, permissionName);
        }
    } else {
        result = AccessTokenKit::VerifyAccessToken(callingToken, firstCallToken, permissionName);
    }

    if (callingType == TOKEN_HAP) {
        int32_t successCnt = (int32_t)(result == TypePermissionState::PERMISSION_GRANTED);
        int32_t failCnt = 1 - successCnt;  // 1 means that there is only one visit in total
        // AddPermissionUsedRecord needs to transfer both the number of successful and failed permission access requests
        int32_t ret = PrivacyKit::AddPermissionUsedRecord(callingToken, permissionName, successCnt, failCnt);
        OAID_HILOGI(OAID_MODULE_SERVICE, "AddPermissionUsedRecord ret=%{public}d", ret);
    }

    if (result == TypePermissionState::PERMISSION_DENIED) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "the caller not granted the app tracking permission");
        return false;
    }
    return true;
}

bool OAIDServiceStub::CheckSystemApp()
{
    FullTokenID callingFullToken = IPCSkeleton::GetCallingFullTokenID();
    auto tokenType = AccessTokenKit::GetTokenTypeFlag(IPCSkeleton::GetCallingTokenID());
    if (TokenIdKit::IsSystemAppByFullTokenID(callingFullToken) && tokenType == TOKEN_HAP) {
        return true;
    }
    OAID_HILOGW(OAID_MODULE_SERVICE, "the caller App is not system app");
    return false;
}

bool LoadAndCheckOaidTrustList(const std::string &bundleName)
{
    char pathBuff[MAX_PATH_LEN];
    GetOneCfgFile(OAID_TRUSTLIST_EXTENSION_CONFIG_PATH.c_str(), pathBuff, MAX_PATH_LEN);
    char realPath[PATH_MAX];
    if (realpath(pathBuff, realPath) == nullptr) {
        GetOneCfgFile(OAID_TRUSTLIST_CONFIG_PATH.c_str(), pathBuff, MAX_PATH_LEN);
        if (realpath(pathBuff, realPath) == nullptr) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "Parse realpath fail");
            return false;
        }
    }
    std::ifstream inFile(realPath, std::ios::in);
    if (!inFile.is_open()) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Open file error.");
        return false;
    }
    std::string fileContent((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    cJSON *root = cJSON_Parse(fileContent.c_str());
    inFile.close();
    if (root == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "ParseJsonFromFile is not in JSON format.");
        return false;
    }
    cJSON *oaidTrustConfig = cJSON_GetObjectItem(root, "resetOAIDBundleName");
    if (oaidTrustConfig == nullptr || !cJSON_IsArray(oaidTrustConfig)) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "not contain resetOAIDBundleName node.");
        cJSON_Delete(root);
        return false;
    }
    int arraySize = cJSON_GetArraySize(oaidTrustConfig);
    if (arraySize == 0) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "oaidTrustConfig list is empty.");
        cJSON_Delete(root);
        return true;
    }
    for (int i = 0; i < arraySize; i++) {
        cJSON *item = cJSON_GetArrayItem(oaidTrustConfig, i);
        if (cJSON_IsString(item)) {
            if (bundleName.compare(item->valuestring) == 0) {
                OAID_HILOGI(OAID_MODULE_SERVICE, "the oaidWhiteList contains this bundle name");
                cJSON_Delete(root);
                return true;
            }
        }
    }
    cJSON_Delete(root);
    return false;
}

int32_t OAIDServiceStub::SendCode(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    switch (code) {
        case static_cast<uint32_t>(OAIDInterfaceCode::GET_OAID): {
            return OAIDServiceStub::OnGetOAID(data, reply);
            break;
        }
        case static_cast<uint32_t>(OAIDInterfaceCode::RESET_OAID): {
            return OAIDServiceStub::OnResetOAID(data, reply);
            break;
        }
        case static_cast<uint32_t>(OAIDInterfaceCode::REGISTER_CONTROL_CONFIG_OBSERVER): {
            return OAIDServiceStub::HandleRegisterControlConfigObserver(data, reply);
            break;
        }
    }
    return ERR_SYSYTEM_ERROR;
}

int32_t OAIDServiceStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    ExitIdleState();
    PostDelayUnloadTask();
    OAID_HILOGI(OAID_MODULE_SERVICE, "Start, code is %{public}u.", code);
    std::string bundleName;
    pid_t uid = IPCSkeleton::GetCallingUid();
    DelayedSingleton<BundleMgrHelper>::GetInstance()->GetBundleNameByUid(static_cast<int>(uid), bundleName);
    if (code == static_cast<uint32_t>(OAIDInterfaceCode::GET_OAID) &&
        !CheckPermission(OAID_TRACKING_CONSENT_PERMISSION)) {
        OAID_HILOGW(
            OAID_MODULE_SERVICE, "bundleName %{public}s not granted the app tracking permission", bundleName.c_str());
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    if (code == static_cast<uint32_t>(OAIDInterfaceCode::RESET_OAID)) {
        if (!LoadAndCheckOaidTrustList(bundleName)) {
            OAID_HILOGW(
                OAID_MODULE_SERVICE, "CheckOaidTrustList fail.errorCode = %{public}d", OAID_ERROR_NOT_IN_TRUST_LIST);
            if (!reply.WriteInt32(OAID_ERROR_NOT_IN_TRUST_LIST)) {
                OAID_HILOGE(OAID_MODULE_SERVICE, "write errorCode to reply failed.");
                return ERR_SYSYTEM_ERROR;
            }
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
        if (!CheckSystemApp()) {
            OAID_HILOGW(
                OAID_MODULE_SERVICE, "CheckSystemApp fail.errorCode = %{public}d", OAID_ERROR_CODE_NOT_SYSTEM_APP);
            if (!reply.WriteInt32(OAID_ERROR_CODE_NOT_SYSTEM_APP)) {
                OAID_HILOGE(OAID_MODULE_SERVICE, "write errorCode to reply failed.");
                return ERR_SYSYTEM_ERROR;
            }
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
    std::u16string myDescripter = OAIDServiceStub::GetDescriptor();
    std::u16string remoteDescripter = data.ReadInterfaceToken();
    if (myDescripter != remoteDescripter) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Descriptor checked fail.");
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    OAID_HILOGI(OAID_MODULE_SERVICE, "Remote bundleName is %{public}s.", bundleName.c_str());
    return SendCode(code, data, reply);
}

int32_t OAIDServiceStub::OnGetOAID(MessageParcel &data, MessageParcel &reply)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "Start.");

    auto oaid = GetOAID();
    if (oaid == "") {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Get OAID failed.");
        return ERR_SYSYTEM_ERROR;
    }

    if (!reply.WriteString(oaid)) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to write parcelable.");
        return ERR_SYSYTEM_ERROR;
    }
    OAID_HILOGI(OAID_MODULE_SERVICE, "End.");
    return ERR_OK;
}

int32_t OAIDServiceStub::OnResetOAID(MessageParcel &data, MessageParcel &reply)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "Reset OAID Start.");

    ResetOAID();

    OAID_HILOGI(OAID_MODULE_SERVICE, "Reset OAID End.");
    return ERR_OK;
}

void OAIDServiceStub::ExitIdleState()
{
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Get samgr failed.");
        return;
    }
    int32_t ret = samgrProxy->CancelUnloadSystemAbility(OAID_SYSTME_ID);
    if (ret != ERR_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "CancelUnload system ability %{public}d failed, result: %{public}d.",
                    OAID_SYSTME_ID, ret);
        return;
    }
}

void OAIDServiceStub::PostDelayUnloadTask()
{
    if (unloadHandler_ == nullptr) {
        const char *runnerName = "unlock";
        auto runner = AppExecFwk::EventRunner::Create(runnerName);
        unloadHandler_ = std::make_shared<AppExecFwk::EventHandler>(runner);
    }
    auto task = [this]() {
        auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgrProxy == nullptr) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "Get samgr failed.");
            return;
        }
        int32_t ret = samgrProxy->UnloadSystemAbility(OAID_SYSTME_ID);
        if (ret != ERR_OK) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "Unload system ability %{public}d failed, result: %{public}d.",
                        OAID_SYSTME_ID, ret);
            return;
        }
    };
    unloadHandler_->RemoveTask(TASK_ID);
    unloadHandler_->PostTask(task, TASK_ID, DELAY_TIME);
}

int32_t OAIDServiceStub::HandleRegisterControlConfigObserver(MessageParcel &data, MessageParcel &reply)
{
    int32_t uid = IPCSkeleton::GetCallingUid();
    if (uid != HA_UID) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "callingUid error.");
        return ERR_INVALID_PARAM;
    }
    auto remoteObject = data.ReadRemoteObject();
    if (!remoteObject) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "remoteObject is null");
        return ERR_SYSYTEM_ERROR;
    }
    auto observer = iface_cast<IRemoteConfigObserver>(remoteObject);
    if (observer == nullptr) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "Null observer.");
        return ERR_SYSYTEM_ERROR;
    }
    return RegisterObserver(observer);
}

int32_t OAIDServiceStub::RegisterObserver(const sptr<IRemoteConfigObserver> &observer)
{
    return DelayedSingleton<OaidObserverManager>::GetInstance()->RegisterObserver(observer);
}
}  // namespace Cloud
}  // namespace OHOS