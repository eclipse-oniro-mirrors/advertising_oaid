/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#include "oaid_remote_config_observer_proxy.h"
#include "oaid_iremote_config_observer.h"
#include "oaid_common.h"

namespace OHOS {
namespace Cloud {
ControlConfigObserverProxy::ControlConfigObserverProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IRemoteConfigObserver>(impl)
{}

bool ControlConfigObserverProxy::WriteInterfaceToken(MessageParcel &data)
{
    if (!data.WriteInterfaceToken(ControlConfigObserverProxy::GetDescriptor())) {
        OAID_HILOGI(OAID_MODULE_CLIENT, "write interface token failed");
        return false;
    }
    return true;
}

void ControlConfigObserverProxy::OnOaidUpdated(const std::string &value)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        OAID_HILOGI(OAID_MODULE_CLIENT, "WriteInterfaceToken data fail");
        return;
    }
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        OAID_HILOGI(OAID_MODULE_CLIENT, "remote is null");
        return;
    }
    if (!data.WriteString(value)) {
        OAID_HILOGI(OAID_MODULE_CLIENT, "WriteString failed.");
        return;
    }
    int ret = remote->SendRequest(static_cast<uint32_t>(RegisterObserverCode::OnOaidUpdated), data, reply, option);
    if (ret != ERR_OK) {
        OAID_HILOGI(OAID_MODULE_CLIENT, "OnOaidUpdated failed, error code: %{public}d", ret);
    }
}
}  // namespace Cloud
}  // namespace OHOS