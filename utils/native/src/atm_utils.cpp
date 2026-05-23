/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "atm_utils.h"

#include "errors.h"
#include "ipc_skeleton.h"
#include "tokenid_kit.h"
#include "oaid_hilog_wreapper.h"

namespace OHOS {
namespace Cloud {

using namespace Security::AccessToken;

bool AtmUtils::IsCallerSystemHap()
{
    return AccessTokenKit::GetTokenType(IPCSkeleton::GetCallingTokenID()) == TOKEN_HAP &&
        TokenIdKit::IsSystemAppByFullTokenID(IPCSkeleton::GetCallingFullTokenID());
}

std::optional<HapTokenInfo> AtmUtils::GetHapTokenInfo(AccessTokenID tokenID)
{
    HapTokenInfo hapTokenInfo;
    auto ret = AccessTokenKit::GetHapTokenInfo(tokenID, hapTokenInfo);
    if (ret != ERR_OK) {
        OAID_HILOGE(OAID_MODULE_COMMON, "call GetHapTokenInfo failed. ret: %{public}d", ret);
        return std::nullopt;
    }
    return {std::move(hapTokenInfo)};
}
}
}