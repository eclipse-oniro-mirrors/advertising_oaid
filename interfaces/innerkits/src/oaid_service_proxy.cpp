/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is: distributed on an "AS is:"BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "oaid_service_proxy.h"

#include "oaid_common.h"
#include "oaid_service_interface.h"
#include "iremote_broker.h"

namespace OHOS {
namespace Cloud {
using namespace OHOS::HiviewDFX;

OAIDServiceProxy::OAIDServiceProxy(const sptr<IRemoteObject>& object) : IRemoteProxy<IOAIDService>(object) {}

std::string OAIDServiceProxy::GetOAID()
{
    OAID_HILOGI(OAID_MODULE_CLIENT, "Begin.");
    MessageParcel data, reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write parcelable");
        return "";
    }

    int32_t result = Remote()->SendRequest(GET_OAID, data, reply, option);
    if (result != ERR_NONE) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Get OAID failed, error code is: %{public}d", result);
        return "";
    }
    auto oaid = reply.ReadString();

    return oaid;
}
} // namespace Cloud
} // namespace OHOS