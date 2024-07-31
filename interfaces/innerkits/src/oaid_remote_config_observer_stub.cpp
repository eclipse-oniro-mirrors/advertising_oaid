/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#include "oaid_remote_config_observer_stub.h"
#include "oaid_common.h"

namespace OHOS {
namespace Cloud {

RemoteConfigObserverStub::RemoteConfigObserverStub()
{}

RemoteConfigObserverStub::~RemoteConfigObserverStub()
{}

int32_t RemoteConfigObserverStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    std::string bundleName;
    std::u16string descriptor = RemoteConfigObserverStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "read descriptor failed.");
        return ERR_INVALID_PARAM;
    }
    switch (code) {
        case static_cast<uint32_t>(RegisterObserverCode::OnOaidUpdated): {
            return HandleOAIDUpdate(data, reply);
        }
        default:
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
}

int32_t RemoteConfigObserverStub::HandleOAIDUpdate(MessageParcel &data, MessageParcel &reply)
{
    std::string oaid;
    if (!data.ReadString(oaid)) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "parcel read value failed.");
        return ERR_INVALID_PARAM;
    }
    OnOaidUpdated(oaid);
    return ERR_OK;
}
}  // namespace Cloud
}  // namespace OHOS