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

#include "oaid_fuzzer.h"

#include <string>
#include <vector>
#include "oaid_service_stub.h"
#include "oaid_service.h"
#include "oaid_hilog_wreapper.h"
#include "oaid_service_ipc_interface_code.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "bundle_mgr_helper.h"
#include "oaid_file_operator.h"
#undef private

using namespace std;
using namespace OHOS::Cloud;

namespace OHOS {
    const std::u16string OAID_INTERFACE_TOKEN = u"ohos.cloud.oaid.IOAIDService";
    const std::string OAID_TRACKING_CONSENT_PERMISSION = "ohos.permission.APP_TRACKING_CONSENT";
    const std::string OAID_TRUSTLIST_EXTENSION_CONFIG_PATH = "/etc/advertising/oaid/oaid_service_config_ext.json";
    const std::string OAID_UPDATE = "/data/service/el1/public/database/oaid_service_manager/update_check.json";

    bool g_isGrant = false;

    void AddPermission()
    {
        (void)remove(OAID_TRUSTLIST_EXTENSION_CONFIG_PATH.c_str());

        if (!g_isGrant) {
            Security::AccessToken::PermissionDef testPermDef = {
                .permissionName = OAID_TRACKING_CONSENT_PERMISSION,
                .bundleName = "test_oaid",
                .grantMode = Security::AccessToken::GrantMode::USER_GRANT,
                .availableLevel = Security::AccessToken::APL_SYSTEM_BASIC,
                .label = "label",
                .labelId = 1,
                .description = "test oaid",
                .descriptionId = 1,
            };

            Security::AccessToken::PermissionStateFull testState = {
                .isGeneral = true,
                .grantStatus = {Security::AccessToken::PermissionState::PERMISSION_GRANTED},
                .permissionName = OAID_TRACKING_CONSENT_PERMISSION,
                .grantFlags = {Security::AccessToken::PermissionFlag::PERMISSION_USER_FIXED},
                .resDeviceID = {"local"},
            };

            Security::AccessToken::HapInfoParams testInfoParms = {
                .userID = 1,
                .bundleName = "test_oaid",
                .instIndex = 0,
                .appIDDesc = "test",
                .isSystemApp = true
            };

            Security::AccessToken::HapPolicyParams testPolicyPrams = {
                .apl = Security::AccessToken::APL_SYSTEM_BASIC,
                .domain = "test.domain",
                .permList = {testPermDef},
                .permStateList = {testState},
            };

            auto tokenID = Security::AccessToken::AccessTokenKit::AllocHapToken(testInfoParms, testPolicyPrams);
            SetSelfTokenID(tokenID.tokenIDEx);

            g_isGrant = true;
        }
    }

    bool OAIDFuzzTest(const uint8_t* rawData, size_t size)
    {
        (void)rawData;
        uint32_t startCode = static_cast<uint32_t>(OHOS::Cloud::OAIDInterfaceCode::GET_OAID);
        uint32_t endCode = static_cast<uint32_t>(OHOS::Cloud::OAIDInterfaceCode::REGISTER_CONTROL_CONFIG_OBSERVER) + 1;
        for (uint32_t code = startCode; code <= endCode; code++) {
            MessageParcel data;
            data.WriteInterfaceToken(OAID_INTERFACE_TOKEN);
            MessageParcel reply;
            MessageOption option;
            auto oaidService =
                sptr<Cloud::OAIDService>(new (std::nothrow) Cloud::OAIDService());
            oaidService->OnRemoteRequest(code, data, reply, option);
        }
        // BundleMgrHelper test
        std::vector<AppExecFwk::BundleInfo> bundleInfos;
        AppExecFwk::BundleInfo bundleInfo;
        bundleInfos.push_back(bundleInfo);
        BundleMgrHelper::GetInstance()->GetBundleInfosV9ByReqPermission(bundleInfos, -1);
        AppExecFwk::ApplicationInfo applicationInfo;
        BundleMgrHelper::GetInstance()->GetApplicationInfoV9WithPermission("", -1, applicationInfo);
        BundleMgrHelper::GetInstance()->ClearBundleMgrHelper();
        // OAIDFileOperator test
        OAIDFileOperator::IsFileExsit(OAID_UPDATE);
        std::string destContent = "test";
        OAIDFileOperator::OpenAndReadFile(OAID_UPDATE, destContent);
        OAIDFileOperator::ClearFile(OAID_UPDATE);
        return true;
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::AddPermission();
    OHOS::OAIDFuzzTest(data, size);
    return 0;
}
