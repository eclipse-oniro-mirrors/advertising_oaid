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
#include "oaid_service.h"
#include <mutex>
#include <openssl/rand.h>
#include <singleton.h>
#include <string>
#include <unistd.h>
#include "oaid_common.h"
#include "oaid_file_operator.h"
#include "system_ability.h"
#include "system_ability_definition.h"
#include "oaid_service_stub.h"
#include "oaid_service_define.h"

using namespace std::chrono;

namespace OHOS {
namespace Cloud {
namespace {
char HexToChar(uint8_t hex)
{
    static const uint8_t MAX_SINGLE_DIGIT = 9; // 9 is the largest single digit
    return (hex > MAX_SINGLE_DIGIT) ? (hex + 0x57) : (hex + 0x30);
}

/**
 * Get v4 uuid.
 *
 * @return std::string, uuid.
 */
std::string GetUUID()
{
    static const int8_t UUID_LENGTH = 16;  // The UUID is 128 bits, that is 16 bytes.
    static const int8_t VERSION_INDEX = 6; // Obtain the seventh byte of the randomly generated UUID, that is uuid[6].
    static const int8_t CHAR_LOW_WIDTH = 4; // Lower 4 bits of the char type
    static const int8_t N_INDEX = 8;       // Reset the ninth byte of the UUID, that is UUID[8].
    unsigned char uuid[UUID_LENGTH] = {0};
    int re = RAND_bytes(uuid, sizeof(uuid));
    if (re == 0) {
        return "";
    }

    /**
     * xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx
     * M is uuid version: 4
     * N is 8,9,a,b
     */
    uuid[VERSION_INDEX] = (uuid[VERSION_INDEX] & 0x0F) | 0x40;
    int minN = 0x8;
    int maxN = 0xb;
    unsigned char randNumber[1] = {minN};
    RAND_bytes(randNumber, sizeof(randNumber));
    unsigned char num = static_cast<unsigned char>(randNumber[0] % (maxN - minN + 1) + minN);
    uuid[N_INDEX] = (uuid[N_INDEX] & 0x0F) | (num << CHAR_LOW_WIDTH);

    static const size_t LINE_INDEX_MAX = 10;  // until i=10
    static const size_t LINE_INDEX_MIN = 4;   // Add a hyphen (-) every two bytes starting from i=4.
    static const size_t EVEN_FACTOR = 2; // the even factor is assigned to 2, and all even numbers are divisible by 2.
    std::string formatUuid = "";
    for (size_t i = 0; i < sizeof(uuid); i++) {
        unsigned char value = uuid[i];
        if (i >= LINE_INDEX_MIN && i <= LINE_INDEX_MAX && i % EVEN_FACTOR == 0) {
            formatUuid += "-";
        }
        formatUuid += HexToChar(value >> CHAR_LOW_WIDTH);
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
std::mutex OAIDService::mutex_;
sptr<OAIDService> OAIDService::instance_;

OAIDService::OAIDService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate), state_(ServiceRunningState::STATE_NOT_START)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "Start.");
}

OAIDService::OAIDService() : state_(ServiceRunningState::STATE_NOT_START)
{}

OAIDService::~OAIDService() {};

sptr<OAIDService> OAIDService::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> autoLock(mutex_);
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

bool OAIDService::InitOaidKvStore()
{
    DistributedKv::DistributedKvDataManager manager;
    DistributedKv::Options options;

    DistributedKv::AppId appId;
    appId.appId = OAID_DATA_BASE_APP_ID;

    options.createIfMissing = true;
    options.encrypt = true;
    options.autoSync = false;
    options.kvStoreType = DistributedKv::KvStoreType::SINGLE_VERSION;
    options.area = DistributedKv::EL1;
    options.baseDir = OAID_DATA_BASE_DIR + appId.appId;
    options.securityLevel = DistributedKv::SecurityLevel::S1;

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
    OAID_HILOGI(OAID_MODULE_SERVICE, "InitOaidKvStore: %{public}s", result == true ? "success" : "failed");
    return result;
}

bool OAIDService::ReadValueFromKvStore(const std::string &kvStoreKey, std::string &kvStoreValue)
{
    std::lock_guard<std::mutex> lock(mutex_);

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
    std::lock_guard<std::mutex> lock(mutex_);

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

std::string OAIDService::GainOAID()
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "Gain OAID Begin.");
    std::string oaidKvStoreStr = OAID_ALLZERO_STR;
    updateMutex_.lock();
    if (OAIDFileOperator::IsFileExsit(OAID_UPDATE)) {
        OAIDFileOperator::OpenAndReadFile(OAID_UPDATE, oaidKvStoreStr);
        OAIDFileOperator::ClearFile(OAID_UPDATE);
        Json::Reader reader;
        Json::Value root;
        std::string oaid;
        if (reader.parse(oaidKvStoreStr, root)) {
            oaid = root["oaid"].asString();
        }
        oaid_ = oaid;
        bool update = WriteValueToKvStore(OAID_KVSTORE_KEY, oaid_);
        OAID_HILOGI(OAID_MODULE_SERVICE, "update oaid %{public}s", update == true ? "success" : "failed");
        updateMutex_.unlock();
        return oaid_;
    }
    updateMutex_.unlock();
    bool result = ReadValueFromKvStore(OAID_KVSTORE_KEY, oaidKvStoreStr);
    OAID_HILOGI(OAID_MODULE_SERVICE, "ReadValueFromKvStore %{public}s", result == true ? "success" : "failed");

    if (oaidKvStoreStr != OAID_ALLZERO_STR && !oaidKvStoreStr.empty()) {
        if (oaid_.empty()) {
            oaid_ = oaidKvStoreStr;
            OAID_HILOGI(OAID_MODULE_SERVICE, "The Oaid in the memory is empty, it get oaid from kvdb successfully");
        }
        return oaid_;
    } else {
        if (oaid_.empty()) {
            oaid_ = GetUUID();
            OAID_HILOGI(OAID_MODULE_SERVICE, "The oaid has been regenerated.");
        }
    }
    result = WriteValueToKvStore(OAID_KVSTORE_KEY, oaid_);
    OAID_HILOGI(OAID_MODULE_SERVICE, "WriteValueToKvStore %{public}s", result == true ? "success" : "failed");
    OAID_HILOGI(OAID_MODULE_SERVICE, "Gain OAID Finish.");
    return oaid_;
}

std::string OAIDService::GetOAID()
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "Begin.");

    std::string oaid = GainOAID();

    OAID_HILOGI(OAID_MODULE_SERVICE, "End.");
    return oaid;
}

void OAIDService::ResetOAID()
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "ResetOAID.");
    std::string resetOaid = GetUUID();
    oaid_ = resetOaid;
    bool result = WriteValueToKvStore(OAID_KVSTORE_KEY, resetOaid);
    OAID_HILOGI(OAID_MODULE_SERVICE, "ResetOAID WriteValueToKvStore %{public}s", result == true ? "success" : "failed");
}
}  // namespace Cloud
}  // namespace OHOS
