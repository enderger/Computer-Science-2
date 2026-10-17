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
// NOTE: Tests have [[maybe_unused]] variables to get around limitations in the
//       system that disables test evaluation in Doctest.
// NOTE: The convention for empty values is "std::optionals if an empty single
//       object is valid, an empty range for spans of the same, and exceptions
//       when an empty value is invalid."

#include "cs2lib/list.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <ranges>

[[maybe_unused]] constexpr const std::size_t RECURSIVE_TEST_SIZE = 4'000'000U;

namespace {
struct Element {
    int x, y;                                       // NOLINT
    constexpr Element(int x, int y) : x{x}, y{y} {} // NOLINT
    constexpr auto operator==(const Element &other) const -> bool = default;

    [[maybe_unused]] constexpr friend auto operator<<(std::ostream &stream,
                                                      const Element &elt)
        -> std::ostream & {
        stream << "(Element " << elt.x << " " << elt.y << ")";
        return stream;
    }
};
} // namespace

TEST_SUITE("test the list") {
    using cs2_lib::list::List;

    TEST_CASE("list length") {
        // NOLINTNEXTLINE(readability-magic-numbers)
        List five_element_list(5, 0);
        REQUIRE(5U == five_element_list.length());
    }

    TEST_CASE("list default constructors") {
        List<int> default_constructed{};
        List<int> empty_initializer_list = std::initializer_list<int>();
        List<int> empty_list(0, 1);
        CHECK(default_constructed == empty_initializer_list);
        CHECK(default_constructed == empty_list);

        List initializer_list_constructed{0, 1, 2, 3, 4};
        List<int> front_pushed{};
        // NOLINTNEXTLINE(readability-magic-numbers)
        for (int idx : std::views::iota(0, 5) | std::views::reverse) {
            front_pushed.push_front(idx);
        }
        CHECK(initializer_list_constructed == front_pushed);

        List copied_list{0, 1, 2, 3, 4};
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        List copy_list{copied_list};
        CHECK(copy_list == copied_list);

        List moved_list{0, 1, 2, 3, 4};
        List move_target_list{std::move(moved_list)};
        CHECK(List{0, 1, 2, 3, 4} == move_target_list);

        [[maybe_unused]] std::array range_array{0, 1, 2, 3, 4};
        CHECK(List{0, 1, 2, 3, 4} == (range_array | std::ranges::to<List>()));
    }

    TEST_CASE("list destructor") {
        // CHECK_OVERFLOW(
        cs2_lib::list::List big_list(RECURSIVE_TEST_SIZE, 0);
        CHECK(RECURSIVE_TEST_SIZE == big_list.length());
        // )
    }

    TEST_CASE("list push/emplace/pop") {
        List<Element> list{};
        CHECK(std::nullopt == list.pop_front());
        list.push_front({1, 1});
        list.emplace_front(0, 0);
        list.emplace_front(-1, -1);
        CHECK(List<Element>{{-1, -1}, {0, 0}, {1, 1}} == list);

        [[maybe_unused]] Element elt = list.pop_front().value();
        CHECK(List<Element>{{0, 0}, {1, 1}} == list);
        CHECK(Element{-1, -1} == elt);
    }

    TEST_CASE("list reverse") {
        List list{0, 1, 2, 3, 4};
        list.reverse();
        CHECK(List{4, 3, 2, 1, 0} == list);
    }

    TEST_CASE("list iterator") {
        List iterable{0, 1, 2, 3, 4};
        const List const_iterable{0, 1, 2, 3, 4};

        SUBCASE("initialized iterators") {
            using cs2_lib::list::Iterator;
            REQUIRE(Iterator<int>{} == Iterator<int>{});
        }

        SUBCASE("length") {
            [[maybe_unused]]
            int sum = std::ranges::fold_left(iterable, 0, std::plus<>());
            REQUIRE(10 == sum);
        }

        SUBCASE("iterator types") {
            [[maybe_unused]] auto begin = iterable.begin();
            [[maybe_unused]] auto const_begin = const_iterable.begin();

            static_assert(std::same_as<decltype(*const_begin), const int &>);
            static_assert(std::same_as<decltype(*begin), int &>);

            static_assert(!std::assignable_from<decltype(*const_begin), int>);
            static_assert(std::assignable_from<decltype(*begin), int>);
        }

        SUBCASE("increment semantics") {
            auto base = iterable.begin();
            [[maybe_unused]] auto old = base++;
            CHECK(iterable.begin() == old);
            CHECK(++old == base);
        }

        SUBCASE("iterator dereference") {
            CHECK(0 == *iterable.begin());
            CHECK(1 == *++iterable.begin());

            CHECK_THROWS([&] {
                (void)*std::ranges::next(iterable.begin(), iterable.end());
            }());
        }
    }

    TEST_CASE("list car/cdr/cndr/index") {
        List test_list{0, 1, 2, 3, 4};

        CHECK(0 == test_list.car());
        CHECK(1 == test_list[1]);
        CHECK(1 == *test_list.cdr().begin());
        CHECK(3 == *test_list.cndr(3).begin());

        const List test_const_list{test_list}; // NOLINT
        CHECK(0 == test_const_list.car());
        CHECK(1 == test_const_list[1]);
        CHECK(1 == *test_const_list.cdr().begin());
        CHECK(3 == *test_const_list.cndr(3).begin());

        List<int> test_empty_list{};
        CHECK(std::nullopt == test_empty_list.car());
    }
}
