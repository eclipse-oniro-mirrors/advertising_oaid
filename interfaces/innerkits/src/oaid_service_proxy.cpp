/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "iremote_broker.h"
#include "oaid_common.h"
#include "oaid_service_interface.h"
#include "oaid_service_ipc_interface_code.h"
#include "ha_inner_api.h"

namespace OHOS {
namespace Cloud {
using namespace OHOS::HiviewDFX;

OAIDServiceProxy::OAIDServiceProxy(const sptr<IRemoteObject>& object) : IRemoteProxy<IOAIDService>(object) {}

std::string OAIDServiceProxy::GetOAID()
{
    OAID_HILOGI(OAID_MODULE_CLIENT, "GetOAID Begin.");
    MessageParcel data, reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write parcelable");
        return "";
    }

    int32_t result = Remote()->SendRequest(static_cast<uint32_t>(OAIDInterfaceCode::GET_OAID), data, reply, option);
    if (result != ERR_NONE) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Get OAID failed, error code is: %{public}d", result);
        return "";
    }
    auto oaid = reply.ReadString();

    return oaid;
}

int32_t OAIDServiceProxy::ResetOAID()
{
    OAID_HILOGI(OAID_MODULE_CLIENT, "Reset OAID Begin.");
    MessageParcel data, reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write parcelable");
    }

    int32_t result = Remote()->SendRequest(static_cast<uint32_t>(OAIDInterfaceCode::RESET_OAID), data, reply, option);
    if (result != ERR_NONE) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Reset OAID failed, error code is: %{public}d", result);
    }
    OAID_HILOGI(OAID_MODULE_CLIENT, "Reset OAID End.");
    if (result == ERR_OK) {
        auto oaid = GetOAID();
        std::string eventId = "ADS_ID_REFRESH";
        std::unordered_map<std::string, std::string> someKv({{"type", "oaid"}, {"id", oaid.c_str()}});
        int32_t onEventRet =
            HaCloud::HaInnerApi::OnEvent("$CommonEvent", HaCloud::EventType::operation, eventId, someKv);
        OAID_HILOGI(OAID_MODULE_CLIENT, "Reset OAID onEventRet: %{public}d", onEventRet);
    }
    int32_t errorCode = reply.ReadInt32();
    OAID_HILOGI(OAID_MODULE_CLIENT, "Reset OAID End.errorCode = %{public}d", errorCode);
    return errorCode;
}
}  // namespace Cloud
}  // namespace OHOS