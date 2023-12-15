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


using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Cloud {
using namespace OHOS::HiviewDFX;
OAIDServiceStub::OAIDServiceStub()
{
    memberFuncMap_[static_cast<uint32_t>(OAIDInterfaceCode::GET_OAID)] = &OAIDServiceStub::OnGetOAID;
    memberFuncMap_[static_cast<uint32_t>(OAIDInterfaceCode::RESET_OAID)] = &OAIDServiceStub::OnResetOAID;
}

OAIDServiceStub::~OAIDServiceStub()
{
    memberFuncMap_.clear();
}

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
        int32_t failCnt = 1 - successCnt; // 1 means that there is only one visit in total
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

    std::ifstream ifs;
    ifs.open(realPath);
    if (!ifs) {
        OAID_HILOGW(OAID_MODULE_SERVICE, "Open file error.");
        ifs.close();
        return false;
    }

    Json::Value jsonValue;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = true;
    JSONCPP_STRING errs;
    if (!parseFromStream(builder, ifs, &jsonValue, &errs)) {
        ifs.close();
        OAID_HILOGW(OAID_MODULE_SERVICE, "Read file failed %{public}s.", errs.c_str());
        return false;
    }

    Json::Value oaidTrustConfig = jsonValue["resetOAIDBundleName"];
    if (oaidTrustConfig.size() == 0) {
        ifs.close();
        return true;
    }
    for (uint32_t i = 0; i < oaidTrustConfig.size(); i++) {
        std::string trustData = oaidTrustConfig[i].asString();
        uint32_t result = bundleName.compare(trustData);
        if (result == 0) {
            OAID_HILOGI(OAID_MODULE_SERVICE, "the oaidWhiteList contains this bundle name");
            ifs.close();
            return true;
        }
    }
    ifs.close();
    return false;
}

int32_t OAIDServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "Start, code is %{public}u.", code);
    std::string bundleName;
    pid_t uid = IPCSkeleton::GetCallingUid();
    DelayedSingleton<BundleMgrHelper>::GetInstance()->GetBundleNameByUid(static_cast<int>(uid), bundleName);
    if (code != static_cast<uint32_t>(OAIDInterfaceCode::RESET_OAID) &&
        !CheckPermission(OAID_TRACKING_CONSENT_PERMISSION)) {
        OAID_HILOGW(OAID_MODULE_SERVICE, "bundleName %{public}s not granted the app tracking permission",
            bundleName.c_str());
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }

    if (code == static_cast<uint32_t>(OAIDInterfaceCode::RESET_OAID)) {
        if (!LoadAndCheckOaidTrustList(bundleName)) {
            OAID_HILOGW(OAID_MODULE_SERVICE, "CheckOaidTrustList fail");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }

        if (!CheckSystemApp()) {
            OAID_HILOGW(OAID_MODULE_SERVICE, "CheckSystemApp fail");
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
    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }

    int32_t ret = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    OAID_HILOGE(OAID_MODULE_SERVICE, "No find process to handle, ret is %{public}d.", ret);
    return ret;
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
} // namespace Cloud
} // namespace OHOS