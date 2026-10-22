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

#include <doctest/doctest.h>
#include <list>
#include <vector>

#include "include/week10-problemE1.hpp"

using namespace std::string_view_literals;

// IMPLEMENTATION

// TESTS
// TODO: Implement the tests here
TEST_SUITE("testing the Stack class") {
    using hutzdog_cs2_week10::Stack;

    TEST_CASE_TEMPLATE("construction", T, std::vector<int>, std::list<int>) {
        Stack<T> empty_default_stack{};
        Stack element_stack{T{1, 2, 3}};
    }

    TEST_CASE_TEMPLATE("push/pop/peek", T, std::vector<int>, std::list<int>) {
        Stack<T> stack{};
        stack.push_back(0);
        stack.push_back(1);
        CHECK(1 == stack.peek_back());
        CHECK(1 == stack.pop_back());
        CHECK(0 == stack.pop_back());
        CHECK(std::nullopt == stack.pop_back());
    }

    // NOLINTBEGIN
    class Emplacable {
      private:
        int i, j;

      public:
        constexpr Emplacable(int i, int j) : i{i}, j{j} {};
        constexpr auto operator==(const Emplacable &other) const
            -> bool = default;
    };
    // NOLINTEND

    TEST_CASE_TEMPLATE("emplace", T, std::vector<Emplacable>,
                       std::list<Emplacable>) {
        Stack<T> stack{};
        stack.emplace_back(0, 0);
        stack.emplace_back(1, 1);
        CHECK(Emplacable{1, 1} == stack.pop_back());
        CHECK(Emplacable{0, 0} == stack.pop_back());
        CHECK(std::nullopt == stack.pop_back());
    }
}
