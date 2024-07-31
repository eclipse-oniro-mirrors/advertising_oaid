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