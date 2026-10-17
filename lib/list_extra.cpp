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

// BEGIN AI CODE
// Written by Claude (AI) on 2026-10-17 at Danielle's request: the non-core
// List tests (items 7-12 of the 2026-10-09 review). The core semantic tests
// live in list.cpp.
//
// [[maybe_unused]] marks variables that are only read inside CHECKs, which
// compile away under DOCTEST_CONFIG_DISABLE (clang -Werror).

#include "cs2lib/list.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <memory>
#include <ostream>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// NOLINTBEGIN(readability-magic-numbers)

namespace {
using cs2_lib::list::List;

/// Long enough that a recursive teardown overflows the stack in every build
/// type (a recursive destructor crashed at 1M nodes even at -O2).
constexpr std::size_t DEEP_SIZE = 2'000'000;

///
/// Element that counts live instances and can be told to throw on a copy.
///
struct Tracked {
    static inline int live = 0;
    /// Copies left before one throws; -1 means never throw.
    static inline int copies_until_throw = -1;

    int value;

    constexpr explicit Tracked(int value_) : value{value_} { ++live; }
    Tracked(const Tracked &other) : value{other.value} {
        if (copies_until_throw == 0) {
            throw std::runtime_error("Tracked: injected copy failure");
        }
        if (copies_until_throw > 0) {
            --copies_until_throw;
        }
        ++live;
    }
    Tracked(Tracked &&other) noexcept : value{other.value} { ++live; }
    auto operator=(const Tracked &) -> Tracked & = delete;
    auto operator=(Tracked &&) -> Tracked & = delete;
    ~Tracked() { --live; }

    auto operator==(const Tracked &other) const -> bool {
        return this->value == other.value;
    }

    [[maybe_unused]] friend auto operator<<(std::ostream &stream,
                                            const Tracked &tracked)
        -> std::ostream & {
        return stream << "(Tracked " << tracked.value << ")";
    }
};

///
/// Arms Tracked's copy failure for one scope, and always disarms it on exit,
/// so a failing CHECK can't leak the setting into later tests.
///
class ThrowAfterCopies {
  public:
    explicit ThrowAfterCopies(int copies) {
        Tracked::copies_until_throw = copies;
    }
    ThrowAfterCopies(const ThrowAfterCopies &) = delete;
    ThrowAfterCopies(ThrowAfterCopies &&) = delete;
    auto operator=(const ThrowAfterCopies &) -> ThrowAfterCopies & = delete;
    auto operator=(ThrowAfterCopies &&) -> ThrowAfterCopies & = delete;
    ~ThrowAfterCopies() { Tracked::copies_until_throw = -1; }
};

///
/// Printable only through operator<<, so it exercises the formatter's
/// ostream fallback.
///
struct Point {
    int x;
    int y;

    [[maybe_unused]] friend auto operator<<(std::ostream &stream,
                                            const Point &point)
        -> std::ostream & {
        return stream << "(Point " << point.x << " " << point.y << ")";
    }
};
static_assert(!std::formattable<Point, char>);
static_assert(cs2_lib::util::Printable<Point>);

} // namespace

///
/// A namespace with its own operator<<. Printing a List from in here only
/// compiles if List's operator<< is found through the List type (ADL).
///
namespace list_print_test {
struct Tag {};
[[maybe_unused]] auto operator<<(std::ostream &stream, Tag) -> std::ostream & {
    return stream << "tag";
}
auto print(const cs2_lib::list::List<int> &list) -> std::string {
    std::ostringstream stream;
    stream << list;
    return stream.str();
}
} // namespace list_print_test

