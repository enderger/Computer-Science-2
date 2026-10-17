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
#include <cstddef>
#include <format>

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
/// A concept for the allocators used in this library
///
template <class Alloc>
concept Allocator = requires(Alloc alloc, size_t n) {
    { *alloc.allocate(n) } -> std::same_as<typename Alloc::value_type &>;
    { alloc.deallocate(alloc.allocate(n), n) };
} && std::copy_constructible<Alloc> && std::equality_comparable<Alloc>;

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

template <class T>
concept Printable =
    std::formattable<T, char> || requires(std::ostream &ost, const T &val) {
        { ost << val } -> std::convertible_to<std::ostream &>;
    };

} // namespace cs2_lib::util

#endif // HUTZDOG_CS2_LIB_UTIL
