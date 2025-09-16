/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "connect_ads_stub.h"
#include <fstream>
#include <charconv>

namespace OHOS {
namespace Cloud {

using namespace std::chrono;
using OHOS::AAFwk::ExtensionManagerClient;
using OHOS::AAFwk::AbilityConnectionStub;
using OHOS::AppExecFwk::ElementName;
using OHOS::AAFwk::Want;
using OHOS::IRemoteObject;
using OHOS::sptr;

// 静态成员初始化
std::u16string ConnectAdsStub::OAID_INFO_TOKEN = u"";
std::mutex ConnectAdsStub::queueMutex_;
std::mutex ConnectAdsStub::setTokenMutex_;
std::mutex ConnectAdsStub::setOaidMutex_;
std::mutex ConnectAdsStub::stateMutex_;
std::mutex ConnectAdsStub::proxyMutex_;
std::mutex ConnectAdsManager::connectMutex_;
std::int32_t ConnectAdsStub::CODE_OAID = GET_ALLOW_OAID_CODE;

// ConnectAdsStub 实现
void ConnectAdsStub::OnAbilityConnectDone(const ElementName &element,
    const sptr<IRemoteObject> &remoteObject, int resultCode)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "enter OnAbilityConnectDone");
    SetProxy(remoteObject);
    SetConnectionState(ConnectionState::CONNECTED);
    ProcessMessageQueue();
}

void ConnectAdsStub::OnAbilityDisconnectDone(const ElementName &element, int resultCode)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "enter OnAbilityDisconnectDone");
    SetProxy(nullptr);
    SetConnectionState(ConnectionState::DISCONNECTED);
    setCodeOaid(GET_ALLOW_OAID_CODE);
}

sptr<IRemoteObject> ConnectAdsStub::GetRemoteObject()
{
    return GetProxy();
}

sptr<IRemoteObject> ConnectAdsStub::GetProxy() const
{
    std::lock_guard<std::mutex> lock(proxyMutex_);
    return proxy_;
}

void ConnectAdsStub::SetProxy(const sptr<IRemoteObject> &remoteObject)
{
    std::lock_guard<std::mutex> lock(proxyMutex_);
    proxy_ = remoteObject;
}

ConnectionState ConnectAdsStub::GetConnectionState() const
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    return connectionState_;
}

void ConnectAdsStub::SetConnectionState(ConnectionState state)
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    connectionState_ = state;
}

void ConnectAdsStub::AddMessageToQueue(int32_t code)
{
    std::lock_guard<std::mutex> lock(queueMutex_);
    // 使用哈希集合去重
    if (messageSet_.find(code) == messageSet_.end()) {
        messageQueue_.push(code);
        messageSet_.insert(code);
        OAID_HILOGD(OAID_MODULE_SERVICE, "Add message %{public}d to queue", code);
    }
}

void ConnectAdsStub::ProcessMessageQueue()
{
    std::queue<int32_t> tempQueue;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        OAID_HILOGD(OAID_MODULE_SERVICE, "Processing message queue");
        if (messageQueue_.empty()) {
            OAID_HILOGI(OAID_MODULE_SERVICE, "Message queue is empty");
            return;
        }
        std::swap(tempQueue, messageQueue_);
        messageSet_.clear();
    }

    // 检查连接状态
    if (GetConnectionState() != ConnectionState::CONNECTED || GetProxy() == nullptr) {
        OAID_HILOGW(OAID_MODULE_SERVICE, "Cannot process queue - not connected");
        // 将未处理的消息重新放回队列
        std::lock_guard<std::mutex> lock(queueMutex_);
        while (!tempQueue.empty()) {
            int32_t code = tempQueue.front();
            if (messageSet_.find(code) == messageSet_.end()) {
                messageQueue_.push(code);
                messageSet_.insert(code);
            }
            tempQueue.pop();
        }
        return;
    }

    while (!tempQueue.empty()) {
        int32_t code = tempQueue.front();
        tempQueue.pop();
        OAID_HILOGD(OAID_MODULE_SERVICE, "Processing message code=%{public}d", code);
        SendMessage(code);
    }
}

void ConnectAdsStub::DisconnectIfIdle()
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "Disconnecting idle connection");
    if (GetProxy() != nullptr && GetConnectionState() == ConnectionState::CONNECTED) {
        ExtensionManagerClient::GetInstance().DisconnectAbility(this);
        SetConnectionState(ConnectionState::DISCONNECTED);
    }
}

