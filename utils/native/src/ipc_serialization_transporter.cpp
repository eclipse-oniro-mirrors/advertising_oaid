/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ipc_serialization_transporter.h"
namespace OHOS {
namespace Cloud {
IpcSerializationTransporter::IpcSerializationTransporter()
{}
std::string IpcSerializationTransporter::GetflatData() const
{
    return ss_.str();
}
IpcSerializationTransporter::Reader::Reader(const uint8_t* data, uint32_t size)
    : data_(data), size_(size), cursor_(const_cast<uint8_t*>(data))
{}
std::optional<std::string> IpcSerializationTransporter::Reader::ReadString()
{
    auto lenOpt = Read<size_t>();
    if (!lenOpt.has_value()) {
        isBad_ = true;
        OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: read string len failed");
        return std::nullopt;
    }
    if (!CheckRemainingSize(lenOpt.value())) {
        OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: remaining size is smaller than str size. cannot get str");
        return std::nullopt;
    }
    std::string result(reinterpret_cast<const char*>(cursor_), lenOpt.value());
    cursor_ += lenOpt.value();
    return std::move(result);
}

std::unique_ptr<IpcSerializationTransporter::Reader> IpcSerializationTransporter::Reader::Build(const uint8_t* data,
    uint32_t size)
{
    if (!data || size == 0) {
        return nullptr;
    }
    return std::make_unique<Reader>(data, size);
}
}  // namespace Cloud
}  // namespace OHOS