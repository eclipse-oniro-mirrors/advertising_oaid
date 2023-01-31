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
#include "oaid_service.h"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <linux/rtc.h>
#include <mutex>
#include <openssl/rand.h>
#include <random>
#include <singleton.h>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "ability_connect_callback_stub.h"
#include "ability_manager_client.h"
#include "iservice_registry.h"
#include "json/json.h"
#include "oaid_common.h"
#include "pthread.h"
#include "system_ability.h"
#include "system_ability_definition.h"
#include "uri.h"

using namespace std::chrono;

namespace OHOS {
namespace Cloud {
namespace {
static const int32_t OAID_SYSTME_ID = 6101;  // The system component ID of the OAID is 6101.
static constexpr uint32_t KVSTORE_CONNECT_RETRY_COUNT = 5;
static constexpr uint32_t KVSTORE_CONNECT_RETRY_DELAY_TIME = 3000;

const std::string OAID_JSON_PATH = "/data/service/el1/public/oaid/ohos_oaid.json";
const std::string OAID_NULL_STR = "00000000-0000-0000-0000-000000000000";

const std::string OAID_DATA_BASE_DIR = "/data/service/el1/public/database/";
const std::string OAID_DATA_BASE_APP_ID = "oaid_service_manager";
const std::string OAID_DATA_BASE_STORE_ID = "oaidservice";
const std::string OAID_KVSTORE_KEY = "oaid_key";

char HexToChar(uint8_t hex)
{
    static const uint8_t maxSingleDigit = 9; // 9 is the largest single digit
    return (hex > maxSingleDigit) ? (hex + 0x57) : (hex + 0x30);
}

/**
 * Get v4 uuid.
 *
 * @return std::string, uuid.
 */
std::string GetUUID()
{
    static const int8_t uuidLength = 16;  // The UUID is 128 bits, that is 16 bytes.
    static const int8_t versionIndex = 6; // Obtain the seventh byte of the randomly generated UUID, that is uuid[6].
    static const int8_t nIndex = 8;       // Reset the ninth byte of the UUID, that is UUID[8].
    static const int8_t charLowWidth = 4; // Lower 4 bits of the char type
    unsigned char uuid[uuidLength] = {0};
    int re = RAND_bytes(uuid, sizeof(uuid));
    if (re == 0) {
        return "";
    }

    /**
     * xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx
     * M is uuid version: 4
     * N is 8,9,a,b
     */
    uuid[versionIndex] = (uuid[versionIndex] & 0x0F) | 0x40;
    int minN = 0x8;
    int maxN = 0xb;
    unsigned char randNumber[1] = {minN};
    RAND_bytes(randNumber, sizeof(randNumber));
    unsigned char num = static_cast<unsigned char>(randNumber[0] % (maxN - minN + 1) + minN);
    uuid[nIndex] = (uuid[nIndex] & 0x0F) | (num << charLowWidth);

    static const size_t lineIndexMin = 4;   // Add a hyphen (-) every two bytes starting from i=4.
    static const size_t lineIndexMax = 10;  // until i=10
    static const size_t evenFactor = 2; // the even factor is assigned to 2, and all even numbers are divisible by 2.
    std::string formatUuid = "";
    for (size_t i = 0; i < sizeof(uuid); i++) {
        unsigned char value = uuid[i];
        if (i >= lineIndexMin && i <= lineIndexMax && i % evenFactor == 0) {
            formatUuid += "-";
        }
        formatUuid += HexToChar(value >> charLowWidth);
        unsigned char highValue = value & 0xF0;
        if (highValue == 0) {
            formatUuid += HexToChar(value);
        } else {
            formatUuid += HexToChar(value % (value & highValue));
        }
    }
    return formatUuid;
}
}  // namespace

REGISTER_SYSTEM_ABILITY_BY_ID(OAIDService, OAID_SYSTME_ID, true);
std::mutex OAIDService::instanceLock_;
sptr<OAIDService> OAIDService::instance_;

OAIDService::OAIDService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate), state_(ServiceRunningState::STATE_NOT_START)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "Start.");
}

OAIDService::OAIDService() : state_(ServiceRunningState::STATE_NOT_START)
{}

OAIDService::~OAIDService(){};

sptr<OAIDService> OAIDService::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> autoLock(instanceLock_);
        if (instance_ == nullptr) {
            OAID_HILOGI(OAID_MODULE_SERVICE, "Instance success.");
            instance_ = new OAIDService;
        }
    }
    return instance_;
}

