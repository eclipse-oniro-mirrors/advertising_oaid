/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
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
