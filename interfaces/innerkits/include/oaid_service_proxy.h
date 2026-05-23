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

#ifndef OHOS_CLOUD_OAID_SERVICE_PROXY_H
#define OHOS_CLOUD_OAID_SERVICE_PROXY_H

#include <string>

#include "oaid_service_interface.h"
#include "iremote_proxy.h"
#include "oaid_common.h"

namespace OHOS {
namespace Cloud {
class OAIDServiceProxy : public IRemoteProxy<IOAIDService> {
public:
    explicit OAIDServiceProxy(const sptr<IRemoteObject>& object);
    virtual ~OAIDServiceProxy() override = default;
    DISALLOW_COPY_AND_MOVE(OAIDServiceProxy);

    /**
     * Get open advertising id.
     *
     * @return std::string, OAID.
     */
    std::string GetOAID() override;

    /**
     * Reset open advertising id.
     */
    int32_t ResetOAID() override;

    /**
     * RegisterObserver
     */
    int32_t RegisterObserver(const sptr<IRemoteConfigObserver>& observer) override;

    /**
     * Set anco switch status.
     *
     * @param userId User space ID.
     * @param bundleName App bundle name.
     * @param uid App uid.
     * @param status Switch status: 0 allow, 1 deny.
     * @return bool, true for success, false for failure.
     */
    bool SetAncoSwitchStatus(int32_t userId, const std::string& bundleName,
        const std::string& uid, int32_t status) override;

    /**
     * Get anco switch status.
     *
     * @param userId User space ID.
     * @param bundleName App bundle name (optional).
     * @param uid App uid (optional).
     * @return Vector of AncoSwitchStatusInfo.
     */
    std::vector<AncoSwitchStatusInfo> GetAncoSwitchStatus(int32_t userId,
        const std::string& bundleName, const std::string& uid) override;

    /**
     * Get anco access records.
     *
     * @param userId User space ID.
     * @param bundleName App bundle name (optional).
     * @param uid App uid (optional).
     * @return Vector of AncoAccessRecordInfo.
     */
    std::vector<AncoAccessRecordInfo> GetAncoAccessRecords(int32_t userId,
        const std::string& bundleName, const std::string& uid) override;

    std::string GetAncoOAID() override;

    int32_t InsertAccessRecord(const int32_t userId, const std::string bundleName, const std::string uid) override;
private:
    static inline BrokerDelegator<OAIDServiceProxy> delegator_;
    std::mutex registerObserverMutex_;
    bool WriteQueryParams(MessageParcel& data, int32_t userId,
        const std::string& bundleName, const std::string& uid);
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_OAID_SERVICE_PROXY_H