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
#include <string>
#include <string_view>

#include <doctest/doctest.h>

#include "include/week@@WEEK@@-problem@@PROBLEM@@.hpp"

using namespace std::string_view_literals;

// IMPLEMENTATION
// TODO: Implement the solution here
auto hutzdog_cs2_week@@WEEK@@::get_greeting(std::string_view who) -> std::string {
    return std::format("Hello, {}!", who);
}

// TESTS
// TODO: Implement the tests here
TEST_CASE("testing the get_greeting function") {
    CHECK(hutzdog_cs2_week@@WEEK@@::get_greeting("World") == "Hello, World!");
    CHECK(hutzdog_cs2_week@@WEEK@@::get_greeting("Professor") == "Hello, Professor!");
}
