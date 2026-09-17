/*
 * Copyright 2026 Danielle Hutzley
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
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

#include "include/week3-problem3.hpp"

using namespace std::string_view_literals;

// IMPLEMENTATION

// TESTS
TEST_CASE("testing the Account class") {
    hutzdog_cs2_week3::Account account1{"Brent", 20'000};
    hutzdog_cs2_week3::Account account2{"Jai", 3'000'000};

    SUBCASE("testing getters") {
        CHECK(account1.getOwnerName() == "Brent");
        CHECK(account1.getBalance() == 20'000);

        CHECK(account2.getOwnerName() == "Jai");
        CHECK(account2.getBalance() == 3'000'000);
    }

    SUBCASE("testing setters") {
        account1.deposit(5'000);
        CHECK(account1.getBalance() == 25'000);

        account1.withdraw(2'500);
        CHECK(account1.getBalance() == 22'500);
    }

    SUBCASE("testing stringification") {
        CHECK(std::format("{}", account1) ==
              "(Account (owner-name . \"Brent\") (balance . 20000))");
        CHECK(std::format("{}", account2) ==
              "(Account (owner-name . \"Jai\") (balance . 3000000))");
    }
}
