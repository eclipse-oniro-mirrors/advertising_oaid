/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is: distributed on an "AS is:"BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "oaid_service_proxy.h"
#include "iremote_broker.h"
#include "oaid_common.h"
#include "oaid_service_interface.h"
#include "oaid_service_ipc_interface_code.h"
#include "oaid_iremote_config_observer.h"
#include "oaid_anco_service.h"

namespace OHOS {
namespace Cloud {
using namespace OHOS::HiviewDFX;

OAIDServiceProxy::OAIDServiceProxy(const sptr<IRemoteObject> &object) : IRemoteProxy<IOAIDService>(object)
{}

std::string OAIDServiceProxy::GetOAID()
{
    OAID_HILOGD(OAID_MODULE_CLIENT, "GetOAID Begin.");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    const int32_t NOPERMISSION = 305;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write parcelable");
        return "";
    }

    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "getoaid get remote failed");
        return "";
    }
    int32_t result = remote->SendRequest(static_cast<uint32_t>(OAIDInterfaceCode::GET_OAID), data, reply, option);
    if (result != ERR_NONE) {
        if (result == NOPERMISSION) {
            OAIDError curErrorCode = ERR_PERMISSION_ERROR;
            result = curErrorCode;
            OAID_HILOGE(OAID_MODULE_CLIENT, "Get OAID failed of No permission, error code is: %{public}d", result);
        } else {
            OAIDError curError = ERR_SYSYTEM_ERROR;
            result = curError;
            OAID_HILOGE(OAID_MODULE_CLIENT, "Get OAID failed of System error, error code is: %{public}d", result);
        }
        return "";
    }
    auto oaid = reply.ReadString();

    return oaid;
}

int32_t OAIDServiceProxy::ResetOAID()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write parcelable");
    }

    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "resetoaid get remote failed");
        return ERR_SYSYTEM_ERROR;
    }
    int32_t result = remote->SendRequest(static_cast<uint32_t>(OAIDInterfaceCode::RESET_OAID), data, reply, option);
    if (result != ERR_NONE) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Reset OAID failed, error code is: %{public}d", result);
    }
    OAID_HILOGI(OAID_MODULE_CLIENT, "Reset OAID End.");
    int32_t errorCode = reply.ReadInt32();
    OAID_HILOGI(OAID_MODULE_CLIENT, "Reset OAID End.errorCode = %{public}d", errorCode);
    return errorCode;
}

int32_t OAIDServiceProxy::RegisterObserver(const sptr<IRemoteConfigObserver> &observer)
{
    if (!observer) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Observer is null, error code is: %{public}d", ERR_NULL_POINTER);
        return ERR_NULL_POINTER;
    }
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        OAID_HILOGE(OAID_MODULE_CLIENT,
            "Failed to write RegisterObserver InterfaceToken, error code is: %{public}d",
            ERR_WRITE_PARCEL_FAILED);
        return ERR_WRITE_PARCEL_FAILED;
    }
    if (!data.WriteRemoteObject(observer->AsObject())) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Observer write failed, error code is: %{public}d", ERR_WRITE_PARCEL_FAILED);
        return ERR_WRITE_PARCEL_FAILED;
    }
    std::unique_lock<std::mutex> lock(registerObserverMutex_);
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        OAID_HILOGI(OAID_MODULE_CLIENT, "remote is null");
        return ERR_NULL_POINTER;
    }
    OAID_HILOGD(OAID_MODULE_CLIENT, "RegisterObserver proxy");
    return remote->SendRequest(
        static_cast<uint32_t>(OAIDInterfaceCode::REGISTER_CONTROL_CONFIG_OBSERVER), data, reply, option);
}

bool OAIDServiceProxy::SetAncoSwitchStatus(int32_t userId, const std::string& bundleName,
    const std::string& uid, int32_t status)
{
    OAID_HILOGD(OAID_MODULE_CLIENT, "SetAncoSwitchStatus Begin.");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write parcelable");
        return false;
    }

    if (!data.WriteInt32(userId)) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write userId");
        return false;
    }
    if (!data.WriteString(bundleName)) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write bundleName");
        return false;
    }
    if (!data.WriteString(uid)) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write uid");
        return false;
    }
    if (!data.WriteInt32(status)) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write status");
        return false;
    }

    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "get remote failed");
        return false;
    }

    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(OAIDInterfaceCode::SET_ANCO_SWITCH_STATUS), data, reply, option);
    if (result != ERR_NONE) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "SetAncoSwitchStatus failed, error code is: %{public}d", result);
        return false;
    }

    bool ret = reply.ReadBool();
    OAID_HILOGI(OAID_MODULE_CLIENT, "SetAncoSwitchStatus End, ret = %{public}d", ret);
    return ret;
}

