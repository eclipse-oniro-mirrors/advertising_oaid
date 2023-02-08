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
#include "iremote_proxy.h"
#include "iservice_registry.h"
#include "config_policy_utils.h"
#include "json/json.h"
#include "oaid_common.h"
#include "pthread.h"
#include "system_ability.h"
#include "system_ability_definition.h"
#include "uri.h"
#include "oaid_service_stub.h"

using namespace std::chrono;

namespace OHOS {
namespace Cloud {
namespace {
static const int32_t OAID_SYSTME_ID = 6101;  // The system component ID of the OAID is 6101.
static constexpr uint32_t KVSTORE_CONNECT_RETRY_COUNT = 5;
static constexpr uint32_t KVSTORE_CONNECT_RETRY_DELAY_TIME = 3000;
static const int8_t CONNECT_TIME_OUT = 3;    // The connection timeout is 3s.
static const int8_t INVOKE_CODE_SEND_REMOTE_OBJECT = 10;
static const int SYSTEM_UERID = 100;

static const std::string DEPENDENCY_CONFIG_FILE_RELATIVE_PATH = "etc/cloud/oaid/oaid_service_config.json";
static const std::string ADS_SA_SERVICE_ACTION = "com.ohos.cloudservice.identifier.oaid";

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

sptr<IBundleMgr> GetBundleService()
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        return nullptr;
    }
    auto object = samgr->CheckSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (object == nullptr) {
        return nullptr;
    }

    sptr<IBundleMgr> bundleMgr = iface_cast<IBundleMgr>(object);
    OAID_HILOGI(OAID_MODULE_SERVICE, "GetBundleService success.");
    return bundleMgr;
}

bool SendOAIDSARemoteObjectToService(const sptr<IRemoteObject>& remoteObject,
    const sptr<IRemoteObject>& oaidServiceStubProxy)
{
    if (remoteObject == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "remoteObject is null.");
        return false;
    }

    MessageParcel data, reply;
    MessageOption option;

    if (oaidServiceStubProxy == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "oaidServiceStubProxy is nullptr");
        return false;
    }

    if (!data.WriteRemoteObject(oaidServiceStubProxy)) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "failed to write remote object");
        return false;
    }

    int error = remoteObject->SendRequest(INVOKE_CODE_SEND_REMOTE_OBJECT, data, reply, option);
    if (error != ERR_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "SendRequest to cloud service failed.");
        return false;
    }

    std::string retStr = Str16ToStr8(reply.ReadString16().c_str());
    OAID_HILOGI(OAID_MODULE_SERVICE, "Service reply retStr is %{public}s.", retStr.c_str());
    if (!strcmp(retStr.c_str(), "success")) {
        return true;
    }

    return false;
}

bool GetCloudServiceProvider(std::vector<OAIDService::CloudServiceProvider>& cloudServiceProviders)
{
    char pathBuff[MAX_PATH_LEN];
    GetOneCfgFile(DEPENDENCY_CONFIG_FILE_RELATIVE_PATH.c_str(), pathBuff, MAX_PATH_LEN);
    OAID_HILOGD(OAID_MODULE_SERVICE, "Config path is %{public}s", pathBuff);
    std::ifstream ifs;
    ifs.open(pathBuff);
    if (!ifs) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Open file error.");
        return false;
    }

    Json::Value jsonValue;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = true;
    JSONCPP_STRING errs;
    if (!parseFromStream(builder, ifs, &jsonValue, &errs)) {
        ifs.close();
        OAID_HILOGE(OAID_MODULE_SERVICE, "Read file failed %{public}s.", errs.c_str());
        return false;
    }

    Json::Value cloudServiceProvidersJson = jsonValue["providerBundleName"];
    for (unsigned int i = 0; i < cloudServiceProvidersJson.size(); i++) {
        if (cloudServiceProvidersJson[i].asString().empty()) {
            continue;
        }
        OAIDService::CloudServiceProvider cloudServiceProvider;
        cloudServiceProvider.bundleName = cloudServiceProvidersJson[i].asString();
        cloudServiceProviders.push_back(cloudServiceProvider);
    }
    ifs.close();

    OAID_HILOGI(
        OAID_MODULE_SERVICE, "Cloud Service provider from config length is %{public}lu.", cloudServiceProviders.size());
    return true;
}

bool GetExtensionAbility(OAIDService::CloudServiceProvider& cloudServiceProvider)
{
    Want cloudServiceWant;
    cloudServiceWant.SetBundle(cloudServiceProvider.bundleName);
    cloudServiceWant.SetAction(ADS_SA_SERVICE_ACTION);
    std::vector<AppExecFwk::ExtensionAbilityInfo> extensionInfos;

    sptr<IBundleMgr> bundleMgr = GetBundleService();
    if (bundleMgr == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "GetBundleService is null.");
        return false;
    }
    bool retBundle = bundleMgr->QueryExtensionAbilityInfos(
        cloudServiceWant, ExtensionAbilityInfoFlag::GET_EXTENSION_INFO_DEFAULT, SYSTEM_UERID, extensionInfos);
    if (retBundle && !extensionInfos.empty() && !extensionInfos[0].name.empty()) {
        OAID_HILOGD(OAID_MODULE_SERVICE,
            "Cloud Service provider extension exist, bundleName is %{public}s, extension name is %{public}s ",
            cloudServiceProvider.bundleName.c_str(), extensionInfos[0].name.c_str());
        cloudServiceProvider.extensionName = extensionInfos[0].name;
        cloudServiceProvider.userId = SYSTEM_UERID;
        return true;
    }

    return false;
}