void ConnectAdsStub::SendMessage(int32_t code)
{
    sptr<IRemoteObject> rpcProxy = GetProxy();
    if (GetConnectionState() != ConnectionState::CONNECTED || rpcProxy == nullptr) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "SendMessage failed - not connected");
        AddMessageToQueue(code);
        return;
    }

    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    if (OAID_INFO_TOKEN.empty() || !data.WriteInterfaceToken(OAID_INFO_TOKEN)) {
        OAID_HILOGW(OAID_MODULE_SERVICE, "SendMessage WriteInterfaceToken failed");
        AddMessageToQueue(code);
        return;
    }
    sptr<ADSCallbackStub> callback = new (std::nothrow) ADSCallbackStub();
    if (callback == nullptr) {
        OAID_HILOGW(OAID_MODULE_SERVICE, "Memory allocation failed for ADSCallbackStub");
        AddMessageToQueue(code);
        return;
    }
    if (!data.WriteRemoteObject(callback->AsObject())) {
        OAID_HILOGW(OAID_MODULE_SERVICE, "Callback write failed.");
        AddMessageToQueue(code);
        return;
    }

    OAID_HILOGI(OAID_MODULE_SERVICE, "SendMessage CODE_OAID = %{public}d", code);
    rpcProxy->SendRequest(code, data, reply, option);
    setCodeOaid(GET_ALLOW_OAID_CODE);
}

void ConnectAdsStub::setToken(std::u16string token)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "setToken enter");
    std::lock_guard<std::mutex> lock(setTokenMutex_);
    OAID_INFO_TOKEN = token;
}

void ConnectAdsStub::setCodeOaid(std::int32_t code)
{
    std::lock_guard<std::mutex> lock(setOaidMutex_);
    CODE_OAID = code;
}

// ConnectAdsManager 实现
ConnectAdsManager::~ConnectAdsManager()
{
    DisconnectService();
    OAID_HILOGI(OAID_MODULE_SERVICE, "destructor ConnectAdsManager");
}

ConnectAdsManager* ConnectAdsManager::GetInstance()
{
    static ConnectAdsManager instance;
    return &instance;
}

int32_t ConnectAdsManager::DisconnectService()
{
    std::lock_guard<std::mutex> lock(connectMutex_);
    if (connectObject_) {
        connectObject_->DisconnectIfIdle();
    }
    return 0;
}

Want ConnectAdsManager::getWantInfo()
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "enter getWantInfo ");
    Want connectionWant;
    char pathBuff[PATH_MAX] = {0};
    GetOneCfgFile(OAID_TRUSTLIST_EXTENSION_CONFIG_PATH.c_str(), pathBuff, PATH_MAX);
    char realPath[PATH_MAX] = {0};
    if (realpath(pathBuff, realPath) == nullptr) {
        GetOneCfgFile(OAID_TRUSTLIST_CONFIG_PATH.c_str(), pathBuff, PATH_MAX);
        if (realpath(pathBuff, realPath) == nullptr) {
            OAID_HILOGE(OAID_MODULE_SERVICE, "Parse realpath fail");
            return connectionWant;
        }
    }
    std::ifstream inFile(realPath, std::ios::in);
    if (!inFile.is_open()) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Open file error.");
        return connectionWant;
    }
    std::string fileContent((std::istreambuf_iterator<char>{inFile}), std::istreambuf_iterator<char>{});
    cJSON *root = cJSON_Parse(fileContent.c_str());
    inFile.close();
    if (root == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "ParseJsonFromFile is not in JSON format.");
        return connectionWant;
    }
    cJSON *oaidProviderBundleNameConfig = cJSON_GetObjectItem(root, "providerBundleName");
    if (oaidProviderBundleNameConfig == nullptr || oaidProviderBundleNameConfig->type != cJSON_String) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "not contain providerBundleName node.");
        cJSON_Delete(root);
        return connectionWant;
    }
    cJSON *oaidProviderAbilityNameConfig = cJSON_GetObjectItem(root, "providerAbilityName");
    if (oaidProviderAbilityNameConfig == nullptr || oaidProviderAbilityNameConfig->type != cJSON_String) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "not contain providerAbilityName node.");
        cJSON_Delete(root);
        return connectionWant;
    }
    cJSON *oaidProviderTokenNameConfig = cJSON_GetObjectItem(root, "providerTokenName");
    if (oaidProviderTokenNameConfig == nullptr || oaidProviderTokenNameConfig->type != cJSON_String
        || oaidProviderTokenNameConfig->valuestring == nullptr) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "not contain providerTokenName node.");
        cJSON_Delete(root);
        return connectionWant;
    }
    ConnectAdsStub::setToken(Str8ToStr16(oaidProviderTokenNameConfig->valuestring));
    connectionWant.SetElementName(oaidProviderBundleNameConfig->valuestring,
        oaidProviderAbilityNameConfig->valuestring);
    cJSON_Delete(root);
    return connectionWant;
}

