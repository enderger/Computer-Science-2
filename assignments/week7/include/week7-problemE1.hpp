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
#include <format>
#include <print>
#include <string>
#include <string_view>

#ifndef HUTZDOG_CS2_ASSIGN_WEEK7_PROBLEME1
#define HUTZDOG_CS2_ASSIGN_WEEK7_PROBLEME1

namespace hutzdog_cs2_week7 {
///
/// Swap two references, this is a thin wrapper around `std::swap`
/// \param first The first value to swap
/// \param second The second value to swap
///
template <class T>
[[gnu::always_inline]] constexpr void swap_values(T &first, T &second) {
    return std::swap(first, second);
}

///
/// Print two formattable values
/// \param fst The first value to print
/// \param snd The second value to print
///
template <class T>
constexpr void print_values(T fst, T snd)
    requires std::formattable<T, char>
{
    std::println("{}, {}", fst, snd);
}
} // namespace hutzdog_cs2_week7

#endif // HUTZDOG_CS2_ASSIGN_WEEK7_PROBLEME1
