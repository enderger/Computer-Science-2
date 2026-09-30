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

#include "include/week6-probleme1.hpp"

using namespace std::string_view_literals;

// IMPLEMENTATION

// TESTS
// TODO: Implement the tests here
TEST_CASE("testing the factorial function") {
    CHECK(hutzdog_cs2_week6::factorial(0U) == 1);
    CHECK(hutzdog_cs2_week6::factorial(1U) == 1);
    CHECK(hutzdog_cs2_week6::factorial(2U) == 2);
    CHECK(hutzdog_cs2_week6::factorial(3U) == 6);
    CHECK(hutzdog_cs2_week6::factorial(4U) == 24);
    CHECK(hutzdog_cs2_week6::factorial(5U) == 120);
}
