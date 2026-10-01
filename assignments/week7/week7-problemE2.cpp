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

#include "include/week7-problemE2.hpp"

using namespace std::string_view_literals;

// IMPLEMENTATION

// TESTS
// TODO: Implement the tests here
TEST_CASE("testing the array_stats function") {
    [[maybe_unused]] int foo[] = {1, 1, 2, 3, 5, 8, 13};
    [[maybe_unused]] double bar[] = {1.0, 2.0, 8.0, 0.5};

    CHECK("(Array (min . 1) (max . 13) (sum . 33))" ==
          hutzdog_cs2_week7::array_stats<int, 7>(foo));

    CHECK("(Array (min . 0.5) (max . 8) (sum . 11.5))" ==
          hutzdog_cs2_week7::array_stats<double, 4>(bar));
}
