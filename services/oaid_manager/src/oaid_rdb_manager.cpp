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

#include "oaid_rdb_manager.h"
#include "oaid_common.h"

namespace OHOS {
namespace Cloud {

namespace {
constexpr int DB_VERSION_INIT = 1; // 此版本起，新增anco_s_status和anco_a_record表
const int DATABASE_VERSION = DB_VERSION_INIT;
constexpr size_t MAX_DELETE_COUNT = 100;
const std::string DB_PATH = "/data/service/el2/public/oaid_service_manager/database/oaid.db";
const std::string SWITCH_STATUS_TABLE = "anco_s_status";
const std::string ACCESS_RECORD_TABLE = "anco_a_record";
const int64_t ONE_MINUTE_MS = 60 * 1000LL;
const int64_t TIME_DIFF_THRESHOLD_MS = 200;
const int64_t SEVEN_DAYS_MS = 7 * 24 * 60 * ONE_MINUTE_MS;
const int64_t TEN_DAYS_MS = 10 * 24 * 60 * ONE_MINUTE_MS;

// 表定义：表名 -> (字段定义, 主键)
const std::vector<std::tuple<std::string, std::string, std::string>>& GetTableDefinitions()
{
    static const std::vector<std::tuple<std::string, std::string, std::string>> tables = {
        { SWITCH_STATUS_TABLE,
          "user_id INTEGER NOT NULL, bn TEXT NOT NULL, uid TEXT NOT NULL, "
          "res INTEGER NOT NULL DEFAULT 0, create_time INTEGER NOT NULL, update_time INTEGER NOT NULL",
          "PRIMARY KEY (user_id, bn, uid)" },
        { ACCESS_RECORD_TABLE,
          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
          "user_id INTEGER NOT NULL, bn TEXT NOT NULL, uid TEXT NOT NULL, "
          "time INTEGER NOT NULL",
          "" }
    };
    return tables;
}
}

int32_t OaidRdbManager::CreateTable(NativeRdb::RdbStore& store, const std::string& tableName,
    const std::string& tableColumns, const std::string& primaryKey)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName + " (" + tableColumns;
    if (!primaryKey.empty()) {
        sql += ", " + primaryKey;
    }
    sql += ")";
    int err = store.ExecuteSql(sql);
    if (err != NativeRdb::E_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to create table %{public}s, err=%{public}d", tableName.c_str(), err);
        return err;
    }
    return NativeRdb::E_OK;
}

class OaidRdbManager::OaidRdbOpenCallback : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore& store) override
    {
        for (const auto& [tableName, tableColumns, primaryKey] : GetTableDefinitions()) {
            int err = CreateTable(store, tableName, tableColumns, primaryKey);
            if (err != NativeRdb::E_OK) {
                return err;
            }
        }
        OAID_HILOGI(OAID_MODULE_SERVICE, "RDB tables created successfully");
        return NativeRdb::E_OK;
    }
    int OnUpgrade(NativeRdb::RdbStore& store, int currentVersion, int targetVersion) override
    {
        return NativeRdb::E_OK;
    }
};

OaidRdbManager::~OaidRdbManager()
{
    if (rdbStore_ != nullptr) {
        rdbStore_ = nullptr;
    }
}

OaidRdbManager& OaidRdbManager::GetInstance()
{
    static OaidRdbManager instance;
    return instance;
}

int32_t OaidRdbManager::Init()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (rdbStore_ != nullptr) {
        return ERR_OK;
    }
    NativeRdb::RdbStoreConfig config(DB_PATH);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S2);
    int errCode = NativeRdb::E_OK;
    OaidRdbOpenCallback callback;
    rdbStore_ = NativeRdb::RdbHelper::GetRdbStore(config, DATABASE_VERSION, callback, errCode);
    if (errCode != NativeRdb::E_OK || rdbStore_ == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "RDB may be corrupted, trying to delete and recreate, errCode=%{public}d",
            errCode);
        NativeRdb::RdbHelper::DeleteRdbStore(DB_PATH);
        rdbStore_ = nullptr;
        rdbStore_ = NativeRdb::RdbHelper::GetRdbStore(config, DATABASE_VERSION, callback, errCode);
        if (errCode != NativeRdb::E_OK || rdbStore_ == nullptr) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to recreate RdbStore after corruption recovery, "
                "errCode=%{public}d", errCode);
            return ERR_DB_CONNECT_FAILED;
        }
    }
    OAID_HILOGI(OAID_MODULE_SERVICE, "RDB initialized successfully");
    return ERR_OK;
}

