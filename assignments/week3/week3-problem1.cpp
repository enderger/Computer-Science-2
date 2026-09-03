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

#include <doctest/doctest.h>

#include "include/week3-problem1.hpp"

using namespace std::string_view_literals;

// HELPERS
namespace {
// HACK: This is just a testable version of printInfo used for testing purposes
constexpr auto _formatEmployee(std::string name, double baseSalary)
    -> std::string {
    return std::format("I'm {} and I make a base salary of ${}", name,
                       baseSalary);
}
} // namespace

// IMPLEMENTATION
void hutzdog_cs2_week3::Employee::printInfo() const {
    std::println("{}", _formatEmployee(this->name, this->baseSalary));
}

[[nodiscard]] constexpr auto
hutzdog_cs2_week3::Manager::totalCompensation() const -> double {
    return baseSalary * (1.0 + bonusPercentage / 100.0);
}

// TESTS
TEST_CASE("testing the Employee class's formatting") {
    CHECK_EQ(_formatEmployee("Sullivan", 14.42),
             "I'm Sullivan and I make a base salary of $14.42");
}
TEST_CASE("testing the Manager's salary computation") {
    CHECK_EQ(hutzdog_cs2_week3::Manager("Mr. Waternoose", 24.84, 100.0)
                 .totalCompensation(),
             49.68);
}
