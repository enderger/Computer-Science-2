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
#include <string>

#include <doctest/doctest.h>

#include "include/week7-problemE1.hpp"

using namespace std::string_view_literals;

// IMPLEMENTATION

// TESTS
TEST_CASE("testing the swap_values function") {
    std::string fst{"First"};
    std::string snd{"Second"};
    hutzdog_cs2_week7::swap_values(fst, snd);

    CHECK(fst == "Second");
    CHECK(snd == "First");
}
