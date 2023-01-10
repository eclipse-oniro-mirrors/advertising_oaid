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
const std::string OAID_JSON_PATH = "/data/service/el1/public/oaid/ohos_oaid.json";
const std::string OAID_NULL_STR = "00000000-0000-0000-0000-000000000000";

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

std::string GetOAIDFromFile(const std::string &filePath)
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

bool SaveOAIDToFile(const std::string &filePath, std::string oaid)
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

std::string OAIDService::GetOAID()
{
    std::string path = OAID_JSON_PATH;
    std::string oaid = OAID_NULL_STR;

    OAID_HILOGI(OAID_MODULE_SERVICE, "Begin.");

    if (!oaid_.empty()) {
        oaid = oaid_;
        OAID_HILOGW(OAID_MODULE_SERVICE, "The Oaid in the memory is not empty");
    }

    bool isReSaveOaidToFile = false;
    bool isReGenerateOaid = false;

    std::string oaidFromFile = GetOAIDFromFile(path);
    if (!oaidFromFile.empty() && oaidFromFile != OAID_NULL_STR) {
        if (!oaid_.empty()) {
            if (oaid_.compare(oaidFromFile)) {
                OAID_HILOGW(OAID_MODULE_SERVICE, "get oaid from file successfully, but oaidFromFile is incorrect");
                oaidFromFile = oaid_;
                isReSaveOaidToFile = true;
            } else {
                OAID_HILOGW(OAID_MODULE_SERVICE, "get oaid from file successfully");
                return oaid;
            }
        } else {
            oaid = oaidFromFile;
            oaid_ = oaidFromFile;
            OAID_HILOGW(OAID_MODULE_SERVICE, "The Oaid in the memory is empty,But it get oaid from file successfully");
            return oaid;
        }
    } else {
        if (!oaid_.empty()) {
            OAID_HILOGW(OAID_MODULE_SERVICE, "The Oaid in the file is empty.");
            oaidFromFile = oaid_;
            isReSaveOaidToFile = true;
        } else {
            isReGenerateOaid = true;
        }
    }

    if (isReGenerateOaid) {
        OAID_HILOGW(OAID_MODULE_SERVICE, "The oaid needs to be regenerated.");
        oaid = GetUUID();
        oaid_ = oaid;
    } else {
        oaid = oaidFromFile;
    }

    bool ret = SaveOAIDToFile(path, oaid);
    if (!ret) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Save oaid failed");
    }

    OAID_HILOGI(OAID_MODULE_SERVICE, "End.");
    return oaid;
}
}  // namespace Cloud
}  // namespace OHOS
