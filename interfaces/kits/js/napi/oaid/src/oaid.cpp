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

#include "oaid.h"

#include <string>
#include <uv.h>
#include "hilog/log.h"
#include "oaid_common.h"
#include "oaid_service_client.h"
#include "securec.h"

namespace OHOS {
namespace CloudNapi {
namespace OAIDNapi {
namespace {
const int32_t NO_ERROR = 0;
const int32_t ERROR = -1;
const size_t OAID_MAX_PARA = 1;
const size_t CALLBACK_ARGS_LENGTH = 2;
const int8_t CALLBACK_CODE = 0;
const int8_t CALLBACK_RESULT = 1;
/* not system app error code */
static const int32_t OAID_ERROR_CODE_NOT_SYSTEM_APP = 202;
/* not in trust list error code */
static const int32_t OAID_ERROR_NOT_IN_TRUST_LIST = 17300002;
} // namespace

std::mutex oaidLock_;

struct AsyncCallbackInfoOAID {
    napi_env env = nullptr;
    napi_async_work asyncWork = nullptr;
    napi_ref callback = nullptr;
    napi_deferred deferred = nullptr;
    std::string oaid;
    bool isCallback = false;
    int32_t errorCode = NO_ERROR;
};

napi_value NapiGetNull(napi_env env)
{
    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}

napi_value GetCallbackErrorValue(napi_env env, int32_t errCode)
{
    napi_value result = nullptr;
    napi_value eCode = nullptr;
    NAPI_CALL(env, napi_create_int32(env, errCode, &eCode));
    NAPI_CALL(env, napi_create_object(env, &result));
    NAPI_CALL(env, napi_set_named_property(env, result, "code", eCode));
    return result;
}

void SetPromise(const napi_env& env, const napi_deferred& deferred, const int32_t& errorCode, const napi_value& result)
{
    if (errorCode == NO_ERROR) {
        NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, deferred, result));
        return;
    }
    NAPI_CALL_RETURN_VOID(env, napi_reject_deferred(env, deferred, result));
}

void SetCallback(const napi_env& env, const napi_ref& callbackIn, const int32_t& errorCode, const napi_value& result)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value callback = nullptr;
    napi_value resultout = nullptr;
    napi_get_reference_value(env, callbackIn, &callback);
    napi_value results[CALLBACK_ARGS_LENGTH] = { 0 };
    results[CALLBACK_CODE] = GetCallbackErrorValue(env, errorCode);
    results[CALLBACK_RESULT] = result;
    NAPI_CALL_RETURN_VOID(
        env, napi_call_function(env, undefined, callback, CALLBACK_ARGS_LENGTH, &results[CALLBACK_CODE], &resultout));
}

napi_value ParaError(const napi_env& env, const napi_ref& callback)
{
    if (callback != nullptr) {
        return NapiGetNull(env);
    }

    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    napi_create_promise(env, &deferred, &promise);
    SetPromise(env, deferred, ERROR, NapiGetNull(env));
    return promise;
}

void ReturnCallbackPromise(const napi_env& env, AsyncCallbackInfoOAID*& info, const napi_value& result)
{
    if (info->isCallback) {
        SetCallback(env, info->callback, info->errorCode, result);
    } else {
        SetPromise(env, info->deferred, info->errorCode, result);
    }
}

