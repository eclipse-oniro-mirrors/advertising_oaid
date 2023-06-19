/*
 * Copyright (c); 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");;
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

#ifndef OHOS_CLOUD_NAPI_OAID_H
#define OHOS_CLOUD_NAPI_OAID_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace CloudNapi {
namespace OAIDNapi {
/**
 * Register OAID function to NAPI.
 *
 * @param env napi environment variable.
 * @param exports napi export variable.
 * @return napi_value, napi export variable.
 */
napi_value OAIDInit(napi_env env, napi_value exports);

/**
 * Get open advertising id.
 *
 * @param env napi environment variable.
 * @param exports napi export variable.
 * @return napi_value, result.
 */
napi_value GetOAID(napi_env env, napi_callback_info info);

/**
 * Reset open advertising id.
 *
 * @param env napi environment variable.
 * @param exports napi export variable.
 * @return napi_value, result.
 */
napi_value ResetOAID(napi_env env, napi_callback_info info);
} // namespace OAIDNapi
} // namespace CloudNapi
} // namespace OHOS
#endif // OHOS_CLOUD_NAPI_OAID_H