void OAIDService::OnStart()
{
    if (state_ == ServiceRunningState::STATE_RUNNING) {
        OAID_HILOGE(OAID_MODULE_SERVICE, " OAIDService is already running.");
        return;
    }

    if (Init() != ERR_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Init failed, Try again 10s later.");
        return;
    }
    AddSystemAbilityListener(OAID_SYSTME_ID);

    OAID_HILOGI(OAID_MODULE_SERVICE, "Start OAID service success.");
    return;
}

int32_t OAIDService::Init()
{
    bool ret = Publish(OAIDService::GetInstance());
    if (!ret) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "OAID service init failed.");
        return ERR_SYSYTEM_ERROR;
    }

    OAID_HILOGI(OAID_MODULE_SERVICE, "OAID service init Success.");
    state_ = ServiceRunningState::STATE_RUNNING;
    return ERR_OK;
}

void OAIDService::OnStop()
{
    if (state_ != ServiceRunningState::STATE_RUNNING) {
        return;
    }

    state_ = ServiceRunningState::STATE_NOT_START;
    OAID_HILOGI(OAID_MODULE_SERVICE, "Stop success.");
}

std::string OAIDService::GetOAIDFromFile(const std::string &filePath)
{
    std::string oaid = OAID_NULL_STR;
    std::ifstream ifs;
    ifs.open(filePath.c_str());
    if (!ifs) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Get oaid, open file error->%{public}d", errno);
        return oaid;
    }

    Json::Value jsonValue;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = true;
    std::string errs;
    if (!parseFromStream(builder, ifs, &jsonValue, &errs)) {
        ifs.close();
        OAID_HILOGE(OAID_MODULE_SERVICE, "Read file failed %{public}s.", errs.c_str());
        return oaid;
    }

    oaid = jsonValue["oaid"].asString();
    ifs.close();

    OAID_HILOGI(OAID_MODULE_SERVICE, "Read oaid from file, length is %{public}zu.", oaid.size());
    return oaid;
}

bool OAIDService::SaveOAIDToFile(const std::string &filePath, std::string oaid)
{
    Json::Value jsonValue;
    std::ofstream ofs;

    ofs.open(filePath.c_str());
    if (!ofs) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Save oaid, open file error->%{public}d", errno);
        return false;
    }

    jsonValue["oaid"] = oaid;
    Json::StreamWriterBuilder builder;
    const std::string jsonStr = Json::writeString(builder, jsonValue);
    ofs << jsonStr;
    ofs.close();
    OAID_HILOGI(OAID_MODULE_SERVICE, "Save oaid, length is %{public}zu.", oaid.size());

    return true;
}

bool OAIDService::InitOaidKvStore()
{
    DistributedKv::DistributedKvDataManager manager;
    DistributedKv::Options options;

    DistributedKv::AppId appId;
    appId.appId = OAID_DATA_BASE_APP_ID;

    options.createIfMissing = true;
    options.encrypt = false;
    options.autoSync = false;
    options.kvStoreType = DistributedKv::KvStoreType::SINGLE_VERSION;
    options.area = DistributedKv::EL1;
    options.baseDir = OAID_DATA_BASE_DIR + appId.appId;

    DistributedKv::StoreId storeId;
    storeId.storeId = OAID_DATA_BASE_STORE_ID;
    DistributedKv::Status status = DistributedKv::Status::SUCCESS;

    if (oaidKvStore_ == nullptr) {
        uint32_t retries = 0;
        do {
            OAID_HILOGI(OAID_MODULE_SERVICE, "InitOaidKvStore: retries=%{public}u!", retries);
            status = manager.GetSingleKvStore(options, appId, storeId, oaidKvStore_);
            if (status == DistributedKv::Status::STORE_NOT_FOUND) {
                OAID_HILOGE(OAID_MODULE_SERVICE, "InitOaidKvStore: STORE_NOT_FOUND!");
            }

            if ((status == DistributedKv::Status::SUCCESS) || (status == DistributedKv::Status::STORE_NOT_FOUND)) {
                break;
            } else {
                OAID_HILOGE(OAID_MODULE_SERVICE, "Kvstore Connect failed! Retrying.");
                retries++;
                usleep(KVSTORE_CONNECT_RETRY_DELAY_TIME);
            }
        } while (retries <= KVSTORE_CONNECT_RETRY_COUNT);
    }

    if (oaidKvStore_ == nullptr) {
        if (status == DistributedKv::Status::STORE_NOT_FOUND) {
            OAID_HILOGI(OAID_MODULE_SERVICE, "First Boot: Create OaidKvStore");
            options.createIfMissing = true;
            // [create and] open and initialize kvstore instance.
            status = manager.GetSingleKvStore(options, appId, storeId, oaidKvStore_);
            if (status == DistributedKv::Status::SUCCESS) {
                OAID_HILOGE(OAID_MODULE_SERVICE, "Create OaidKvStore success!");
            } else {
                OAID_HILOGE(OAID_MODULE_SERVICE, "Create OaidKvStore Failed!");
            }
        }
    }

    if (oaidKvStore_ == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "InitOaidKvStore: Failed!");
        return false;
    }

    return true;
}