napi_value ParseParameters(const napi_env& env, const napi_value (&argv)[OAID_MAX_PARA],
    const size_t& argc, napi_ref& callback)
{
    OAID_HILOGI(OHOS::Cloud::OAID_MODULE_JS_NAPI, "Begin.");
    NAPI_ASSERT(env, argc >= OAID_MAX_PARA - 1, "Wrong number of arguments.");

    napi_valuetype valuetype = napi_undefined;
    if (argc >= OAID_MAX_PARA) {
        NAPI_CALL(env, napi_typeof(env, argv[0], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_function, "Wrong argument type, function expected.");
        NAPI_CALL(env, napi_create_reference(env, argv[0], 1, &callback));
    }

    OAID_HILOGI(OHOS::Cloud::OAID_MODULE_JS_NAPI, "End.");
    return NapiGetNull(env);
}

void PaddingCallbackInfo(
    const napi_env& env, AsyncCallbackInfoOAID*& asynccallbackinfo, const napi_ref& callback, napi_value& promise)
{
    if (callback != nullptr) {
        asynccallbackinfo->isCallback = true;
        asynccallbackinfo->callback = callback;
    } else {
        asynccallbackinfo->isCallback = false;
        napi_deferred deferred = nullptr;
        NAPI_CALL_RETURN_VOID(env, napi_create_promise(env, &deferred, &promise));
        asynccallbackinfo->deferred = deferred;
    }
}

void GetOAIDExecuteCallBack(napi_env env, void* data)
{
    std::lock_guard<std::mutex> autoLock(oaidLock_);
    AsyncCallbackInfoOAID* asynccallbackinfo = (AsyncCallbackInfoOAID*)data;

    asynccallbackinfo->oaid = Cloud::OAIDServiceClient::GetInstance()->GetOAID();
    if (asynccallbackinfo->oaid.empty()) {
        asynccallbackinfo->errorCode = ERROR;
        return;
    }
}

void GetOAIDCompleteCallBack(napi_env env, napi_status status, void* data)
{
    AsyncCallbackInfoOAID* asynccallbackinfo = (AsyncCallbackInfoOAID*)data;
    napi_value result = nullptr;
    NAPI_CALL_RETURN_VOID(
        env, napi_create_string_utf8(env, asynccallbackinfo->oaid.c_str(), NAPI_AUTO_LENGTH, &result));
    ReturnCallbackPromise(env, asynccallbackinfo, result);
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, asynccallbackinfo->asyncWork));
    delete asynccallbackinfo;
    asynccallbackinfo = nullptr;
}

AsyncCallbackInfoOAID* GetAsyncCallbackInfoOAID(
    napi_env& env, napi_callback_info& info, napi_ref& callback, napi_value& promise)
{
    size_t argc = OAID_MAX_PARA;
    napi_value argv[OAID_MAX_PARA] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));

    if (ParseParameters(env, argv, argc, callback) == nullptr) {
        return nullptr;
    }

    AsyncCallbackInfoOAID* asynccallbackinfo =
        new (std::nothrow) AsyncCallbackInfoOAID { .env = env, .asyncWork = nullptr };
    if (!asynccallbackinfo) {
        return nullptr;
    }

    PaddingCallbackInfo(env, asynccallbackinfo, callback, promise);
    return asynccallbackinfo;
}

napi_value GetOAID(napi_env env, napi_callback_info info)
{
    OAID_HILOGI(OHOS::Cloud::OAID_MODULE_JS_NAPI, "Begin.");
    napi_ref callback = nullptr;
    napi_value promise = nullptr;
    AsyncCallbackInfoOAID* asynccallbackinfo = GetAsyncCallbackInfoOAID(env, info, callback, promise);
    if (asynccallbackinfo == nullptr) {
        return ParaError(env, callback);
    }

    napi_value resourceName = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "getAdsIdentifierInfo", NAPI_AUTO_LENGTH, &resourceName));
    napi_async_execute_callback getOAIDExecuteCallBack = GetOAIDExecuteCallBack;
    napi_async_complete_callback getOAIDCompleteCallBack = GetOAIDCompleteCallBack;
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resourceName, getOAIDExecuteCallBack, getOAIDCompleteCallBack,
                       (void*)asynccallbackinfo, &asynccallbackinfo->asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, asynccallbackinfo->asyncWork));

    OAID_HILOGI(OHOS::Cloud::OAID_MODULE_JS_NAPI, "End.");

    if (asynccallbackinfo->isCallback) {
        return NapiGetNull(env);
    } else {
        return promise;
    }
}

napi_value ResetOAID(napi_env env, napi_callback_info info)
{
    OAID_HILOGI(OHOS::Cloud::OAID_MODULE_JS_NAPI, "ResetOAID Begin.");

    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    int32_t errorCode = Cloud::OAIDServiceClient::GetInstance()->ResetOAID();
    OAID_HILOGI(OHOS::Cloud::OAID_MODULE_JS_NAPI, "ResetOAID code = %{public}d", errorCode);
    if (errorCode == OAID_ERROR_CODE_NOT_SYSTEM_APP) {
        napi_throw_error(env, std::to_string(errorCode).c_str(), "not system app");
    }

    if (errorCode == OAID_ERROR_NOT_IN_TRUST_LIST) {
        napi_throw_error(env, std::to_string(errorCode).c_str(), "not in trust list");
    }

    OAID_HILOGI(OHOS::Cloud::OAID_MODULE_JS_NAPI, "ResetOAID End.");

    return result;
}

napi_value OAIDInit(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("getOAID", GetOAID),
        DECLARE_NAPI_FUNCTION("resetOAID", ResetOAID),
    };

    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}
} // namespace OAIDNapi
} // namespace CloudNapi
} // namespace OHOS