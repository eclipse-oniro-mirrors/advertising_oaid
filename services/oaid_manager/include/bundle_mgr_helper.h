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

#ifndef OHOS_CLOUD_BUNDLE_MGR_HELPER_H
#define OHOS_CLOUD_BUNDLE_MGR_HELPER_H

#include <string>
#include <vector>

#include "bundle_mgr_interface.h"
#include "iremote_object.h"
#include "refbase.h"
#include "singleton.h"
#include "oaid_death_recipient.h"

namespace OHOS {
namespace Cloud {
class BundleMgrHelper : public DelayedSingleton<BundleMgrHelper> {
public:
    using IBundleMgr = AppExecFwk::IBundleMgr;

    BundleMgrHelper();

    virtual ~BundleMgrHelper();

    /**
     * Get all bundle infos about permission.
     */
    bool GetBundleInfosV9ByReqPermission(std::vector<AppExecFwk::BundleInfo> &bundleInfos, int32_t userId);

    bool GetApplicationInfoV9WithPermission(
        const std::string bundleName, int32_t userId, AppExecFwk::ApplicationInfo &applicationInfo);

    /**
     * Clears bundle manager helper.
     */
    void ClearBundleMgrHelper();

    void GetBundleNameByUid(const int uid, std::string &name);

private:
    bool GetBundleMgrProxy();

    sptr<AppExecFwk::IBundleMgr> bundleMgrProxy_;
    std::mutex mutex_;
    sptr<OAIDDeathRecipient> oaidDeathRecipient_;
};
}  // namespace Cloud
}  // namespace OHOS

#endif  // OHOS_CLOUD_BUNDLE_MGR_HELPER_H