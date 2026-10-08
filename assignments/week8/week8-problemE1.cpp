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
#include <stdexcept>
#include <string>
#include <string_view>

#include <doctest/doctest.h>

#include "include/week8-problemE1.hpp"

using namespace std::string_view_literals;

// IMPLEMENTATION
auto hutzdog_cs2_week8::IntArray::operator[](std::size_t index) -> int & {
    if (index >= this->size()) {
        throw std::logic_error("Index out of bounds");
    }
    return this->data[index];
}

void hutzdog_cs2_week8::IntArray::display() const {
    std::print("(IntArray");
    for (std::size_t i = 0; i < this->size(); i++) {
        std::print(" {}", this->data[i]);
    }
    std::println(")");
}

// TESTS
TEST_CASE("testing the IntArray class") {
    using hutzdog_cs2_week8::IntArray;

    SUBCASE("test iteration 1 properties") {
        IntArray foo{4};
        foo[3] = 4;
        CHECK(4 == foo[3]);
    }

    SUBCASE("test iteration 2 properties (copy)") {
        IntArray foo{4};
        IntArray bar{foo};

        for (std::size_t i = 0; i < foo.size(); i++) {
            CHECK(foo[i] == bar[i]);
        }
    }
}