int64_t GetInstallTime(OAIDService::CloudServiceProvider& cloudServiceProvider)
{
    sptr<IBundleMgr> bundleMgr = GetBundleService();
    if (bundleMgr == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "GetBundleService is null.");
        return INT64_MAX;
    }
    BundleInfo bundleInfo;
    bool ret = bundleMgr->GetBundleInfo(
        cloudServiceProvider.bundleName, BundleFlag::GET_BUNDLE_DEFAULT, bundleInfo, cloudServiceProvider.userId);
    if (!ret) {
        return INT64_MAX;
    }
    OAID_HILOGD(OAID_MODULE_SERVICE, "Cloud Service provider installtime is %{public}ld.", bundleInfo.installTime);
    return bundleInfo.installTime;
}

bool CloudServiceExist(OAIDService::CloudServiceProvider& cloudServiceProvider)
{
    std::vector<OAIDService::CloudServiceProvider> cloudServiceProviders;
    GetCloudServiceProvider(cloudServiceProviders);
    for (std::vector<OAIDService::CloudServiceProvider>::iterator it = cloudServiceProviders.begin();
         it != cloudServiceProviders.end();) {
        if (!GetExtensionAbility(*it)) {
            it = cloudServiceProviders.erase(it);
        } else {
            it++;
        }
    }

    OAID_HILOGI(
        OAID_MODULE_SERVICE, "CloudServiceProvider Filter action size is %{public}lu", cloudServiceProviders.size());
    if (cloudServiceProviders.size() > 1) {
        int64_t installTime = INT64_MAX;
        std::vector<OAIDService::CloudServiceProvider> minInstallTimeProviders;
        for (size_t i = 0; i < cloudServiceProviders.size(); i++) {
            int64_t installTimeCurr = GetInstallTime(cloudServiceProviders[i]);
            if (installTimeCurr < installTime) {
                installTime = installTimeCurr;
                minInstallTimeProviders.clear();
                minInstallTimeProviders.push_back(cloudServiceProviders[i]);
            } else if (installTimeCurr == installTime) {
                minInstallTimeProviders.push_back(cloudServiceProviders[i]);
            } else {
                OAID_HILOGI(OAID_MODULE_SERVICE, "Cloud Service  installTimeCurr > installTime.");
            }
        }
        cloudServiceProviders = minInstallTimeProviders;
    }

    OAID_HILOGI(
        OAID_MODULE_SERVICE, "CloudServiceProvider Filter InstallTime size %{public}lu", cloudServiceProviders.size());
    if (cloudServiceProviders.size() == 0) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Cloud Service provider is empty.");
        return false;
    }

    std::sort(cloudServiceProviders.begin(), cloudServiceProviders.end(),
        [](OAIDService::CloudServiceProvider a, OAIDService::CloudServiceProvider b) {
            return a.bundleName.compare(b.bundleName) < 0;
        });

    cloudServiceProvider = cloudServiceProviders[0];

    return true;
}
}  // namespace

REGISTER_SYSTEM_ABILITY_BY_ID(OAIDService, OAID_SYSTME_ID, true);
std::mutex OAIDService::instanceLock_;
sptr<OAIDService> OAIDService::instance_;

class CloudConnection : public AbilityConnectionStub {
public:
    void OnAbilityConnectDone(
        const AppExecFwk::ElementName& element, const sptr<IRemoteObject>& remoteObject, int resultCode) override
    {
        std::string uri = element.GetURI();
        OAID_HILOGI(OAID_MODULE_SERVICE, "Connected uri is %{public}s, result is %{public}d.", uri.c_str(), resultCode);
        if (resultCode != ERR_OK) {
            return;
        }
        OAIDService::GetInstance()->notifyConnected(element, remoteObject);
    }

    void OnAbilityDisconnectDone(const AppExecFwk::ElementName& element, int) override
    {
        std::string uri = element.GetURI();
        OAID_HILOGI(OAID_MODULE_SERVICE, "Disconnected uri is %{public}s.", uri.c_str());
        OAIDService::GetInstance()->notifyDisConnected(element);
    }
};

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
    cloudConnection_ = nullptr;
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

