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
 */

#ifndef HUTZDOG_CS2_LIB_UTIL
#define HUTZDOG_CS2_LIB_UTIL

#include <concepts>

namespace cs2_lib::util {

///
/// Data type for types that can be used in a poweer relationship
///
// TODO: This name is ridiculous. Change it
template <class T, class U>
concept Exponentable = requires(T base, U exponent) {
    { base *= base };
    { base++ };
    { T() };

    std::unsigned_integral<U>;
};

///
/// Raise a base to an exponent. Why only a float version from C exists in the
/// standard escapes me.
///
template <class T, class U>
[[nodiscard]] constexpr auto pow(T base, U exponent) -> T
    requires Exponentable<T, U>
{
    T total{};
    total++;

    for (U current_pow{}; current_pow < exponent; current_pow++) {
        total *= base;
    }

    return total;
}

} // namespace cs2_lib::util

#endif // HUTZDOG_CS2_LIB_UTIL
