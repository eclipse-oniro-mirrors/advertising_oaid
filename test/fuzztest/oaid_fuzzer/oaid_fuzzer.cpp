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
#undef private

using namespace std;
using namespace OHOS::Cloud;

namespace OHOS {
const std::u16string OAID_INTERFACE_TOKEN = u"ohos.cloud.oaid.IOAIDService";
const std::string OAID_TRACKING_CONSENT_PERMISSION = "ohos.permission.APP_TRACKING_CONSENT";
const std::string NETWORK_PERMISSION = "ohos.permission.INTERNET";
const std::string OAID_TRUSTLIST_EXTENSION_CONFIG_PATH = "/etc/advertising/oaid/oaid_service_config_ext.json";

void AddPermission(bool isSetSystemApp, bool isGrandOaidPermission, bool isNeedRemoveConfigFile)
{
    if (isNeedRemoveConfigFile) {
        (void)remove(OAID_TRUSTLIST_EXTENSION_CONFIG_PATH.c_str());
    }

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
        .permissionName = isGrandOaidPermission ? OAID_TRACKING_CONSENT_PERMISSION : NETWORK_PERMISSION,
        .grantFlags = {Security::AccessToken::PermissionFlag::PERMISSION_USER_FIXED},
        .resDeviceID = {"local"},
    };

    Security::AccessToken::HapInfoParams testInfoParms = {.userID = 1,
        .bundleName = "test_oaid",
        .instIndex = 0,
        .appIDDesc = "test",
        .isSystemApp = isSetSystemApp ? true : false};

    Security::AccessToken::HapPolicyParams testPolicyPrams = {
        .apl = Security::AccessToken::APL_SYSTEM_BASIC,
        .domain = "test.domain",
        .permList = {testPermDef},
        .permStateList = {testState},
    };

    auto tokenID = Security::AccessToken::AccessTokenKit::AllocHapToken(testInfoParms, testPolicyPrams);
    SetSelfTokenID(tokenID.tokenIDEx);
}

bool OAIDFuzzTest(const uint8_t *rawData, size_t size)
{
    uint32_t startCode = static_cast<uint32_t>(OHOS::Cloud::OAIDInterfaceCode::GET_OAID);
    uint32_t endCode = static_cast<uint32_t>(OHOS::Cloud::OAIDInterfaceCode::REGISTER_CONTROL_CONFIG_OBSERVER) + 1;
    for (uint32_t code = startCode; code <= endCode; code++) {
        MessageParcel data;
        data.WriteInterfaceToken(OAID_INTERFACE_TOKEN);
        MessageParcel reply;
        MessageOption option;
        auto oaidService = std::make_shared<Cloud::OAIDService>();
        if (!oaidService) {
            return false;
        }
        oaidService->OnRemoteRequest(code, data, reply, option);
    }
    return true;
}
}  // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    // not system app, no grand permission, no remove config file
    OHOS::AddPermission(false, false, false);
    OHOS::OAIDFuzzTest(data, size);
    // system app, no grand permission, no remove config file
    OHOS::AddPermission(true, false, false);
    OHOS::OAIDFuzzTest(data, size);
    // not system app, grand permission, no remove config file
    OHOS::AddPermission(false, true, false);
    OHOS::OAIDFuzzTest(data, size);
    // system app, grand permission, no remove config file
    OHOS::AddPermission(true, true, false);
    OHOS::OAIDFuzzTest(data, size);
    // not system app, grand permission, remove config file
    OHOS::AddPermission(false, true, true);
    OHOS::OAIDFuzzTest(data, size);
    // system app, grand permission, remove config file
    OHOS::AddPermission(true, true, true);
    OHOS::OAIDFuzzTest(data, size);
    return 0;
}
