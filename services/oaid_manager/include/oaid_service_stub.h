/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_CLOUD_OAID_SERVICE_STUB_H
#define OHOS_CLOUD_OAID_SERVICE_STUB_H

#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <pthread.h>
#include "cJSON.h"
#include "oaid_service_interface.h"
#include "ipc_skeleton.h"
#include "iremote_stub.h"
#include "event_handler.h"
#include "event_runner.h"

namespace OHOS {
namespace Cloud {
class OAIDServiceStub : public IRemoteStub<IOAIDService> {
public:
    OAIDServiceStub();
    virtual ~OAIDServiceStub() override;

    /* *
     * Handle remote request.
     *
     * @param data Input param.
     * @param reply Output param.
     * @param option Message option.
     * @return int32_t, return ERR_OK on success, others on failure.
     */
    int32_t OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option) override;

    int32_t RegisterObserver(const sptr<IRemoteConfigObserver>& observer) override;

private:
    int32_t OnGetOAID(MessageParcel& data, MessageParcel& reply);
    int32_t OnResetOAID(MessageParcel& data, MessageParcel& reply);
    int32_t SendCode(uint32_t code, MessageParcel &data, MessageParcel &reply);
    int32_t HandleRegisterControlConfigObserver(MessageParcel& data, MessageParcel& reply);

    bool CheckPermission(const std::string &permissionName);
    bool CheckSystemApp();
    void ExitIdleState();
    void PostDelayUnloadTask();
    int32_t ValidateResetOAIDPermission(std::string bundleName, MessageParcel &reply);

    std::shared_ptr<AppExecFwk::EventHandler> unloadHandler_;
    std::mutex init_eventHandler_Mutex_;
};
} // namespace Cloud
} // namespace OHOS

#endif // OHOS_CLOUD_OAID_SERVICE_STUB_H