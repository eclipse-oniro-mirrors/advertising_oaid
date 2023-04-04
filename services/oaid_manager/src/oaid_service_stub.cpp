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
#include "oaid_service_stub.h"
#include "oaid_service_define.h"
#include <singleton.h>
#include "bundle_mgr_helper.h"
#include "oaid_common.h"
#include "bundle_mgr_client.h"
#include "oaid_service.h"
#include "accesstoken_kit.h"

using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Cloud {
using namespace OHOS::HiviewDFX;
pthread_mutex_t mutex_;
OAIDServiceStub::OAIDServiceStub()
{
    pthread_mutex_init(&mutex_, nullptr);
    memberFuncMap_[GET_OAID] = &OAIDServiceStub::OnGetOAID;
    memberFuncMap_[CLEAR_OAID] = &OAIDServiceStub::OnClearOAID;
    memberFuncMap_[HMS_GAIN_OAID] = &OAIDServiceStub::OnHmsGainOAID;
}

OAIDServiceStub::~OAIDServiceStub()
{
    memberFuncMap_.clear();
}

bool OAIDServiceStub::InitThread()
{
    pthread_mutex_lock(&mutex_);
    pthread_mutex_unlock(&mutex_);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int32_t ret = pthread_create(&oaidThreadId, &attr, StartThreadMain, this);
    if (ret != 0) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "pthread_create failed %{public}d", ret);
        pthread_mutex_lock(&mutex_);
        pthread_mutex_unlock(&mutex_);
        return false;
    }
    return true;
}

void *OAIDServiceStub::StartThreadMain(void *arg)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "StartThreadMain");
    OAIDServiceStub *oaidServiceStub = (OAIDServiceStub *)arg;
    oaidServiceStub->CtrlOAIDByAdsTrackingPermissionsState(SYSTEM_UERID);
    return nullptr;
}

bool OAIDServiceStub::CheckPermission(const std::string &permissionName)
{
    AccessTokenID callingToken = IPCSkeleton::GetCallingTokenID();
    ErrCode result = AccessTokenKit::VerifyAccessToken(callingToken, permissionName);
    if (result == TypePermissionState::PERMISSION_DENIED) {
        return false;
    }
    return true;
}

void OAIDServiceStub::CtrlOAIDByAdsTrackingPermissionsState(int32_t userId)
{
    std::vector<AppExecFwk::BundleInfo> bundleInfos;
    if (!DelayedSingleton<BundleMgrHelper>::GetInstance()->GetBundleInfosV9ByReqPermission(bundleInfos, userId) ||
        bundleInfos.size() <= 0) {
        OAID_HILOGW(OAID_MODULE_SERVICE, "GetBundleInfos fail,bundleInfos.size()=%{public}lu", bundleInfos.size());
        return;
    }

    // When some applications is granted permissions
    bool isSomeAppsGrantedPermissions = false;
    OAID_HILOGI(OAID_MODULE_SERVICE, "bundleInfos.size=%{public}lu", bundleInfos.size());
    for (auto it = bundleInfos.begin(); it != bundleInfos.end(); it++) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "it->bundleName=%{public}s.", it->name.c_str());
        if (it->versionCode < OHOS_API_VERSION) {
            continue;
        }
        AppExecFwk::ApplicationInfo applicationInfo;
        if (!DelayedSingleton<BundleMgrHelper>::GetInstance()->GetApplicationInfoV9WithPermission(
            it->name, userId, applicationInfo)) {
            OAID_HILOGW(OAID_MODULE_SERVICE, "GetApplicationInfo fail, bundleName=%{public}s", it->name.c_str());
            continue;
        }

        for (auto reqPermissionItor = it->reqPermissionDetails.begin();
            reqPermissionItor != it->reqPermissionDetails.end(); reqPermissionItor++) {
            std::string permissionName = reqPermissionItor->name;
            if (permissionName != OAID_TRACKING_CONSENT_PERMISSION) {
                continue;
            }

            ErrCode result = AccessTokenKit::VerifyAccessToken(applicationInfo.accessTokenId, permissionName);
            if (result == TypePermissionState::PERMISSION_DENIED) {
                OAID_HILOGW(OAID_MODULE_SERVICE, "VerifyAccessToken fail, bundleName=%{public}s", it->name.c_str());
                break;
            }
            isSomeAppsGrantedPermissions = true;
            break;
        }

        if (isSomeAppsGrantedPermissions) {
            OAID_HILOGI(OAID_MODULE_SERVICE, "Some apps are granted this permission.");
            break;
        }
    }

    if (!isSomeAppsGrantedPermissions) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "All apps are not granted this permission.");
        OAIDService::GetInstance()->ClearOAID();
    }

    return;
}

int32_t OAIDServiceStub::OnRemoteRequest(
    uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "Start, code is %{public}u.", code);
    std::string bundleName;
    pid_t uid = IPCSkeleton::GetCallingUid();
    DelayedSingleton<BundleMgrHelper>::GetInstance()->GetBundleNameByUid(static_cast<int>(uid), bundleName);
    if (!CheckPermission(OAID_TRACKING_CONSENT_PERMISSION)) {
        OAID_HILOGW(
            OAID_MODULE_SERVICE, "bundleName %{public}s not granted the app tracking permission", bundleName.c_str());
        bool resultExecThread = InitThread();
        OAID_HILOGI(
            OAID_MODULE_SERVICE, "resultExecThread = %{public}s", resultExecThread ? "success" : "failed");
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }

    std::u16string myDescripter = OAIDServiceStub::GetDescriptor();
    std::u16string remoteDescripter = data.ReadInterfaceToken();
    oaidServiceStubProxy_ = data.ReadRemoteObject();
    if (oaidServiceStubProxy_ == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "oaidServiceStubProxy is nullptr");
    } else {
        OAID_HILOGE(OAID_MODULE_SERVICE, "oaidServiceStubProxy is not nullptr");
    }

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

int32_t OAIDServiceStub::OnGetOAID(MessageParcel& data, MessageParcel& reply)
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

int32_t OAIDServiceStub::OnClearOAID(MessageParcel& data, MessageParcel& reply)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "Clear OAID Start.");

    OAIDService::GetInstance()->ClearOAID();

    OAID_HILOGI(OAID_MODULE_SERVICE, "Clear OAID End.");
    return ERR_OK;
}

int32_t OAIDServiceStub::OnHmsGainOAID(MessageParcel& data, MessageParcel& reply)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "Hms Gain OAID OAID Start.");

    auto oaid = OAIDService::GetInstance()->HmsGainOAID();
    if (oaid == "") {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Hms Gain OAID failed.");
        return ERR_SYSYTEM_ERROR;
    }
    if (!reply.WriteString16(to_utf16(oaid))) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "OnHmsGainOAID Failed to write parcelable.");
        return ERR_SYSYTEM_ERROR;
    }
    OAID_HILOGI(OAID_MODULE_SERVICE, "Hms Gain OAID End.");
    return ERR_OK;
}

sptr<IRemoteObject> OAIDServiceStub::GainOAIDServiceStubProxy()
{
    return oaidServiceStubProxy_;
}

} // namespace Cloud
} // namespace OHOS