void OAIDService::OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "OnAddSystemAbility OAIDService");
    bool result = false;
    switch (systemAbilityId) {
        case OAID_SYSTME_ID:
            OAID_HILOGI(OAID_MODULE_SERVICE, "OnAddSystemAbility kv data service start");
            result = InitOaidKvStore();
            OAID_HILOGI(OAID_MODULE_SERVICE, "OnAddSystemAbility InitOaidKvStore is %{public}d", result);
            break;
        default:
            OAID_HILOGI(OAID_MODULE_SERVICE, "OnAddSystemAbility unhandled sysabilityId:%{public}d", systemAbilityId);
            break;
    }
}

bool OAIDService::CheckKvStore()
{
    if (oaidKvStore_ != nullptr) {
        return true;
    }

    bool result = InitOaidKvStore();
    OAID_HILOGI(OAID_MODULE_SERVICE, "InitOaidKvStore: success!");
    return result;
}

bool OAIDService::ReadValueFromKvStore(const std::string &kvStoreKey, std::string &kvStoreValue)
{
    if (!CheckKvStore()) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "ReadValueFromKvStore:oaidKvStore_ is nullptr");
        return false;
    }

    DistributedKv::Key key(kvStoreKey);
    DistributedKv::Value value;
    DistributedKv::Status status = oaidKvStore_->Get(key, value);
    if (status == DistributedKv::Status::SUCCESS) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "%{public}d get value from kvStore", status);
    } else {
        OAID_HILOGE(OAID_MODULE_SERVICE, "%{public}d get value from kvStore failed", status);
        return false;
    }
    kvStoreValue = value.ToString();

    return true;
}

bool OAIDService::WriteValueToKvStore(const std::string &kvStoreKey, const std::string &kvStoreValue)
{
    if (!CheckKvStore()) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "WriteValueToKvStore:oaidKvStore_ is nullptr");
        return false;
    }

    DistributedKv::Key key(kvStoreKey);
    DistributedKv::Value value(kvStoreValue);
    DistributedKv::Status status = oaidKvStore_->Put(key, value);
    if (status == DistributedKv::Status::SUCCESS) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "%{public}d updated to kvStore", status);
    } else {
        OAID_HILOGE(OAID_MODULE_SERVICE, "%{public}d update to kvStore failed", status);
        return false;
    }

    return true;
}

std::string OAIDService::GetOAID()
{
    std::string path = OAID_JSON_PATH;
    std::string oaid;

    OAID_HILOGI(OAID_MODULE_SERVICE, "Begin.");

    std::string oaidKvStoreStr = OAID_NULL_STR;

    bool result = ReadValueFromKvStore(OAID_KVSTORE_KEY, oaidKvStoreStr);
    OAID_HILOGI(OAID_MODULE_SERVICE, "ReadValueFromKvStore %{public}s", result == true ? "success" : "failed");

    if (oaidKvStoreStr != OAID_NULL_STR) {
        if (!oaid_.empty()) {
            oaid = oaid_;
            OAID_HILOGI(OAID_MODULE_SERVICE, "get oaid from file successfully");
        } else {
            oaid = oaidKvStoreStr;
            oaid_ = oaidKvStoreStr;
            OAID_HILOGW(OAID_MODULE_SERVICE, "The Oaid in the memory is empty,it get oaid from file successfully");
        }
        return oaid;
    } else {
        if (oaid_.empty()) {
            oaid_ = GetUUID();
            OAID_HILOGI(OAID_MODULE_SERVICE, "The oaid has been regenerated.");
        }
    }

    oaid = oaid_;

    result = WriteValueToKvStore(OAID_KVSTORE_KEY, oaid);
    OAID_HILOGI(OAID_MODULE_SERVICE, "WriteValueToKvStore %{public}s", result == true ? "success" : "failed");

    OAID_HILOGI(OAID_MODULE_SERVICE, "End.");
    return oaid;
}
}  // namespace Cloud
}  // namespace OHOS
