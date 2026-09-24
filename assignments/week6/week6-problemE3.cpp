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
#include <cctype>
#include <format>
#include <functional>
#include <ranges>
#include <string>
#include <string_view>

#include <doctest/doctest.h>

#include "include/week6-problemE3.hpp"

using namespace std::string_view_literals;

// IMPLEMENTATION
auto hutzdog_cs2_week6::is_palindrome(std::string_view maybe_palindrome)
    -> bool {
    std::string data = maybe_palindrome |
                       std::views::filter([](const auto &chara) -> bool {
                           return std::isalnum(chara, std::locale::classic());
                       }) |
                       std::views::transform([](const auto &chara) -> char {
                           return std::tolower(chara, std::locale::classic());
                       }) |
                       std::ranges::to<std::string>();

    const std::function<bool(std::string_view)> _impl =
        [&_impl](std::string_view data) -> bool {
        if (data.length() <= 1) {
            return true;
        }
        return data[0] == data[data.length() - 1] &&
               _impl(data.substr(1, data.length() - 2));
    };

    return _impl(data);
}

// TESTS
// TODO: Implement the tests here
TEST_CASE("testing the maybe_palindrome function") {
    CHECK(hutzdog_cs2_week6::is_palindrome("oofoo"sv));
    CHECK(hutzdog_cs2_week6::is_palindrome("racecar"sv));
    CHECK(hutzdog_cs2_week6::is_palindrome("Taco.cat"sv));
    CHECK(!hutzdog_cs2_week6::is_palindrome("foo"sv));
    CHECK(!hutzdog_cs2_week6::is_palindrome("bAr"sv));
}
