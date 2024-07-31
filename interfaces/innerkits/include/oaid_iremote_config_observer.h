/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#ifndef OHOS_CLOUD_OAID_IREMOTE_CONFIG_OBSERVER_H
#define OHOS_CLOUD_OAID_IREMOTE_CONFIG_OBSERVER_H

#include <string>

#include "iremote_broker.h"

namespace OHOS {
namespace Cloud {
class IRemoteConfigObserver : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.cloud.oaid.IRemoteConfigObserver");
    enum class RegisterObserverCode {
        OnOaidUpdated = 0,
    };
    virtual void OnOaidUpdated(const std::string& value) = 0;
};
} // namespace HaCloud
} // namespace OHOS
#endif // OHOS_CLOUD_HA_IREMOTE_CONFIG_OBSERVER_H