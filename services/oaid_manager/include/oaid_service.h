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

#ifndef OHOS_CLOUD_OAID_SERVICES_H
#define OHOS_CLOUD_OAID_SERVICES_H

#include <inttypes.h>
#include <mutex>
#include <string>

#include "oaid_service_stub.h"
#include "bundle_mgr_interface.h"
#include "iremote_proxy.h"
#include "securec.h"
#include "system_ability.h"
#include "time.h"

namespace OHOS {
namespace Cloud {
using namespace OHOS::AAFwk;
using namespace OHOS::AppExecFwk;

enum class ServiceRunningState { STATE_NOT_START, STATE_RUNNING };

class OAIDService : public SystemAbility, public OAIDServiceStub {
    DECLARE_SYSTEM_ABILITY(OAIDService);

public:
    DISALLOW_COPY_AND_MOVE(OAIDService);
    OAIDService(int32_t systemAbilityId, bool runOnCreate);
    OAIDService();
    virtual ~OAIDService() override;

    /**
     * Get open advertising id.
     *
     * @return std::string, OAID.
     */
    std::string GetOAID() override;

protected:
    void OnStart() override;
    void OnStop() override;

private:
    int32_t Init();
    static sptr<OAIDService> GetInstance();
    ServiceRunningState state_;
    static std::mutex instanceLock_;
    static sptr<OAIDService> instance_;

    std::string oaid_;
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_OAID_SERVICES_H
