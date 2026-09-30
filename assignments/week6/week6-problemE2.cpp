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

#include "include/week6-problemE2.hpp"

using namespace std::string_view_literals;

// IMPLEMENTATION

// TESTS
// TODO: Implement the tests here
TEST_CASE("testing the sum_of_digits function") {
    CHECK(hutzdog_cs2_week6::sum_of_digits(0U) == 0U);
    CHECK(hutzdog_cs2_week6::sum_of_digits(1U) == 1U);
    CHECK(hutzdog_cs2_week6::sum_of_digits(10U) == 1U);
    CHECK(hutzdog_cs2_week6::sum_of_digits(16U) == 7U);
    CHECK(hutzdog_cs2_week6::sum_of_digits(128U) == 11U);
}
