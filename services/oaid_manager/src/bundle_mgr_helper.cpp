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

#include "bundle_mgr_helper.h"
#include "oaid_common.h"
#include "bundle_constants.h"
#include "bundle_mgr_client.h"
#include "bundle_mgr_interface.h"
#include "bundle_mgr_proxy.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace Cloud {
BundleMgrHelper::BundleMgrHelper() : bundleMgrProxy_(nullptr), oaidDeathRecipient_(nullptr)
{}

BundleMgrHelper::~BundleMgrHelper()
{}

bool BundleMgrHelper::GetBundleMgrProxy()
{
    OAID_HILOGD(OAID_MODULE_SERVICE, "GetBundleMgrProxy");

    if (!bundleMgrProxy_) {
        sptr<ISystemAbilityManager> systemAbilityManager =
            SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (!systemAbilityManager) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to get system ability mgr.");
            return false;
        }

        sptr<IRemoteObject> remoteObject =
            systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
        if (!remoteObject) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to get bundle manager service.");
            return false;
        }

        bundleMgrProxy_ = iface_cast<IBundleMgr>(remoteObject);
        if ((!bundleMgrProxy_) || (!bundleMgrProxy_->AsObject())) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to get system bundle manager services ability");
            return false;
        }

        oaidDeathRecipient_ = new (std::nothrow) OAIDDeathRecipient();
        if (!oaidDeathRecipient_) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to create death Recipient ptr OAIDDeathRecipient");
            return false;
        }
        if (!bundleMgrProxy_->AsObject()->AddDeathRecipient(oaidDeathRecipient_)) {
            OAID_HILOGW(OAID_MODULE_SERVICE, "Failed to add death recipient");
        }
    }

    return true;
}

bool BundleMgrHelper::GetBundleInfosV9ByReqPermission(std::vector<AppExecFwk::BundleInfo> &bundleInfos, int32_t userId)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "GetBundleInfosV9ByReqPermission");
    std::lock_guard<std::mutex> lock(mutex_);

    if (!GetBundleMgrProxy()) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "get BundleMgr Proxy fail");
        return false;
    }

    int32_t flag = static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_REQUESTED_PERMISSION);
    ErrCode ret = bundleMgrProxy_->GetBundleInfosV9(flag, bundleInfos, userId);
    if (ret != ERR_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to getBundleInfos");
        return false;
    }

    return true;
}

bool BundleMgrHelper::GetApplicationInfoV9WithPermission(
    const std::string bundleName, int32_t userId, AppExecFwk::ApplicationInfo &applicationInfo)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "GetApplicationInfoV9ByReqPermission");
    std::lock_guard<std::mutex> lock(mutex_);

    if (!GetBundleMgrProxy()) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "get BundleMgr Proxy fail");
        return false;
    }

    int32_t applicationFlag =
        static_cast<int32_t>(AppExecFwk::GetApplicationFlag::GET_APPLICATION_INFO_WITH_PERMISSION);
    ErrCode ret = bundleMgrProxy_->GetApplicationInfoV9(bundleName, applicationFlag, userId, applicationInfo);
    if (ret != ERR_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to GetApplicationInfoV9");
        return false;
    }

    return true;
}

void BundleMgrHelper::GetBundleNameByUid(const int uid, std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!GetBundleMgrProxy()) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "get BundleMgr Proxy fail");
        return;
    }

    ErrCode ret = bundleMgrProxy_->GetNameForUid(uid, name);
    if (ret != ERR_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to GetNameForUid");
        return;
    }
    OAID_HILOGD(OAID_MODULE_SERVICE, "GetBundleNameByUid success");
    return;
}

void BundleMgrHelper::ClearBundleMgrHelper()
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "ClearBundleMgrHelper");
    std::lock_guard<std::mutex> lock(mutex_);

    if ((bundleMgrProxy_ != nullptr) && (bundleMgrProxy_->AsObject() != nullptr)) {
        bundleMgrProxy_->AsObject()->RemoveDeathRecipient(oaidDeathRecipient_);
    }
    bundleMgrProxy_ = nullptr;
}
}  // namespace Cloud
}  // namespace OHOS