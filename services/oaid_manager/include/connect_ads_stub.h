/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef PPSOPENHARMONYSA_OPEN_CONNECT_ADS_STUB_H
#define PPSOPENHARMONYSA_OPEN_CONNECT_ADS_STUB_H

#include "extension_manager_client.h"
#include "ability_connect_callback_stub.h"
#include "oaid_common.h"
#include "oaid_service_define.h"
#include "config_policy_utils.h"
#include "distributed_kv_data_manager.h"
#include "oaid_service.h"
#include "iremote_broker.h"
#include "iremote_stub.h"
#include "message_parcel.h"
#include "message_option.h"
#include "oaid_file_operator.h"
#include "cJSON.h"
#include <mutex>
#include <queue>
#include <unordered_set>

namespace OHOS {
namespace Cloud {

// 连接状态枚举
enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED
};
class AdsCallback : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.cloud.oaid.AdsCallback");
};

class ADSCallbackStub : public IRemoteStub<AdsCallback> {
public:
    DISALLOW_COPY_AND_MOVE(ADSCallbackStub);
    ADSCallbackStub() = default;
    virtual ~ADSCallbackStub() = default;

    int OnRemoteRequest(
            uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option) override;
};

class ConnectAdsStub : public AAFwk::AbilityConnectionStub {
public:
    void OnAbilityConnectDone(const AppExecFwk::ElementName &element,
                             const sptr<IRemoteObject> &remoteObject,
                             int resultCode) override;
    void OnAbilityDisconnectDone(const AppExecFwk::ElementName &element,
                                int resultCode) override;

    sptr<IRemoteObject> GetRemoteObject();
    ConnectionState GetConnectionState() const;
    void SetConnectionState(ConnectionState state);
    sptr<IRemoteObject> GetProxy() const;
    void SetProxy(const sptr<IRemoteObject> &remoteObject);
    void AddMessageToQueue(int32_t code);
    void ProcessMessageQueue();
    void DisconnectIfIdle();
    void SendMessage(int32_t code);

    static void setToken(std::u16string token);
    static void setCodeOaid(std::int32_t code);

private:
    sptr<IRemoteObject> proxy_;
    ConnectionState connectionState_ = ConnectionState::DISCONNECTED;
    std::queue<int32_t> messageQueue_;
    std::unordered_set<int32_t> messageSet_;
    static std::u16string OAID_INFO_TOKEN;
    static std::mutex queueMutex_;
    static std::int32_t CODE_OAID;
    static std::mutex setTokenMutex_;
    static std::mutex setOaidMutex_;
    static std::mutex stateMutex_;
    static std::mutex proxyMutex_;
};

class ConnectAdsManager {
public:
    static ConnectAdsManager* GetInstance();
    int32_t DisconnectService();
    AAFwk::Want getWantInfo();
    bool checkAllowGetOaid();
    void notifyKit(int32_t code);
    sptr<ConnectAdsStub> getConnection();

private:
    ConnectAdsManager();
    ~ConnectAdsManager();
    ConnectAdsManager(const ConnectAdsManager&) = delete;
    ConnectAdsManager& operator=(const ConnectAdsManager&) = delete;

    static std::mutex connectMutex_;
    sptr<ConnectAdsStub> connectObject_;
    int32_t DEFAULT_VALUE = -1;
};

} // namespace Cloud
} // namespace OHOS
#endif //PPSOPENHARMONYSA_OPEN_CONNECT_ADS_STUB_H