int32_t OaidRdbManager::InsertOrReplaceSwitchStatus(int32_t userId,
    const std::string& bundleName, const std::string& uid, int32_t status)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (rdbStore_ == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "RDB not initialized");
        return ERR_DB_CONNECT_FAILED;
    }
    int64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    NativeRdb::RdbPredicates predicates(SWITCH_STATUS_TABLE);
    predicates.EqualTo("user_id", NativeRdb::ValueObject(userId));
    predicates.And();
    predicates.EqualTo("bn", NativeRdb::ValueObject(bundleName));
    predicates.And();
    predicates.EqualTo("uid", NativeRdb::ValueObject(uid));
    int64_t count = 0;
    int err = rdbStore_->Count(count, predicates);
    if (err != NativeRdb::E_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to count switch status, err=%{public}d", err);
        return ERR_DB_CONNECT_FAILED;
    }
    if (count > 0) {
        NativeRdb::ValuesBucket row;
        row.PutInt("res", status);
        row.PutLong("update_time", currentTime);
        int changedRows = 0;
        err = rdbStore_->Update(changedRows, row, predicates);
        if (err != NativeRdb::E_OK) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to update switch status, err=%{public}d", err);
            return ERR_DB_CONNECT_FAILED;
        }
    } else {
        NativeRdb::ValuesBucket row;
        row.PutInt("user_id", userId);
        row.PutString("bn", bundleName);
        row.PutString("uid", uid);
        row.PutInt("res", status);
        row.PutLong("create_time", currentTime);
        row.PutLong("update_time", currentTime);
        int64_t outRowId = 0;
        err = rdbStore_->Insert(outRowId, SWITCH_STATUS_TABLE, row);
        if (err != NativeRdb::E_OK) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to insert switch status, err=%{public}d", err);
            return ERR_DB_CONNECT_FAILED;
        }
    }
    return ERR_OK;
}

std::vector<AncoSwitchStatusInfo> OaidRdbManager::QuerySwitchStatus(int32_t userId,
    const std::string& bundleName, const std::string& uid)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<AncoSwitchStatusInfo> result;
    if (rdbStore_ == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "RDB not initialized");
        return result;
    }
    NativeRdb::RdbPredicates predicates(SWITCH_STATUS_TABLE);
    predicates.EqualTo("user_id", NativeRdb::ValueObject(userId));
    if (!bundleName.empty() && !uid.empty()) {
        predicates.And();
        predicates.EqualTo("bn", NativeRdb::ValueObject(bundleName));
        predicates.And();
        predicates.EqualTo("uid", NativeRdb::ValueObject(uid));
    }
    auto resultSet = rdbStore_->Query(predicates, {});
    if (resultSet == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Query result set is null");
        return result;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AncoSwitchStatusInfo info;
        int columnIndex = 0;
        resultSet->GetInt(columnIndex++, info.userId);
        resultSet->GetString(columnIndex++, info.bundleName);
        resultSet->GetString(columnIndex++, info.uid);
        resultSet->GetInt(columnIndex++, info.status);

        result.push_back(info);
    }

    resultSet->Close();
    OAID_HILOGI(OAID_MODULE_SERVICE, "QuerySwitchStatus success, count=%{public}zu", result.size());
    return result;
}

std::vector<AncoAccessRecordInfo> OaidRdbManager::QueryAccessRecordsFromDatabase(
    int32_t userId, const std::string& bundleName, const std::string& uid, int64_t sevenDaysAgo)
{
    std::vector<AncoAccessRecordInfo> result;
    std::string sql;
    std::vector<NativeRdb::ValueObject> args;
    if (!bundleName.empty() && !uid.empty()) {
        sql = "SELECT user_id, bn, uid, time FROM " + ACCESS_RECORD_TABLE +
              " WHERE user_id = ? AND bn = ? AND uid = ? AND time >= ?";
        args.push_back(NativeRdb::ValueObject(userId));
        args.push_back(NativeRdb::ValueObject(bundleName));
        args.push_back(NativeRdb::ValueObject(uid));
        args.push_back(NativeRdb::ValueObject(sevenDaysAgo));
    } else {
        sql = "SELECT user_id, bn, uid, time FROM " + ACCESS_RECORD_TABLE +
              " WHERE user_id = ? AND time >= ?";
        args.push_back(NativeRdb::ValueObject(userId));
        args.push_back(NativeRdb::ValueObject(sevenDaysAgo));
    }
    auto resultSet = rdbStore_->QuerySql(sql, args);
    if (resultSet == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Query result set is null");
        return result;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int columnIndex = 0;
        int32_t recordUserId;
        std::string recordBundleName;
        std::string recordUid;
        int64_t timeValue = 0;
        resultSet->GetInt(columnIndex++, recordUserId);
        resultSet->GetString(columnIndex++, recordBundleName);
        resultSet->GetString(columnIndex++, recordUid);
        resultSet->GetLong(columnIndex++, timeValue);
        AncoAccessRecordInfo info;
        info.userId = recordUserId;
        info.bundleName = recordBundleName;
        info.uid = recordUid;
        info.time = std::to_string(timeValue);
        info.count = 1; // Initialize count to 1
        result.push_back(info);
    }
    resultSet->Close();
    return result;
}

