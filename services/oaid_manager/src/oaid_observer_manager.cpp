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

#include <memory>

#include "oaid_service.h"
#include "oaid_common.h"
#include "oaid_observer_manager.h"

namespace OHOS {
namespace Cloud {
OaidObserverManager::OaidObserverManager()
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "OaidObserverManager construct");
}

OaidObserverManager::~OaidObserverManager()
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "OaidObserverManager destruct");
}

int32_t OaidObserverManager::RegisterObserver(const sptr<IRemoteConfigObserver> &observer)
{
    if (observer == nullptr) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "observer is null");
        return ERR_INVALID_PARAM;
    }
    std::unique_lock<std::shared_mutex> lockRegister(observerMutex_);
    observer_ = observer;

    auto oaid = OAIDService::GetInstance()->GetOAID();
    observer->OnOaidUpdated(oaid);
    return ERR_OK;
}

void OaidObserverManager::OnUpdateOaid(const std::string &oaid)
{
    std::shared_lock<std::shared_mutex> lockUpdate(observerMutex_);
    if (observer_ == nullptr) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "observer is null");
        return;
    }
    observer_->OnOaidUpdated(oaid);
}
}  // namespace Cloud
}  // namespace OHOS