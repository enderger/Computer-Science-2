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
#include "cs2lib/util.hpp"

#ifndef HUTZDOG_CS2_LIB_DOCTEST
#define HUTZDOG_CS2_LIB_DOCTEST
#include <doctest/doctest.h>
#endif // HUTZDOG_CS2_LIB_DOCTEST

TEST_CASE("power function") {
    CHECK(cs2_lib::util::pow(std::uint64_t{2}, std::uint8_t{0}) == 1);
    CHECK(cs2_lib::util::pow(std::uint64_t{2}, std::uint8_t{1}) == 2);
    CHECK(cs2_lib::util::pow(std::uint64_t{2}, std::uint8_t{4}) == 16);
    CHECK(cs2_lib::util::pow(std::uint64_t{10}, std::uint8_t{3}) == 1000);
}
