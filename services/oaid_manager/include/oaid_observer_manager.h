/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#ifndef OHOS_CLOUD_OAID_REMOTE_CONFIG_OBSERVER_MANAGER_H
#define OHOS_CLOUD_OAID_REMOTE_CONFIG_OBSERVER_MANAGER_H

#include <shared_mutex>

#include "singleton.h"
#include "oaid_iremote_config_observer.h"

namespace OHOS {
namespace Cloud {
class OaidObserverManager {
    DECLARE_DELAYED_SINGLETON(OaidObserverManager)
public:
    int32_t RegisterObserver(const sptr<IRemoteConfigObserver>& observer);

    void OnUpdateOaid(const std::string& oaid);

private:
   sptr<IRemoteConfigObserver> observer_;
   std::shared_mutex observerMutex_;
};
} // namespace HaCloud
} // namespace OHOS
#endif // OHOS_CLOUD_HA_REMOTE_CONFIG_OBSERVER_MANAGER_H