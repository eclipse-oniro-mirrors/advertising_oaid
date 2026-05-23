/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_CLOUD_OAID_ANCO_SERVICE_H
#define OHOS_CLOUD_OAID_ANCO_SERVICE_H

#include <string>
#include <vector>

#include "oaid_common.h"

namespace OHOS {
namespace Cloud {

/**
 * Anco switch status info.
 */
struct AncoSwitchStatusInfo {
    int32_t userId;
    std::string bundleName;
    std::string uid;
    int32_t status;
};

/**
 * Anco access record info.
 */
struct AncoAccessRecordInfo {
    int32_t userId;
    std::string bundleName;
    std::string uid;
    std::string time;
    int32_t count;
};

/**
 * AncoService class for internal API.
 * Provides static methods for anco switch status and access records.
 */
class AncoService {
public:
    /**
     * Set anco switch status for lake app.
     *
     * @param userId User space ID.
     * @param bundleName App bundle name.
     * @param uid App uid.
     * @param status Switch status: 0 allow, 1 deny.
     * @return bool, true for success, false for failure.
     */
    static bool SetAncoSwitchStatus(int32_t userId, const std::string& bundleName,
        const std::string& uid, int32_t status);

    /**
     * Get anco switch status for lake app.
     *
     * @param userId User space ID.
     * @param bundleName App bundle name (optional, must be paired with uid).
     * @param uid App uid (optional, must be paired with bundleName).
     * @return Vector of AncoSwitchStatusInfo.
     */
    static std::vector<AncoSwitchStatusInfo> GetAncoSwitchStatus(int32_t userId,
        const std::string& bundleName = "", const std::string& uid = "");

    /**
     * Get anco access records.
     *
     * @param userId User space ID.
     * @param bundleName App bundle name (optional, must be paired with uid).
     * @param uid App uid (optional, must be paired with bundleName).
     * @return Vector of AncoAccessRecordInfo.
     */
    static std::vector<AncoAccessRecordInfo> GetAncoAccessRecords(int32_t userId,
        const std::string& bundleName = "", const std::string& uid = "");
};

} // namespace Cloud
} // namespace OHOS

#endif // OHOS_CLOUD_OAID_ANCO_SERVICE_H