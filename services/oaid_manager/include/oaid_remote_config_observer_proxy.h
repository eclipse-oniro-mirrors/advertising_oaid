/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_CLOUD_OAID_REMOTE_CONFIG_OBSERVER_PROXY_H
#define OHOS_CLOUD_OAID_REMOTE_CONFIG_OBSERVER_PROXY_H

#include "iremote_proxy.h"
#include "oaid_iremote_config_observer.h"

namespace OHOS {
namespace Cloud {
class ControlConfigObserverProxy : public IRemoteProxy<IRemoteConfigObserver> {
public:
    explicit ControlConfigObserverProxy(const sptr<IRemoteObject>& impl);
    virtual ~ControlConfigObserverProxy() override = default;

    virtual void OnOaidUpdated(const std::string& value) override;

private:
    bool WriteInterfaceToken(MessageParcel& data);
    static inline BrokerDelegator<ControlConfigObserverProxy> delegator_;
};

} // namespace HaCloud
} // namespace OHOS
#endif // OHOS_CLOUD_HA_REMOTE_CONFIG_OBSERVER_PROXY_H
