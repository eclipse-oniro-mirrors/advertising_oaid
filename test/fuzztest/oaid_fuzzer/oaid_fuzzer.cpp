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
    bool g_isGrant = false;

    void AddPermission()
    {
        if (!g_isGrant) {
            const char *perms[] = {
                OAID_TRACKING_CONSENT_PERMISSION.c_str()
            };
            NativeTokenInfoParams infoInstance = {
                .dcapsNum = 0,
                .permsNum = 1,
                .aclsNum = 0,
                .dcaps = nullptr,
                .perms = perms,
                .acls = nullptr,
                .processName = "OAIDFuzzer",
                .aplStr = "system_basic",
            };
            uint64_t tokenId = GetAccessTokenId(&infoInstance);
            SetSelfTokenID(tokenId);
            Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
            g_isGrant = true;
        }
    }

    bool OAIDFuzzTest(const uint8_t* rawData, size_t size)
    {
        uint32_t startCode = static_cast<uint32_t>(OHOS::Cloud::OAIDInterfaceCode::GET_OAID);
        uint32_t endCode = static_cast<uint32_t>(OHOS::Cloud::OAIDInterfaceCode::RESET_OAID);
        for (uint32_t code = startCode; code <= endCode; code++) {
            MessageParcel data;
            data.WriteInterfaceToken(OAID_INTERFACE_TOKEN);
            MessageParcel reply;
            MessageOption option;
            auto oaidService =
                sptr<Cloud::OAIDService>(new (std::nothrow) Cloud::OAIDService());
            oaidService->OnRemoteRequest(code, data, reply, option);
        }

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

