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
#include <algorithm>
#include <format>
#include <numeric>
#include <string>

#ifndef HUTZDOG_CS2_ASSIGN_WEEK7_PROBLEME2
#define HUTZDOG_CS2_ASSIGN_WEEK7_PROBLEME2

namespace hutzdog_cs2_week7 {
///
/// Get a string documenting the array's current status
///
// NOTE: I made this return a string rather than print one for testing purposes
//       Also, we accept a C array because that's what's requested
template <class T, size_t SIZE>
constexpr auto array_stats(T value[SIZE]) -> std::string // NOLINT
    requires std::is_arithmetic_v<T> && std::formattable<T, char>
{
    auto [min, max] = std::minmax_element(value, value + SIZE);
    T sum = std::accumulate<T *>(value, value + SIZE, static_cast<T>(0));

    return std::format("(Array (min . {}) (max . {}) (sum . {}))", *min, *max,
                       sum);
}
} // namespace hutzdog_cs2_week7

#endif // HUTZDOG_CS2_ASSIGN_WEEK7_PROBLEME2