std::vector<AncoSwitchStatusInfo> OAIDServiceProxy::GetAncoSwitchStatus(int32_t userId,
    const std::string& bundleName, const std::string& uid)
{
    OAID_HILOGD(OAID_MODULE_CLIENT, "GetAncoSwitchStatus Begin.");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write parcelable");
        return {};
    }

    if (!data.WriteInt32(userId)) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write userId");
        return {};
    }
    if (!bundleName.empty() && !data.WriteString(bundleName)) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write bundleName");
        return {};
    }
    if (!uid.empty() && !data.WriteString(uid)) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write uid");
        return {};
    }

    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "get remote failed");
        return {};
    }

    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(OAIDInterfaceCode::GET_ANCO_SWITCH_STATUS), data, reply, option);
    if (result != ERR_NONE) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "GetAncoSwitchStatus failed, error code is: %{public}d", result);
        return {};
    }

    int32_t size = reply.ReadInt32();
    std::vector<AncoSwitchStatusInfo> resultVec;
    for (int32_t i = 0; i < size; i++) {
        AncoSwitchStatusInfo info;
        info.userId = reply.ReadInt32();
        info.bundleName = reply.ReadString();
        info.uid = reply.ReadString();
        info.status = reply.ReadInt32();
        resultVec.push_back(info);
    }

    OAID_HILOGI(OAID_MODULE_CLIENT, "GetAncoSwitchStatus End, size = %{public}zu", resultVec.size());
    return resultVec;
}

std::vector<AncoAccessRecordInfo> OAIDServiceProxy::GetAncoAccessRecords(int32_t userId,
    const std::string& bundleName, const std::string& uid)
{
    OAID_HILOGD(OAID_MODULE_CLIENT, "GetAncoAccessRecords Begin.");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write parcelable");
        return {};
    }

    if (!data.WriteInt32(userId)) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write userId");
        return {};
    }
    if (!bundleName.empty() && !data.WriteString(bundleName)) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write bundleName");
        return {};
    }
    if (!uid.empty() && !data.WriteString(uid)) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write uid");
        return {};
    }

    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "get remote failed");
        return {};
    }

    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(OAIDInterfaceCode::GET_ANCO_ACCESS_RECORDS), data, reply, option);
    if (result != ERR_NONE) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "GetAncoAccessRecords failed, error code is: %{public}d", result);
        return {};
    }

    int32_t size = reply.ReadInt32();
    std::vector<AncoAccessRecordInfo> resultVec;
    for (int32_t i = 0; i < size; i++) {
        AncoAccessRecordInfo info;
        info.userId = reply.ReadInt32();
        info.bundleName = reply.ReadString();
        info.uid = reply.ReadString();
        info.time = reply.ReadString();
        info.count = reply.ReadInt32();
        resultVec.push_back(info);
    }

    OAID_HILOGI(OAID_MODULE_CLIENT, "GetAncoAccessRecords End, size = %{public}zu", resultVec.size());
    return resultVec;
}
std::string OAIDServiceProxy::GetAncoOAID()
{
    OAID_HILOGD(OAID_MODULE_CLIENT, "GetAncoOAID Begin.");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write parcelable");
        return "";
    }
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "getAncoOaid get remote failed");
        return "";
    }
    int32_t result = remote->SendRequest(static_cast<uint32_t>(OAIDInterfaceCode::GET_ANCO_OAID), data, reply, option);
    if (result != ERR_NONE) {
        return "";
    }
    auto oaid = reply.ReadString();
    return oaid;
}

int32_t OAIDServiceProxy::InsertAccessRecord(const int32_t userId, const std::string bundleName, const std::string uid)
{
    OAID_HILOGD(OAID_MODULE_CLIENT, "InsertAccessRecord Begin.");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write parcelable");
        return ERR_WRITE_PARCEL_FAILED;
    }

    if (!data.WriteInt32(userId)) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write userId");
        return ERR_WRITE_PARCEL_FAILED;
    }
    if (!data.WriteString(bundleName)) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write bundleName");
        return ERR_WRITE_PARCEL_FAILED;
    }
    if (!data.WriteString(uid)) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "Failed to write uid");
        return ERR_WRITE_PARCEL_FAILED;
    }
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "get remote failed");
        return ERR_NULL_POINTER;
    }
    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(OAIDInterfaceCode::SET_ANCO_ACCESS_RECORDS), data, reply, option);
    if (result != ERR_NONE) {
        OAID_HILOGE(OAID_MODULE_CLIENT, "InsertAccessRecord failed, error code is: %{public}d", result);
        return ERR_SYSYTEM_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    OAID_HILOGI(OAID_MODULE_CLIENT, "InsertAccessRecord End, errorCode = %{public}d", errorCode);
    return errorCode;
}

}  // namespace Cloud
}  // namespace OHOS