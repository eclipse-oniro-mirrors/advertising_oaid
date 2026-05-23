/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_CLOUD_IPC_SERIALIZATION_TRANSPORTER_H
#define OHOS_CLOUD_IPC_SERIALIZATION_TRANSPORTER_H
#include <optional>
#include <utility>
#include <cstdint>
#include <sstream>
#include <memory>
#include "securec.h"
#include "nocopyable.h"
#include "template_util.h"
#include "oaid_hilog_wreapper.h"
namespace OHOS {
namespace Cloud {
class IpcSerializationTransporter {
public:
    IpcSerializationTransporter();
    DISALLOW_COPY_AND_MOVE(IpcSerializationTransporter);
    template <typename T>
    [[nodiscard]] std::optional<std::string> Serialize(const T& rawData);
    std::string GetflatData() const;
    template <typename T>
    bool Flat(const T& rawData);
    class Reader {
    public:
        [[nodiscard]] static std::unique_ptr<Reader> Build(const uint8_t* data, uint32_t size);
        DISALLOW_COPY_AND_MOVE(Reader);
        template <typename T>
        [[nodiscard]] std::optional<T> Read();
        template <typename T, bool IS_ORDERED>
        [[nodiscard]] std::optional<T> ReadCommonContainer();
    private:
        template <typename T>
        [[nodiscard]] std::optional<T> ReadArithmetic();
        [[nodiscard]] std::optional<std::string> ReadString();
        [[nodiscard]] bool CheckRemainingSize(uint32_t expected) const
        {
            return size_ - (cursor_ - data_) >= expected;
        }
        Reader(const uint8_t* data, uint32_t size);
        const uint8_t* const data_;
        const uint32_t size_;
        const uint8_t* cursor_;
        bool isBad_ = false;
    };
    using LenSizeT = uint32_t;
    static constexpr uint32_t LEN_SIZE = sizeof(uint32_t);
private:
    std::stringstream ss_;
};

template <typename T>
std::optional<T> IpcSerializationTransporter::Reader::Read()
{
    using namespace Template;
    if (isBad_) {
        OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: reader is bad");
        return std::nullopt;
    }
    if constexpr (std::is_arithmetic_v<T>) {
        return ReadArithmetic<T>();
    } else if constexpr (std::is_same_v<T, std::string>) {
        return ReadString();
    } else if constexpr (std::is_enum_v<T>) {
        auto int32Opt = Read<int32_t>();
        if (!int32Opt.has_value()) {
            OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: read int32_t to convert to enum failed");
            return std::nullopt;
        }
        return static_cast<T>(int32Opt.value());
    } else if constexpr (std::disjunction_v<is_vector<T>, is_unordered_set<T>>) {
        std::optional<T> tOpt;
        if constexpr (is_vector_v<T>) {
            tOpt = ReadCommonContainer<T, false>();
        } else {
            tOpt = ReadCommonContainer<T, true>();
        }
        if (!tOpt.has_value()) {
            OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: read container T failed");
        }
        return tOpt;
    } else {
        static_assert(Template::failed_v<T>, "please implement Reader::Read for T");
    }
}

template <typename T>
std::optional<T> IpcSerializationTransporter::Reader::ReadArithmetic()
{
    if (!CheckRemainingSize(LEN_SIZE)) {
        isBad_ = true;
        OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: remaining size is smaller than len size. cannot get len");
        return std::nullopt;
    }
    LenSizeT len;
    if (memcpy_s(&len, LEN_SIZE, cursor_, LEN_SIZE) != EOK) {
        OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: memcpy failed");
        return std::nullopt;
    }
    cursor_ += LEN_SIZE;
    if (len != sizeof(T)) {
        isBad_ = true;
        OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: len(%{public}u) is not equal to sizeof T(%{public}lu)",
            len, sizeof(T));
        return std::nullopt;
    }
    T result;
    if (memcpy_s(&result, sizeof(T), cursor_, len) != EOK) {
        OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: memcpy failed");
        isBad_ = true;
        return std::nullopt;
    }
    cursor_ += len;
    return std::make_optional<T>(result);
}

template <typename T, bool IS_ORDERED>
std::optional<T> IpcSerializationTransporter::Reader::ReadCommonContainer()
{
    auto int64Opt = Read<size_t>();
    if (!int64Opt.has_value()) {
        OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: read container size failed");
        return std::nullopt;
    }
    T t;
    for (size_t i = 0; i < int64Opt.value(); ++i) {
        auto valueOpt = Read<typename T::value_type>();
        if (!valueOpt.has_value()) {
            OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: read value for container T failed. index: %{public}zu", i);
            return std::nullopt;
        }
        if constexpr (IS_ORDERED) {
            t.emplace(std::move(valueOpt.value()));
        } else {
            t.emplace_back(std::move(valueOpt.value()));
        }
    }
    return {std::move(t)};
}

template <typename T>
std::optional<std::string> IpcSerializationTransporter::Serialize(const T& rawData)
{
    return Flat(rawData) ? std::make_optional(ss_.str()) : std::nullopt;
}

template <typename T>
bool IpcSerializationTransporter::Flat(const T& rawData)
{
    using namespace Template;
    static_assert(std::negation_v<std::is_pointer<T>>, "T cannot be a pointer");
    if constexpr (std::is_arithmetic_v<T>) {
        const LenSizeT len = sizeof(T);
        ss_.write(reinterpret_cast<const char*>(&len), LEN_SIZE);
        if (!ss_.fail()) {
            ss_.write(reinterpret_cast<const char*>(&rawData), len);
        }
        if (ss_.fail()) {
            OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: flat arithmetic failed. bad stream");
            return false;
        }
    } else if constexpr (std::is_enum_v<T>) {
        if (!Flat(static_cast<int32_t>(rawData))) {
            OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: flat rawData failed");
            return false;
        }
    } else if constexpr (std::is_same_v<T, std::string>) {
        if (!Flat(rawData.size())) {
            OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: flat string size failed");
            return false;
        }
        ss_.write(reinterpret_cast<const char*>(rawData.c_str()), rawData.size());
        if (ss_.fail()) {
            OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: flat string size failed. bad stream");
            return false;
        }
    } else if constexpr (std::disjunction_v<is_vector<T>, is_unordered_set<T>>) {
        if (!Flat(rawData.size())) {
            OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: flat container size failed");
            return false;
        }
        for (const typename T::value_type& elem : rawData) {
            if (!Flat(elem)) {
            OAID_HILOGE(OAID_MODULE_COMMON, "ipc_serialize: flat elem failed");
                return false;
            }
        }
    } else {
        static_assert(failed_v<T>, "please implement flat for T");
    }
    return true;
}
}  // namespace Cloud
}  // namespace OHOS
#endif // OHOS_CLOUD_IPC_SERIALIZATION_TRANSPORTER_H