std::vector<AncoAccessRecordInfo> OaidRdbManager::ProcessAccessRecords(
    const std::vector<AncoAccessRecordInfo>& records)
{
    std::vector<AncoAccessRecordInfo> result;
    struct MinuteGroupKey {
        int32_t userId;
        std::string bundleName;
        std::string uid;
        int64_t minuteGroup;
        bool operator<(const MinuteGroupKey& other) const
        {
            if (userId != other.userId) return userId < other.userId;
            if (bundleName != other.bundleName) return bundleName < other.bundleName;
            if (uid != other.uid) return uid < other.uid;
            return minuteGroup < other.minuteGroup;
        }
    };
    std::map<MinuteGroupKey, std::vector<std::pair<int64_t, int32_t>>> minuteGroups;
    for (const auto& record : records) {
        MinuteGroupKey key;
        key.userId = record.userId;
        key.bundleName = record.bundleName;
        key.uid = record.uid;
        key.minuteGroup = std::stoll(record.time) / ONE_MINUTE_MS;
        minuteGroups[key].push_back({std::stoll(record.time), 1});
    }
    for (auto& pair : minuteGroups) {
        auto& timeList = pair.second;
        std::sort(timeList.begin(), timeList.end(),
                  [](const std::pair<int64_t, int32_t>& a, const std::pair<int64_t, int32_t>& b) {
                      return a.first < b.first;
                  });
        std::vector<std::pair<int64_t, int32_t>> mergedList;
        for (const auto& item : timeList) {
            if (mergedList.empty()) {
                mergedList.push_back(item);
            } else {
                int64_t timeDiff = item.first - mergedList.back().first;
                if (timeDiff <= TIME_DIFF_THRESHOLD_MS) {
                    mergedList.back().second += item.second;
                } else {
                    mergedList.push_back(item);
                }
            }
        }
        AncoAccessRecordInfo info;
        info.userId = pair.first.userId;
        info.bundleName = pair.first.bundleName;
        info.uid = pair.first.uid;
        info.time = std::to_string(mergedList.front().first);
        info.count = static_cast<int32_t>(mergedList.size());
        result.push_back(info);
    }
    return result;
}

std::vector<AncoAccessRecordInfo> OaidRdbManager::QueryAccessRecords(int32_t userId,
    const std::string& bundleName, const std::string& uid)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<AncoAccessRecordInfo> result;
    if (rdbStore_ == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "RDB not initialized");
        return result;
    }
    int64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t sevenDaysAgo = currentTime - SEVEN_DAYS_MS;
    // Query records from the database
    std::vector<AncoAccessRecordInfo> records = QueryAccessRecordsFromDatabase(userId, bundleName, uid, sevenDaysAgo);
    // Process the records
    result = ProcessAccessRecords(records);
    OAID_HILOGI(OAID_MODULE_SERVICE, "QueryAccessRecords success, count=%{public}zu", result.size());
    return result;
}
int32_t OaidRdbManager::InsertAccessRecord(const int32_t userId, const std::string bundleName, const std::string uid)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (rdbStore_ == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "RDB not initialized");
        return ERR_DB_CONNECT_FAILED;
    }
    int64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    NativeRdb::ValuesBucket row;
    row.PutInt("user_id", userId);
    row.PutString("bn", bundleName);
    row.PutString("uid", uid);
    row.PutLong("time", currentTime);
    int64_t outRowId = 0;
    int err = rdbStore_->Insert(outRowId, ACCESS_RECORD_TABLE, row);
    if (err != NativeRdb::E_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to insert accessRecord, err=%{public}d", err);
        return ERR_DB_CONNECT_FAILED;
    }
    OAID_HILOGI(OAID_MODULE_SERVICE, "InsertAccessRecord success");
    return ERR_OK;
}

