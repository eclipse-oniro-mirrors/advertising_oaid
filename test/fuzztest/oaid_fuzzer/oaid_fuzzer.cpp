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
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#undef private

using namespace std;
using namespace OHOS::Cloud;

namespace OHOS {
    const std::u16string OAID_INTERFACE_TOKEN = u"ohos.cloud.oaid.IOAIDService";
    const std::string OAID_TRACKING_CONSENT_PERMISSION = "ohos.permission.APP_TRACKING_CONSENT";
    const int32_t SHIFT_LEFT_8 = 8;
    const int32_t SHIFT_LEFT_16 = 16;
    const int32_t SHIFT_LEFT_24 = 24;
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

    uint32_t Convert2Uint32(const uint8_t *ptr)
    {
        if (ptr == nullptr) {
            return 0;
        }
        /* Move the 0th digit to the left by 24 bits, the 1st digit to the left by 16 bits,
           the 2nd digit to the left by 8 bits, and the 3rd digit not to the left */
        return (ptr[0] << SHIFT_LEFT_24) | (ptr[1] << SHIFT_LEFT_16) | (ptr[2] << SHIFT_LEFT_8) | (ptr[3]);
    }

    bool OAIDFuzzTest(const uint8_t* rawData, size_t size)
    {
        uint32_t code = Convert2Uint32(rawData);

        MessageParcel data;
        data.WriteInterfaceToken(OAID_INTERFACE_TOKEN);
        MessageParcel reply;
        MessageOption option;
        auto oaidService =
            sptr<Cloud::OAIDService>(new (std::nothrow) Cloud::OAIDService());
        oaidService->OnRemoteRequest(code, data, reply, option);
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

