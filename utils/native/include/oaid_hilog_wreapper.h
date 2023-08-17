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

#ifndef OHOS_CLOUD_OAID_HILOG_WRAPPER_H
#define OHOS_CLOUD_OAID_HILOG_WRAPPER_H

#include "hilog/log.h"

namespace OHOS {
namespace Cloud {
// param of log interface, such as OAID_HILOGF.
enum OAIDSubModule {
    OAID_MODULE_INNERKIT = 0,
    OAID_MODULE_CLIENT,
    OAID_MODULE_SERVICE,
    OAID_MODULE_JAVAKIT,  // java kit, defined to avoid repeated use of domain.
    OAID_MODULE_JNI,
    OAID_MODULE_COMMON,
    OAID_MODULE_JS_NAPI,
    OAID_MODULE_TEST,
    OAID_MODULE_BUTT,
};

static constexpr unsigned int OAID_DOMAIN_ID = 0xD004701;

static constexpr OHOS::HiviewDFX::HiLogLabel OAID_MODULE_LABEL[OAID_MODULE_BUTT] = {
    { LOG_CORE, OAID_DOMAIN_ID, "OAIDInnerKit" },
    { LOG_CORE, OAID_DOMAIN_ID, "OAIDClient" },
    { LOG_CORE, OAID_DOMAIN_ID, "OAIDService" },
    { LOG_CORE, OAID_DOMAIN_ID, "OAIDJavaKit" },
    { LOG_CORE, OAID_DOMAIN_ID, "OAIDJni" },
    { LOG_CORE, OAID_DOMAIN_ID, "OAIDCommon" },
    { LOG_CORE, OAID_DOMAIN_ID, "OAIDJSNAPI" },
    { LOG_CORE, OAID_DOMAIN_ID, "OAIDTest" },
};

#define R_FILENAME (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#define R_FORMATED(fmt, ...) "[%{public}s] %{public}s# " fmt, R_FILENAME, __FUNCTION__, ##__VA_ARGS__

// In order to improve performance, do not check the module range.
// Besides, make sure module is less than OAID_MODULE_BUTT.
#define OAID_HILOGF(module, ...) \
    (void)OHOS::HiviewDFX::HiLog::Fatal(OHOS::Cloud::OAID_MODULE_LABEL[module], R_FORMATED(__VA_ARGS__))
#define OAID_HILOGE(module, ...) \
    (void)OHOS::HiviewDFX::HiLog::Error(OHOS::Cloud::OAID_MODULE_LABEL[module], R_FORMATED(__VA_ARGS__))
#define OAID_HILOGW(module, ...) \
    (void)OHOS::HiviewDFX::HiLog::Warn(OHOS::Cloud::OAID_MODULE_LABEL[module], R_FORMATED(__VA_ARGS__))
#define OAID_HILOGI(module, ...) \
    (void)OHOS::HiviewDFX::HiLog::Info(OHOS::Cloud::OAID_MODULE_LABEL[module], R_FORMATED(__VA_ARGS__))
#define OAID_HILOGD(module, ...) \
    (void)OHOS::HiviewDFX::HiLog::Debug(OHOS::Cloud::OAID_MODULE_LABEL[module], R_FORMATED(__VA_ARGS__))
}  // namespace Cloud
}  // namespace OHOS
#endif  // OHOS_CLOUD_OAID_HILOG_WRAPPER_H
