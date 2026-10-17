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
#include <optional>
#include <ostream>
#include <ranges>
#include <sstream>

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

template <std::ranges::forward_range R>
    requires Printable<std::ranges::range_value_t<R>>
class RangeFormatter {
  public:
    RangeFormatter(R &range)
        : range_begin{range.begin()}, range_end{range.end()} {}
    RangeFormatter(R &&range)
        : data{range}, range_begin{data.value().begin()},
          range_end{data.value().end()} {}

    friend constexpr auto operator<<(std::ostream &out,
                                     const RangeFormatter &fmt)
        -> std::ostream & {
        out << "(Range";
        std::ranges::for_each(
            fmt.range_begin, fmt.range_end,
            [&](auto &val) -> void { out << ' ' << fmt.format_elem(val); });
        out << ")";

        return out;
    }

  private:
    // This purely exists to keep the data alive if necessary
    std::optional<R> data{std::nullopt};
    std::ranges::iterator_t<R> range_begin;
    std::ranges::sentinel_t<R> range_end;

    [[nodiscard]] constexpr auto
    format_elem(const std::ranges::range_value_t<R> &val) const -> std::string {
        if constexpr (std::formattable<std::ranges::range_value_t<R>, char>) {
            return std::format("{}", val);
        } else {
            std::ostringstream sstream{};
            sstream << val;
            return sstream.str();
        }
    }

    friend class std::formatter<RangeFormatter<R>>;
};

} // namespace cs2_lib::util

template <std::ranges::forward_range R>
    requires cs2_lib::util::Printable<std::ranges::range_value_t<R>>
struct std::formatter<cs2_lib::util::RangeFormatter<R>>
    : std::formatter<std::string> {
    template <class FormatCtx>
    auto format(const cs2_lib::util::RangeFormatter<R> &fmt, FormatCtx ctx) {
        std::string result{"(Range"};
        std::ranges::for_each(fmt.range, [&](auto &val) {
            std::format_to(std::back_inserter(result), " {}",
                           fmt.format_elem(val));
        });
        result += ')';
        return std::formatter<std::string>::format(result, ctx);
    }
};

#endif // HUTZDOG_CS2_LIB_UTIL
