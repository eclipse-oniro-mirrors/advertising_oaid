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

#ifndef OHOS_CLOUD_OAID_SERVICES_INTERFACE_H
#define OHOS_CLOUD_OAID_SERVICES_INTERFACE_H

#include <string>
#include <vector>

#include "iremote_broker.h"
#include "oaid_iremote_config_observer.h"
#include "oaid_anco_service.h"

namespace OHOS {
namespace Cloud {
class IOAIDService : public IRemoteBroker {
public:
    /**
     * Get open advertising id.
     *
     * @return std::string, OAID.
     */
    virtual std::string GetOAID() = 0;

    /**
     * Reset open advertising id.
     */
    virtual int32_t ResetOAID() = 0;

    /**
     * RegisterObserver
     */
    virtual int32_t RegisterObserver(const sptr<IRemoteConfigObserver>& observer) = 0;

    /**
     * Set anco switch status.
     *
     * @param userId User space ID.
     * @param bundleName App bundle name.
     * @param uid App uid.
     * @param status Switch status: 0 allow, 1 deny.
     * @return bool, true for success, false for failure.
     */
    virtual bool SetAncoSwitchStatus(int32_t userId, const std::string& bundleName,
        const std::string& uid, int32_t status) = 0;

    /**
     * Get anco switch status.
     *
     * @param userId User space ID.
     * @param bundleName App bundle name (optional).
     * @param uid App uid (optional).
     * @return Vector of AncoSwitchStatusInfo.
     */
    virtual std::vector<AncoSwitchStatusInfo> GetAncoSwitchStatus(int32_t userId,
        const std::string& bundleName, const std::string& uid) = 0;

    /**
     * Get anco access records.
     *
     * @param userId User space ID.
     * @param bundleName App bundle name (optional).
     * @param uid App uid (optional).
     * @return Vector of AncoAccessRecordInfo.
     */
    virtual std::vector<AncoAccessRecordInfo> GetAncoAccessRecords(int32_t userId,
        const std::string& bundleName, const std::string& uid) = 0;

    virtual std::string GetAncoOAID() = 0;

    virtual int32_t InsertAccessRecord(const int32_t userId, const std::string bundleName, const std::string uid) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.cloud.oaid.IOAIDService");
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_OAID_SERVICES_INTERFACE_H