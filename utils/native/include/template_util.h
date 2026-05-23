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

#ifndef OHOS_CLOUD_TEMPLATE_UTIL_H
#define OHOS_CLOUD_TEMPLATE_UTIL_H
#include <vector>
#include <unordered_set>
namespace OHOS {
namespace Cloud {
namespace Template {
// "failed" make static_assert failed in the template deduction phase.
template <typename T>
struct failed : std::false_type {};
template <typename T>
inline constexpr bool failed_v = failed<T>::value; // Use type traits to postpone to second stage.
// -------------------------------- is vector --------------------------------
template <typename T>
struct is_vector_impl {
    static constexpr bool value = false;
};
template <typename T>
struct is_vector_impl<std::vector<T>> {
    static constexpr bool value = true;
};
template <typename T>
struct is_vector {
    static constexpr bool value = is_vector_impl<std::decay_t<T>>::value;
};
template <typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;
// -------------------------------- is vector --------------------------------

// -------------------------------- is unordered_set --------------------------------
template <typename T>
struct is_unordered_set_impl {
    static constexpr bool value = false;
};
template <typename T>
struct is_unordered_set_impl<std::unordered_set<T>> {
    static constexpr bool value = true;
};
template <typename T>
struct is_unordered_set {
    static constexpr bool value = is_unordered_set_impl<std::decay_t<T>>::value;
};
template <typename T>
inline constexpr bool is_unordered_set_v = is_unordered_set<T>::value;
// -------------------------------- is unordered_set --------------------------------
}  // namespace Template
}  // namespace Cloud
}  // namespace OHOS
#endif // OHOS_CLOUD_TEMPLATE_UTIL_H