bool ConnectAdsManager::checkAllowGetOaid()
{
    DistributedKv::Value allowGetOaid;
    DistributedKv::Value updateTime;
    OAIDService::GetInstance()->ReadValueFromUnderAgeKvStore(ALLOW_GET_OAID_KEY, allowGetOaid);
    OAIDService::GetInstance()->ReadValueFromUnderAgeKvStore(LAST_UPDATE_TIME_KEY, updateTime);
    if (allowGetOaid == nullptr || updateTime == nullptr) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "checkAllowGetOaid get kvData failed");
        ConnectAdsManager::GetInstance()->notifyKit(GET_ALLOW_OAID_CODE);
        return true;
    }
    std::string updateTimeStr = updateTime.ToString();
    long long updateTimestamp = 0;
    // 使用 std::from_chars 转换字符串为 long long
    auto [ptr, ec] = std::from_chars(
        updateTimeStr.data(),
        updateTimeStr.data() + updateTimeStr.size(),
        updateTimestamp);
    // 检查转换是否成功
    if (ec != std::errc() || ptr != updateTimeStr.data() + updateTimeStr.size()) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to convert timestamp: invalid or out of range");
        return true;
    }
    long long nowTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (nowTimestamp < updateTimestamp) {
        OAID_HILOGW(OAID_MODULE_SERVICE, "user time illegal");
    } else {
        long long interval = nowTimestamp - updateTimestamp;
        OAID_HILOGD(OAID_MODULE_SERVICE,
            "checkAllowGetOaid now = %{public}lld  updateTime = %{public}s  interval = %{public}lld, ",
            nowTimestamp,
            updateTimeStr.c_str(),
            interval);
        if (interval >= EXPIRATION_TIME) {
            ConnectAdsManager::GetInstance()->notifyKit(GET_ALLOW_OAID_CODE);
        }
    }
    if (allowGetOaid.ToString() == "true") {
        return true;
    } else {
        return false;
    }
}

int ADSCallbackStub::OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "OnRemoteRequest enter");
    int32_t respCode = data.ReadInt32();
    OAID_HILOGI(OAID_MODULE_SERVICE, "OnRemoteRequest respCode = %{public}d", respCode);
    std::string isAllowGetOaid = Str16ToStr8(data.ReadString16());
    std::string updateTimeStr = Str16ToStr8(data.ReadString16());
    OAID_HILOGI(OAID_MODULE_SERVICE, "isAllowGetOaid = %{public}s, updateTimeStr = %{public}s", isAllowGetOaid.c_str(),
        updateTimeStr.c_str());
    if (isAllowGetOaid.empty() || updateTimeStr.empty()) {
        OAID_HILOGI(OAID_MODULE_SERVICE, "OnRemoteRequest return info is empty");
        ConnectAdsManager::GetInstance()->DisconnectService();
        return ERR_OK;
    }
    DistributedKv::Value value1(isAllowGetOaid);
    DistributedKv::Value value2(updateTimeStr);
    bool allowGetOaidResult = OAIDService::GetInstance()->WriteValueToUnderAgeKvStore(ALLOW_GET_OAID_KEY, value1);
    bool lastUpdateTimeResult = OAIDService::GetInstance()->WriteValueToUnderAgeKvStore(LAST_UPDATE_TIME_KEY, value2);
    OAID_HILOGI(OAID_MODULE_SERVICE,
        "OnRemoteRequest Write AllowGetOaid result=%{public}s.OnRemoteRequest Write lastUpdateTime result=%{public}s",
        allowGetOaidResult == true ? "success" : "failed",
        lastUpdateTimeResult == true ? "success" : "failed");
    ConnectAdsManager::GetInstance()->DisconnectService();
    return ERR_OK;
}

void ConnectAdsManager::notifyKit(int32_t code)
{
    OAID_HILOGD(OAID_MODULE_SERVICE, "enter notifyKit = %{public}d", code);
    ConnectionState currentState = connectObject_->GetConnectionState();
    // 待发送的消息放到队列中，连接成功后处理队列消息
    connectObject_->AddMessageToQueue(code);
    if (currentState == ConnectionState::CONNECTED) {
        OAID_HILOGD(OAID_MODULE_SERVICE, "already connected, process message queue");
        connectObject_->ProcessMessageQueue();
        return;
    }

    if (currentState == ConnectionState::DISCONNECTED) {
        std::lock_guard<std::mutex> lock(connectMutex_);
        // 再次检查状态，防止竞态条件
        if (connectObject_->GetConnectionState() == ConnectionState::DISCONNECTED) {
            OAID_HILOGD(OAID_MODULE_SERVICE, "not connected");
            connectObject_->SetConnectionState(ConnectionState::CONNECTING);
            Want want = getWantInfo();
            if (code == NOTIFY_RESET_OAID_CODE) {
                ConnectAdsStub::setCodeOaid(code);
                want.SetParam("code_oaid", code);
            }
            int32_t resultNumber = ExtensionManagerClient::GetInstance().ConnectServiceExtensionAbility(
                want, connectObject_, nullptr, DEFAULT_VALUE);
            if (resultNumber != ERR_OK) {
                connectObject_->SetConnectionState(ConnectionState::DISCONNECTED);
                OAID_HILOGI(OAID_MODULE_SERVICE, "failed to ConnectToAds");
            }
        }
    } else {
        OAID_HILOGD(OAID_MODULE_SERVICE, "connection in progress, message added to queue");
    }
}

sptr<ConnectAdsStub> ConnectAdsManager::getConnection()
{
    return connectObject_;
}

ConnectAdsManager::ConnectAdsManager()
{
    connectObject_ = sptr<ConnectAdsStub>(new ConnectAdsStub);
    connectObject_->SetConnectionState(ConnectionState::DISCONNECTED);
    OAID_HILOGI(OAID_MODULE_SERVICE, "constructor ConnectAdsManager");
}

} // namespace Cloud
} // namespace OHOS
