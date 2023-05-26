/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_CLOUD_OAID_SERVICES_H
#define OHOS_CLOUD_OAID_SERVICES_H

#include <inttypes.h>
#include <mutex>
#include <string>

#include "iremote_proxy.h"
#include "distributed_kv_data_manager.h"
#include "securec.h"
#include "system_ability.h"
#include "time.h"
#include "oaid_service_stub.h"

namespace OHOS {
namespace Cloud {
enum class ServiceRunningState { STATE_NOT_START, STATE_RUNNING };

class OAIDService : public SystemAbility, public OAIDServiceStub {
    DECLARE_SYSTEM_ABILITY(OAIDService);

public:
    DISALLOW_COPY_AND_MOVE(OAIDService);
    OAIDService(int32_t systemAbilityId, bool runOnCreate);
    static sptr<OAIDService> GetInstance();
    OAIDService();
    bool InitOaidKvStore();
    virtual ~OAIDService() override;

    /**
     * Get open advertising id And connnect hms for send OaidServiceStub remoteObject to hms.
     *
     * @return std::string, OAID.
     */
    std::string GetOAID() override;

    /**
     * Clear the open advertising id.
     */
    void ClearOAID();

protected:
    void OnStart() override;
    void OnStop() override;
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;
private:
    int32_t Init();
    bool CheckKvStore();
    bool ReadValueFromKvStore(const std::string &kvStoreKey, std::string &kvStoreValue);
    bool WriteValueToKvStore(const std::string &kvStoreKey, const std::string &kvStoreValue);
    std::string GainOAID();

    ServiceRunningState state_;
    static std::mutex mutex_;
    static sptr<OAIDService> instance_;

    std::shared_ptr<DistributedKv::SingleKvStore> oaidKvStore_;
    std::string oaid_;
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_OAID_SERVICES_H
