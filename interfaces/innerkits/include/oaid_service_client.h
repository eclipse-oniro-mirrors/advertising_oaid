/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_CLOUD_OAID_SERVICE_CLIENT_H
#define OHOS_CLOUD_OAID_SERVICE_CLIENT_H

#include <mutex>
#include <string>

#include "oaid_service_interface.h"
#include "iremote_object.h"
#include "refbase.h"

namespace OHOS {
namespace Cloud {
class OAIDSaDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    explicit OAIDSaDeathRecipient();
    virtual ~OAIDSaDeathRecipient() override = default;
    void OnRemoteDied(const wptr<IRemoteObject>& object) override;

private:
    DISALLOW_COPY_AND_MOVE(OAIDSaDeathRecipient);
};

class OAIDServiceClient : public RefBase {
public:
    DISALLOW_COPY_AND_MOVE(OAIDServiceClient);
    static sptr<OAIDServiceClient> GetInstance();

    /**
     * Get open advertising id.
     *
     * @return std::string, OAID.
     */
    std::string GetOAID();

    void OnRemoteSaDied(const wptr<IRemoteObject>& object);

    void LoadServerFail();

    void LoadServerSuccess(const sptr<IRemoteObject>& remoteObject);

private:
    OAIDServiceClient();
    virtual ~OAIDServiceClient() override;
    static std::mutex instanceLock_;
    static sptr<OAIDServiceClient> instance_;

    bool LoadServcie();
    bool loadServiceReady_ = false;
    std::mutex loadServiceLock_;
    std::condition_variable loadServiceCondition_;
    std::mutex loadServiceConditionLock_;

    sptr<IOAIDService> oaidServiceProxy_;
    sptr<OAIDSaDeathRecipient> deathRecipient_;
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_OAID_SERVICE_CLIENT_H