bool OAIDService::TryConnectCloud(CloudServiceProvider& cloudServiceProvider)
{
    std::unique_lock<std::mutex> uniqueLock(connectMutex_);
    if (connectServiceReady_) {
        return true;
    }

    OAID_HILOGI(OAID_MODULE_SERVICE,
        "Begin connect extension ability, bundleName is %{public}s, extension name is %{public}s, userId is %{public}d",
        cloudServiceProvider.bundleName.c_str(), cloudServiceProvider.extensionName.c_str(),
        cloudServiceProvider.userId);
    Want connectionWant;
    connectionWant.SetElementName(cloudServiceProvider.bundleName, cloudServiceProvider.extensionName);
    if (cloudConnection_ == nullptr) {
        cloudConnection_ = new (std::nothrow) CloudConnection();
    }
    int32_t ret = AbilityManagerClient::GetInstance()->ConnectAbility(
        connectionWant, cloudConnection_, cloudServiceProvider.userId);
    if (ret != ERR_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Invoke connect ability failed.");
        return false;
    }

    auto waitStatus = connectCondition_.wait_for(
        uniqueLock, std::chrono::seconds(CONNECT_TIME_OUT), [this]() { return connectServiceReady_; });
    if (!waitStatus) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Connect cloudService failed.");
    }
    return waitStatus;
}

bool OAIDService::BeginDisConnectCloud()
{
    std::unique_lock<std::mutex> uniqueLock(connectMutex_);
    if (!connectServiceReady_) {
        return true;
    }

    if (cloudConnection_ == nullptr) {
        OAID_HILOGW(OAID_MODULE_SERVICE, "cloudConnection_ is null.");
        return false;
    }
    OAID_HILOGI(OAID_MODULE_SERVICE, "Begin disconnect ability.");
    ErrCode ret = AbilityManagerClient::GetInstance()->DisconnectAbility(cloudConnection_);
    if (ret != ERR_OK) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Invoke disconnect ability failed.");
        return false;
    }

    auto waitStatus = connectCondition_.wait_for(
        uniqueLock, std::chrono::seconds(CONNECT_TIME_OUT), [this]() { return cloudServiceProxy_ == nullptr; });
    if (!waitStatus) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Disconnect cloud service failed.");
    }
    return waitStatus;
}

void OAIDService::checkLastCloudServce(CloudServiceProvider& cloudServiceProvider)
{
    std::lock_guard<std::mutex> autoLock(instanceLock_);
    if (cloudServiceProvider.bundleName == currCloudServiceProvider_.bundleName &&
        cloudServiceProvider.extensionName == currCloudServiceProvider_.extensionName &&
        cloudServiceProvider.userId == currCloudServiceProvider_.userId) {
        return;
    }

    if (BeginDisConnectCloud()) {
        currCloudServiceProvider_ = cloudServiceProvider;
        return;
    }

    clearConnect();
    currCloudServiceProvider_ = cloudServiceProvider;
}

std::string OAIDService::GetOAID()
{
    std::string path = OAID_JSON_PATH;
    std::string oaid;

    OAID_HILOGI(OAID_MODULE_SERVICE, "Begin.");

    CloudServiceProvider cloudServiceProvider;
    if (CloudServiceExist(cloudServiceProvider)) {
        checkLastCloudServce(cloudServiceProvider);
        TryConnectCloud(cloudServiceProvider);
        bool ret = SendOAIDSARemoteObjectToService(cloudServiceProxy_, OAIDServiceStub::GainOAIDServiceStubProxy());
        OAID_HILOGW(OAID_MODULE_SERVICE, "SendOAIDSARemoteObjectToService ret=%{public}d", ret);
    }

    std::string oaidKvStoreStr = OAID_NULL_STR;

    bool result = ReadValueFromKvStore(OAID_KVSTORE_KEY, oaidKvStoreStr);
    OAID_HILOGI(OAID_MODULE_SERVICE, "ReadValueFromKvStore %{public}s", result == true ? "success" : "failed");

    if (oaidKvStoreStr != OAID_NULL_STR) {
        if (!oaid_.empty()) {
            oaid = oaid_;
            OAID_HILOGI(OAID_MODULE_SERVICE, "get oaid from kvdb successfully");
        } else {
            oaid = oaidKvStoreStr;
            oaid_ = oaidKvStoreStr;
            OAID_HILOGW(OAID_MODULE_SERVICE, "The Oaid in the memory is empty,it get oaid from kvdb successfully");
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

void OAIDService::notifyConnected(const AppExecFwk::ElementName& element, const sptr<IRemoteObject>& remoteObject)
{
    if (element.GetBundleName() != currCloudServiceProvider_.bundleName ||
        element.GetAbilityName() != currCloudServiceProvider_.extensionName) {
        return;
    }

    std::unique_lock<std::mutex> uniqueLock(connectMutex_);
    connectServiceReady_ = true;
    cloudServiceProxy_ = remoteObject;
    connectCondition_.notify_all();
}

void OAIDService::notifyDisConnected(const AppExecFwk::ElementName& element)
{
    if (element.GetBundleName() != currCloudServiceProvider_.bundleName ||
        element.GetAbilityName() != currCloudServiceProvider_.extensionName) {
        return;
    }

    clearConnect();
    connectCondition_.notify_all();
}

void OAIDService::clearConnect()
{
    std::unique_lock<std::mutex> uniqueLock(connectMutex_);
    connectServiceReady_ = false;
    cloudServiceProxy_ = nullptr;
}
}  // namespace Cloud
}  // namespace OHOS
