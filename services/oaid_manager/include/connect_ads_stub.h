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
using OHOS::AAFwk::ExtensionManagerClient;
using OHOS::AAFwk::AbilityConnectionStub;
using OHOS::AppExecFwk::ElementName;
using OHOS::AAFwk::Want;
using OHOS::IRemoteObject;
using OHOS::sptr;

namespace OHOS {
namespace Cloud {
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

class ConnectAdsStub : public AbilityConnectionStub {
public:

    void OnAbilityConnectDone(const ElementName &element, const sptr<IRemoteObject> &remoteObject,
                              int resultCode) override
    {
        OAID_HILOGI(OAID_MODULE_SERVICE, "enter OnAbilityConnectDone");
        proxy_ = remoteObject;
        SendMessage();
    }

    void OnAbilityDisconnectDone(const ElementName &element, int resultCode) override
    {
        OAID_HILOGI(OAID_MODULE_SERVICE, "enter OnAbilityDisconnectDone");
        proxy_ = nullptr;
    }

    sptr<IRemoteObject> GetRemoteObject()
    {
        return proxy_;
    }

    void SendMessage()
    {
        OAID_HILOGI(OAID_MODULE_SERVICE, "SendMessage enter");
        if (proxy_ == nullptr) {
            return;
        }
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;
        std::lock_guard<std::mutex> lock(checkMutex_);
        if (OAID_INFO_TOKEN.empty() || !data.WriteInterfaceToken(OAID_INFO_TOKEN)) {
            OAID_HILOGW(OAID_MODULE_SERVICE, "SendMessage WriteInterfaceToken failed");
            return;
        }
        auto callback = new ADSCallbackStub();
        if (!data.WriteRemoteObject(callback->AsObject())) {
            OAID_HILOGW(OAID_MODULE_SERVICE, "Callback write failed.");
            return;
        }
        proxy_->SendRequest(CODE_ALLOW_GET_OAID, data, reply, option);
        OAID_HILOGI(OAID_MODULE_SERVICE, "SendMessage finished");
    }

    static void setToken(std::u16string token)
    {
        OAID_HILOGI(OAID_MODULE_SERVICE, "setToken enter");
        std::lock_guard<std::mutex> lock(checkMutex_);
        OAID_INFO_TOKEN = token;
    }

private:
    sptr<IRemoteObject> proxy_;
    static std::u16string OAID_INFO_TOKEN;
    static std::mutex checkMutex_;
};
std::u16string ConnectAdsStub::OAID_INFO_TOKEN = u"";
std::mutex ConnectAdsStub::checkMutex_;

class ConnectAdsManager {
public:

    ConnectAdsManager(ConnectAdsManager &) = delete;
    ConnectAdsManager& operator=(const ConnectAdsManager&)=delete;

    ~ConnectAdsManager()
    {
        DisconnectService();
        OAID_HILOGI(OAID_MODULE_SERVICE, "destructor ConnectAdsManager");
    }

    static ConnectAdsManager* GetInstance()
    {
        static ConnectAdsManager instance;
        return &instance;
    }

    bool ConnectToAds(Want want)
    {
        OAID_HILOGI(OAID_MODULE_SERVICE, "enter ConnectToAds isConnect=%{public}d", isConnect);
        if (!isConnect) {
            OAID_HILOGI(OAID_MODULE_SERVICE, "start ConnectToAds ");
            int32_t resultNumber = ExtensionManagerClient::GetInstance().ConnectServiceExtensionAbility(want,
                connectObject_, nullptr, DEFAULT_VALUE);
            OAID_HILOGI(OAID_MODULE_SERVICE, "ConnectToAds result=%{public}d", resultNumber);
            if (resultNumber != ERR_OK) {
                OAID_HILOGI(OAID_MODULE_SERVICE, " failed to ConnectToAds ability");
                return false;
            }
            isConnect = true;
        }
        return true;
    }

    int32_t DisconnectService()
    {
        OAID_HILOGI(OAID_MODULE_SERVICE, "enter DisconnectService isConnect=%{public}d", isConnect);
        if (isConnect) {
            OAID_HILOGI(OAID_MODULE_SERVICE, "start DisconnectService ");
            ExtensionManagerClient::GetInstance().DisconnectAbility(connectObject_);
            isConnect = false;
        }
        return 0;
    }

    Want getWantInfo();

    bool checkAllowGetOaid();

    static void getAllowGetOAIDFromKit()
    {
        OAID_HILOGI(OAID_MODULE_SERVICE, "getAllowGetOAIDFromKit isConnect = %{public}d", isConnect);
        if (!isConnect) {
            OAID_HILOGI(OAID_MODULE_SERVICE, "getAllowGetOAIDFromKit enter ConnectToAds");
            Want want = ConnectAdsManager::GetInstance()->getWantInfo();
            ConnectAdsManager::GetInstance()->ConnectToAds(want);
        }
    }

    sptr<ConnectAdsStub> getConnection()
    {
        return connectObject_;
    }

private:
    sptr<ConnectAdsStub> connectObject_;
    int32_t DEFAULT_VALUE = -1;
    static bool isConnect;
    ConnectAdsManager()
    {
        connectObject_ = sptr<ConnectAdsStub>(new ConnectAdsStub);
        OAID_HILOGI(OAID_MODULE_SERVICE, "constructor ConnectAdsManager");
    }
};
bool ConnectAdsManager::isConnect = false;

} // namespace Cloud
} // namespace OHOS
#endif //PPSOPENHARMONYSA_OPEN_CONNECT_ADS_STUB_H
