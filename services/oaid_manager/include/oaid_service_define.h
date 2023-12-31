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

#ifndef OHOS_CLOUD_OAID_SERVICE_DEFINE_H
#define OHOS_CLOUD_OAID_SERVICE_DEFINE_H

#include <string>

namespace OHOS {
namespace Cloud {
/* The package management API define */
static const int32_t OHOS_API_VERSION = 10;

/* getOAID permission define */
static const std::string OAID_TRACKING_CONSENT_PERMISSION = "ohos.permission.APP_TRACKING_CONSENT";

/* The system component ID of the OAID is 6101. */
static const int32_t OAID_SYSTME_ID = 6101;

/* communication settings define */
static constexpr uint32_t KVSTORE_CONNECT_RETRY_COUNT = 5;
static constexpr uint32_t KVSTORE_CONNECT_RETRY_DELAY_TIME = 3000; // Unit: ms
static const int8_t CONNECT_TIME_OUT = 3;    // The connection timeout is 3s.

static const std::string OAID_ALLZERO_STR = "00000000-0000-0000-0000-000000000000";

/* database define */
static const std::string OAID_DATA_BASE_DIR = "/data/service/el1/public/database/";
static const std::string OAID_DATA_BASE_APP_ID = "oaid_service_manager";
static const std::string OAID_DATA_BASE_STORE_ID = "oaidservice";
static const std::string OAID_KVSTORE_KEY = "oaid_key";
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_OAID_SERVICE_DEFINE_H