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
#ifndef HUTZDOG_CS2_ASSIGN_WEEK6_PROBLEMe1
#define HUTZDOG_CS2_ASSIGN_WEEK6_PROBLEMe1

#include <concepts>
namespace hutzdog_cs2_week6 {
///
/// Compute the factorial of a number
/// \param n The number whose factorial should be computed
/// \return The factorial of the number
///
template <std::unsigned_integral T> constexpr auto factorial(T n) -> T {
    if (n == 0) {
        return 1;
    }
    return n * factorial(n - 1);
}
} // namespace hutzdog_cs2_week6

#endif // HUTZDOG_CS2_ASSIGN_WEEK6_PROBLEMe1
