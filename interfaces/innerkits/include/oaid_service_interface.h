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

#ifndef OHOS_CLOUD_OAID_SERVICES_INTERFACE_H
#define OHOS_CLOUD_OAID_SERVICES_INTERFACE_H

#include <string>

#include "iremote_broker.h"
#include "want_agent_helper.h"

namespace OHOS {
namespace Cloud {
class IOAIDService : public IRemoteBroker {
public:
    /**
     * Remote method code.
     */
    enum {
        GET_OAID = 0,
        GET_LIMIT_AD_TRACKING_ENABLED_STATUS = 1
    };

    /**
     * Get open advertising id.
     *
     * @return std::string, OAID.
     */
    virtual std::string GetOAID() = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.cloud.oaid.IOAIDService");
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_OAID_SERVICES_INTERFACE_H