std::vector<std::string> OaidRdbManager::QueryAllBundleNames(int32_t userId)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<std::string> bundleNames;
    if (rdbStore_ == nullptr) {
        return bundleNames;
    }
    // UNION 查询两个表，返回去重的 bundleName
    std::string sql = "SELECT DISTINCT bn FROM " + SWITCH_STATUS_TABLE +
                      " WHERE user_id = ? UNION SELECT DISTINCT bn FROM " +
                      ACCESS_RECORD_TABLE + " WHERE user_id = ?";
    std::vector<NativeRdb::ValueObject> args = {
        NativeRdb::ValueObject(userId),
        NativeRdb::ValueObject(userId)
    };
    auto resultSet = rdbStore_->QuerySql(sql, args);
    if (resultSet == nullptr) {
        return bundleNames;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string bundleName;
        resultSet->GetString(0, bundleName);
        bundleNames.push_back(bundleName);
    }
    resultSet->Close();
    return bundleNames;
}

int32_t OaidRdbManager::CleanUninstalledAppRecords(int32_t userId)
{
    std::vector<std::string> allBundleNames = QueryAllBundleNames(userId);
    if (allBundleNames.empty()) {
        return ERR_OK;
    }
    // 过滤出已卸载应用的 bundleName
    std::vector<std::string> uninstalledBundles;
    for (const auto& bundleName : allBundleNames) {
        AppExecFwk::BundleInfo bundleInfo;
            if (uninstalledBundles.size() < MAX_DELETE_COUNT &&
                !BundleMgrHelper::GetInstance()->GetBundleInfo(bundleName, bundleInfo, userId)) {
                uninstalledBundles.push_back(bundleName);
            }
    }
    if (uninstalledBundles.empty()) {
        return ERR_OK;
    }
    std::unique_lock<std::shared_mutex> lock(mutex_);
    // 批量删除开关状态表记录
    auto [switchDeleteSql, switchArgs] = BuildBatchDeleteSql(SWITCH_STATUS_TABLE, uninstalledBundles);
    int32_t ret = rdbStore_->ExecuteSql(switchDeleteSql, switchArgs);
    if (ret != NativeRdb::E_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to batch delete switch status, ret=%{public}d", ret);
        return ERR_DB_CONNECT_FAILED;
    }
    // 批量删除访问记录表记录
    auto [recordDeleteSql, recordArgs] = BuildBatchDeleteSql(ACCESS_RECORD_TABLE, uninstalledBundles);
    ret = rdbStore_->ExecuteSql(recordDeleteSql, recordArgs);
    if (ret != NativeRdb::E_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to batch delete access records, ret=%{public}d", ret);
        return ERR_DB_CONNECT_FAILED;
    }
    OAID_HILOGI(OAID_MODULE_SERVICE, "CleanUninstalledAppRecords success, cleaned=%{public}zu",
        uninstalledBundles.size());
    return ERR_OK;
}

std::pair<std::string, std::vector<NativeRdb::ValueObject>> OaidRdbManager::BuildBatchDeleteSql(
    const std::string& tableName, const std::vector<std::string>& bundleNames)
{
    std::string sql = "DELETE FROM " + tableName + " WHERE bn IN (";
    std::vector<NativeRdb::ValueObject> args;
    for (size_t i = 0; i < bundleNames.size(); ++i) {
        if (i > 0) {
            sql += ", ";
        }
        sql += "?";
        args.push_back(NativeRdb::ValueObject(bundleNames[i]));
    }
    sql += ")";
    return std::make_pair(sql, args);
}

int32_t OaidRdbManager::CleanExpiredAccessRecords(int32_t userId)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (rdbStore_ == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "RDB not initialized");
        return ERR_DB_CONNECT_FAILED;
    }
    int64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t tenDaysAgo = currentTime - TEN_DAYS_MS;
    NativeRdb::RdbPredicates predicates(ACCESS_RECORD_TABLE);
    predicates.EqualTo("user_id", NativeRdb::ValueObject(userId));
    predicates.LessThan("time", NativeRdb::ValueObject(tenDaysAgo));
    int deletedRows = 0;
    int32_t ret = rdbStore_->Delete(deletedRows, predicates);
    if (ret != NativeRdb::E_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to clean expired access records, ret=%{public}d", ret);
        return ERR_DB_CONNECT_FAILED;
    }
    OAID_HILOGI(OAID_MODULE_SERVICE, "CleanExpiredAccessRecords success, deletedRows=%{public}d", deletedRows);
    return ERR_OK;
}

} // namespace Cloud
} // namespace OHOS