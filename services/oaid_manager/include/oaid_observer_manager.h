/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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