///
/// Compile-time tests: each function is checked by static_assert (so the
/// constant evaluator rejects any undefined behaviour as a compile error) and
/// again at runtime under the sanitizers.
///
namespace list_constexpr_test {
using cs2_lib::list::List;

constexpr auto construct_and_compare() -> bool {
    const List<int> listed{1, 2, 3};
    const List<int> filled(3, 7);
    const List<int> ranged(std::from_range, std::array{1, 2, 3});
    return listed == ranged && filled == List<int>{7, 7, 7} &&
           listed != filled && List<int>{} == List<int>{};
}

constexpr auto push_emplace_pop() -> bool {
    List<int> list;
    list.push_front(2);
    list.emplace_front(1);
    const auto first = list.pop_front();
    const auto second = list.pop_front();
    const auto none = list.pop_front();
    return first == 1 && second == 2 && !none.has_value() && list.empty();
}

constexpr auto reverse_list() -> bool {
    List<int> list{1, 2, 3, 4};
    list.reverse();
    List<int> empty_list;
    empty_list.reverse();
    return list == List<int>{4, 3, 2, 1} && empty_list.empty();
}

constexpr auto copy_is_independent() -> bool {
    const List<int> original{1, 2, 3};
    List<int> copy(original);
    copy[0] = 99;
    List<int> assigned;
    assigned = original;
    return original == List<int>{1, 2, 3} && copy == List<int>{99, 2, 3} &&
           assigned == original;
}

constexpr auto move_semantics() -> bool {
    List<int> source{1, 2, 3};
    List<int> moved(std::move(source));
    List<int> target{9};
    target = std::move(moved);
    // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    return source.empty() && target == List<int>{1, 2, 3};
}

constexpr auto views_and_access() -> bool {
    List<int> list{10, 20, 30, 40};
    list[2] = 31;
    const auto head = list.car();
    return head.has_value() && head->get() == 10 && list[2] == 31 &&
           std::ranges::distance(list.cdr()) == 3 &&
           std::ranges::distance(list.cndr(3)) == 1 &&
           std::ranges::empty(list.cndr(10)) && list.length() == 4;
}

constexpr auto ranges_to() -> bool {
    const auto list = std::array{1, 2, 3} | std::ranges::to<List>();
    return list == List<int>{1, 2, 3};
}

static_assert(construct_and_compare());
static_assert(push_emplace_pop());
static_assert(reverse_list());
static_assert(copy_is_independent());
static_assert(move_semantics());
static_assert(views_and_access());
static_assert(ranges_to());
} // namespace list_constexpr_test

