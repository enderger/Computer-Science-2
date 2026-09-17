/*
 * Copyright 2026 Danielle Hutzley
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Generated from template
 */

#ifndef HUTZDOG_CS2_LIB_HASH
#define HUTZDOG_CS2_LIB_HASH

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace cs2_lib {
constexpr auto knr_hash(const std::string_view data) -> std::uint64_t {
    return std::ranges::fold_left(
        data, 0, [](std::uint64_t acc, char character) -> uint64_t {
            return character + (acc << 5);
        });
}

namespace literals {
constexpr auto operator""_knr_hash(const char *data, uintptr_t len)
    -> uint64_t {
    return cs2_lib::knr_hash(std::string_view(data, len));
}
} // namespace literals

} // namespace cs2_lib

#endif // HUTZDOG_CS2_LIB_HASH
