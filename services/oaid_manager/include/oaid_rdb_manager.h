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

#ifndef OHOS_CLOUD_OAID_RDB_MANAGER_H
#define OHOS_CLOUD_OAID_RDB_MANAGER_H

#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

#include "rdb_helper.h"
#include "rdb_store.h"
#include "rdb_predicates.h"
#include "rdb_store_config.h"
#include "rdb_open_callback.h"
#include "values_bucket.h"
#include "value_object.h"
#include "oaid_anco_service.h"
#include "bundle_mgr_helper.h"

namespace OHOS {
namespace Cloud {

class OaidRdbManager {
public:
    static OaidRdbManager& GetInstance();

    int32_t Init();

    int32_t InsertOrReplaceSwitchStatus(int32_t userId,
        const std::string& bundleName, const std::string& uid, int32_t status);

    std::vector<AncoSwitchStatusInfo> QuerySwitchStatus(int32_t userId,
        const std::string& bundleName, const std::string& uid);

    std::vector<AncoAccessRecordInfo> QueryAccessRecordsFromDatabase(int32_t userId, const std::string& bundleName,
        const std::string& uid, int64_t sevenDaysAgo);

    std::vector<AncoAccessRecordInfo> ProcessAccessRecords(const std::vector<AncoAccessRecordInfo>& records);

    std::vector<AncoAccessRecordInfo> QueryAccessRecords(int32_t userId, const std::string& bundleName,
        const std::string& uid);

    int32_t InsertAccessRecord(const int32_t userId, const std::string bundleName, const std::string uid);

    std::vector<std::string> QueryAllBundleNames(int32_t userId);

    int32_t CleanUninstalledAppRecords(int32_t userId);

    int32_t CleanExpiredAccessRecords(int32_t userId);
private:
    OaidRdbManager() = default;
    ~OaidRdbManager();

    static int32_t CreateTable(NativeRdb::RdbStore& store, const std::string& tableName,
        const std::string& tableColumns, const std::string& primaryKey = "");

    class OaidRdbOpenCallback;
    mutable std::shared_mutex mutex_;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore_;

    std::pair<std::string, std::vector<NativeRdb::ValueObject>> BuildBatchDeleteSql(const std::string& tableName,
        const std::vector<std::string>& bundleNames);
};

} // namespace Cloud
} // namespace OHOS

#endif // OHOS_CLOUD_OAID_RDB_MANAGER_H