TEST_SUITE("list (non-core)") {

    TEST_CASE("exception safety: a throwing element copy leaks nothing") {
        [[maybe_unused]] const int baseline = Tracked::live;

        SUBCASE("copy constructor, failing on the last of a deep copy") {
            {
                const List<Tracked> source(DEEP_SIZE, Tracked{1});
                const ThrowAfterCopies guard(static_cast<int>(DEEP_SIZE) - 1);
                CHECK_THROWS_AS(
                    [&source] {
                        [[maybe_unused]] List<Tracked> copy(source);
                    }(),
                    std::runtime_error);
                CHECK(Tracked::live == baseline + static_cast<int>(DEEP_SIZE));
            }
            CHECK(Tracked::live == baseline);
        }

        SUBCASE("initializer_list constructor") {
            const ThrowAfterCopies guard(2);
            // Named outside the macro: the braced list's commas would otherwise
            // split the macro arguments.
            [[maybe_unused]] const auto build = [] {
                [[maybe_unused]] const List<Tracked> list{
                    Tracked{1}, Tracked{2}, Tracked{3}, Tracked{4}};
            };
            CHECK_THROWS_AS(build(), std::runtime_error);
            CHECK(Tracked::live == baseline);
        }

        SUBCASE(
            "(size, value) constructor, failing halfway through a deep list") {
            const ThrowAfterCopies guard(static_cast<int>(DEEP_SIZE / 2));
            CHECK_THROWS_AS(
                [] {
                    [[maybe_unused]] List<Tracked> list(DEEP_SIZE, Tracked{1});
                }(),
                std::runtime_error);
            CHECK(Tracked::live == baseline);
        }

        SUBCASE("from_range constructor") {
            {
                std::vector<Tracked> source;
                source.reserve(4);
                for (int idx = 0; idx < 4; ++idx) {
                    source.emplace_back(idx);
                }
                const ThrowAfterCopies guard(2);
                CHECK_THROWS_AS(
                    [&source] {
                        [[maybe_unused]] List<Tracked> list(std::from_range,
                                                            source);
                    }(),
                    std::runtime_error);
                CHECK(Tracked::live == baseline + 4);
            }
            CHECK(Tracked::live == baseline);
        }

        SUBCASE("emplace_front leaves the list unchanged (strong guarantee)") {
            {
                List<Tracked> list{Tracked{1}, Tracked{2}};
                [[maybe_unused]] const List<Tracked> expected{Tracked{1},
                                                              Tracked{2}};
                const Tracked item{9};
                {
                    const ThrowAfterCopies guard(0);
                    CHECK_THROWS_AS(list.emplace_front(item),
                                    std::runtime_error);
                }
                CHECK(list == expected);
            }
            CHECK(Tracked::live == baseline);
        }

        SUBCASE(
            "copy assignment leaves the target unchanged (strong guarantee)") {
            {
                List<Tracked> target{Tracked{1}, Tracked{2}};
                [[maybe_unused]] const List<Tracked> expected{Tracked{1},
                                                              Tracked{2}};
                const List<Tracked> source{Tracked{7}, Tracked{8}, Tracked{9}};
                {
                    const ThrowAfterCopies guard(1);
                    CHECK_THROWS_AS(target = source, std::runtime_error);
                }
                CHECK(target == expected);
            }
            CHECK(Tracked::live == baseline);
        }

        SUBCASE("moves, reverse and pop_front never copy an element") {
            {
                List<Tracked> list{Tracked{1}, Tracked{2}, Tracked{3}};
                const ThrowAfterCopies guard(0); // any copy now throws
                CHECK_NOTHROW(list.reverse());
                CHECK_NOTHROW([&list] {
                    List<Tracked> moved(std::move(list));
                    list = std::move(moved);
                }());
                CHECK_NOTHROW([&list] {
                    [[maybe_unused]] const auto popped = list.pop_front();
                }());
            }
            CHECK(Tracked::live == baseline);
        }
    }

    TEST_CASE("move-only elements") {
        using Ptr = std::unique_ptr<int>;
        static_assert(!std::copy_constructible<List<Ptr>>);
        static_assert(std::movable<List<Ptr>>);

        List<Ptr> list;
        list.push_front(std::make_unique<int>(2));
        list.emplace_front(std::make_unique<int>(1));

        SUBCASE("iterate, read and write through iterators") {
            [[maybe_unused]] int sum = 0;
            for (const auto &ptr : list) {
                sum += *ptr;
            }
            CHECK(sum == 3);

            // Through a const list the iterator must still yield a reference;
            // a by-value operator* would try to copy the unique_ptr.
            const List<Ptr> &const_list = list;
            [[maybe_unused]] const Ptr &first = *const_list.begin();
            CHECK(&first == &list.car()->get());

            *list.begin() = std::make_unique<int>(10);
            CHECK(*list.car()->get() == 10);
        }

        SUBCASE("pop_front moves the element out") {
            auto popped = list.pop_front();
            REQUIRE(popped.has_value());
            CHECK(**popped == 1);
            CHECK(list.length() == 1);
        }

        SUBCASE("reverse and moves") {
            list.reverse();
            CHECK(*list.car()->get() == 2);

            List<Ptr> moved(std::move(list));
            // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
            CHECK(list.empty());
            CHECK(moved.length() == 2);
        }

        SUBCASE("from_range moves elements in from an rvalue view") {
            std::vector<Ptr> source;
            source.push_back(std::make_unique<int>(5));
            source.push_back(std::make_unique<int>(6));

            const List<Ptr> moved_in(std::from_range,
                                     source | std::views::as_rvalue);
            CHECK(*moved_in.car()->get() == 5);
            CHECK(moved_in.length() == 2);
            CHECK(source[0] == nullptr); // ownership left the vector
        }
    }

    TEST_CASE("formatting and printing") {
        CHECK(std::format("{}", List{0, 1, 2}) == "(List 0 1 2)");
        CHECK(std::format("{}", List<int>{}) == "(List)");
        CHECK(std::format("{}", List<List<int>>{{1, 2}, {3}}) ==
              "(List (List 1 2) (List 3))");
        CHECK(std::format("{}", List<std::string>{"a", "bc"}) == "(List a bc)");

        SUBCASE("ostream-only elements use the stream fallback") {
            CHECK(std::format("{}", List<Point>{{1, 2}, {3, 4}}) ==
                  "(List (Point 1 2) (Point 3 4))");
        }

        SUBCASE("format specs pass through to the string formatter") {
            CHECK(std::format("{:>12}", List{1, 2}) == "  (List 1 2)");
            CHECK(std::format("{:*<12}", List{1, 2}) == "(List 1 2)**");
        }

        SUBCASE("operator<<") {
            std::ostringstream stream;
            stream << List{1, 2, 3};
            CHECK(stream.str() == "(List 1 2 3)");
        }

        SUBCASE("operator<< is found from inside another namespace") {
            CHECK(list_print_test::print(List{4, 5}) == "(List 4 5)");
        }
    }

    TEST_CASE("ranges interoperability") {
        static_assert(std::ranges::forward_range<List<int>>);
        static_assert(
            !std::ranges::bidirectional_range<List<int>>);   // singly linked
        static_assert(!std::ranges::sized_range<List<int>>); // length() is O(n)
        static_assert(std::ranges::output_range<List<int> &, int>);
        static_assert(!std::ranges::output_range<const List<int> &, int>);
        static_assert(
            std::ranges::view<decltype(std::declval<List<int> &>().cdr())>);
        static_assert(
            std::same_as<std::ranges::range_reference_t<List<int>>, int &>);
        static_assert(
            std::same_as<std::ranges::range_reference_t<const List<int>>,
                         const int &>);

        SUBCASE("construction from other ranges") {
            CHECK(List<int>(std::from_range, std::vector{1, 2, 3}) ==
                  List{1, 2, 3});

            // Not sized: take_while over an unbounded iota.
            CHECK((std::views::iota(0) |
                   std::views::take_while([](int value) { return value < 4; }) |
                   std::ranges::to<List<int>>()) == List{0, 1, 2, 3});

            // Single-pass: an input-only range.
            std::istringstream input{"4 5 6"};
            CHECK(List<int>(std::from_range, std::views::istream<int>(input)) ==
                  List{4, 5, 6});
        }

        SUBCASE("ranges::to deduces the element type") {
            auto deduced =
                std::vector{1, 2} | std::ranges::to<cs2_lib::list::List>();
            static_assert(std::same_as<decltype(deduced), List<int>>);
            CHECK(deduced == List{1, 2});
        }

        SUBCASE("composes with views and algorithms") {
            List list{1, 2, 3, 4};
            CHECK((list |
                   std::views::transform([](int value) { return value * 10; }) |
                   std::ranges::to<List>()) == List{10, 20, 30, 40});
            CHECK(std::ranges::equal(list, std::array{1, 2, 3, 4}));
            CHECK(std::ranges::count_if(
                      list, [](int value) { return value % 2 == 0; }) == 2);
            CHECK(std::ranges::find(list, 3) != list.end());
            CHECK(std::ranges::find(list, 5) == list.end());
            CHECK(std::ranges::equal(list.cdr(), std::array{2, 3, 4}));
            CHECK(std::ranges::equal(list.cndr(2), std::array{3, 4}));

            std::ranges::fill(list, 7);
            CHECK(list == List{7, 7, 7, 7});

            [[maybe_unused]] const List<int> &view_of = list;
            CHECK(std::ranges::fold_left(view_of, 0, std::plus<>()) == 28);
        }
    }

    TEST_CASE("compile-time tests also pass at runtime") {
        CHECK(list_constexpr_test::construct_and_compare());
        CHECK(list_constexpr_test::push_emplace_pop());
        CHECK(list_constexpr_test::reverse_list());
        CHECK(list_constexpr_test::copy_is_independent());
        CHECK(list_constexpr_test::move_semantics());
        CHECK(list_constexpr_test::views_and_access());
        CHECK(list_constexpr_test::ranges_to());
    }

    TEST_CASE("length() and empty() agree after every operation") {
        List<int> list;
        CHECK(list.empty());
        CHECK(list.length() == 0U);

        list.push_front(1);
        list.emplace_front(0);
        CHECK_FALSE(list.empty());
        CHECK(list.length() == 2U);

        list.reverse();
        CHECK(list.length() == 2U);

        [[maybe_unused]] const List<int> copy(list);
        CHECK(copy.length() == 2U);

        List<int> moved(std::move(list));
        // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
        CHECK(list.empty());
        CHECK(list.length() == 0U);
        CHECK(moved.length() == 2U);

        CHECK(moved.pop_front().has_value());
        CHECK(moved.pop_front().has_value());
        CHECK_FALSE(moved.pop_front().has_value()); // popping an empty list
        CHECK(moved.empty());
        CHECK(moved.length() == 0U);

        [[maybe_unused]] const List<int> ranged(std::from_range,
                                                std::array{1, 2, 3});
        CHECK(ranged.length() == 3U);
        CHECK_FALSE(ranged.empty());
    }

} // TEST_SUITE

// NOLINTEND(readability-magic-numbers)
// END AI CODE
