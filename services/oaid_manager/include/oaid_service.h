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

#ifndef OHOS_CLOUD_OAID_SERVICES_H
#define OHOS_CLOUD_OAID_SERVICES_H

#include <mutex>
#include <string>

#include "iremote_proxy.h"
#include "distributed_kv_data_manager.h"
#include "securec.h"
#include "system_ability.h"
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
    bool InitKvStore(std::string storeIdStr);
    virtual ~OAIDService() override;

    /**
     * Get OAID
     *
     * @return std::string, OAID.
     */
    std::string GetOAID() override;

    /**
     * Reset open advertising id.
     */
    int32_t ResetOAID() override;
    bool ReadValueFromUnderAgeKvStore(const std::string &kvStoreKey, DistributedKv::Value &kvStoreValue);
    bool WriteValueToUnderAgeKvStore(const std::string &kvStoreKey, const DistributedKv::Value &kvStoreValue);
protected:
    void OnStart() override;
    void OnStop() override;
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;
private:
    int32_t Init();
    bool CheckKvStore();
    bool ReadValueFromKvStore(const std::string &kvStoreKey, std::string &kvStoreValue);
    bool WriteValueToKvStore(const std::string &kvStoreKey, const std::string &kvStoreValue);
    bool CheckUnderAgeKvStore();
    std::string GainOAID();

    ServiceRunningState state_;
    static std::mutex mutex_;
    static sptr<OAIDService> instance_;

    std::shared_ptr<DistributedKv::SingleKvStore> oaidKvStore_;
    std::shared_ptr<DistributedKv::SingleKvStore> oaidUnderAgeKvStore_;
    std::mutex updateMutex_;
    std::string oaid_;
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_OAID_SERVICES_H
