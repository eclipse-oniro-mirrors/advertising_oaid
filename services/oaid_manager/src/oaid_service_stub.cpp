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
#include "oaid_service_stub.h"

#include <singleton.h>

#include "oaid_common.h"
#include "bundle_mgr_client.h"

namespace OHOS {
namespace Cloud {
using namespace OHOS::HiviewDFX;

OAIDServiceStub::OAIDServiceStub()
{
    memberFuncMap_[GET_OAID] = &OAIDServiceStub::OnGetOAID;
}

OAIDServiceStub::~OAIDServiceStub()
{
    memberFuncMap_.clear();
}

int32_t OAIDServiceStub::OnRemoteRequest(
    uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "Start, code is %{public}u.", code);
    std::u16string myDescripter = OAIDServiceStub::GetDescriptor();
    std::u16string remoteDescripter = data.ReadInterfaceToken();
    if (myDescripter != remoteDescripter) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Descriptor checked fail.");
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    pid_t uid = IPCSkeleton::GetCallingUid();
    std::string bundleName;
    std::shared_ptr<AppExecFwk::BundleMgrClient> client =
        OHOS::DelayedSingleton<AppExecFwk::BundleMgrClient>::GetInstance();
    client->GetBundleNameForUid(static_cast<int>(uid), bundleName);
    OAID_HILOGI(OAID_MODULE_SERVICE, "Remote bundleName is %{public}s.", bundleName.c_str());
    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }

    int32_t ret = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    OAID_HILOGE(OAID_MODULE_SERVICE, "No find process to handle, ret is %{public}d.", ret);
    return ret;
}

int32_t OAIDServiceStub::OnGetOAID(MessageParcel& data, MessageParcel& reply)
{
    OAID_HILOGI(OAID_MODULE_SERVICE, "Start.");

    auto oaid = GetOAID();
    if (oaid == "") {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Get OAID failed.");
        return ERR_SYSYTEM_ERROR;
    }
    if (!reply.WriteString(oaid)) {
        OAID_HILOGE(OAID_MODULE_SERVICE, "Failed to write parcelable.");
        return ERR_SYSYTEM_ERROR;
    }
    OAID_HILOGI(OAID_MODULE_SERVICE, "End.");
    return ERR_OK;
}
} // namespace Cloud
} // namespace OHOS