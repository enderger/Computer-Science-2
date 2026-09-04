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
#include <print>

#include "include/week3-problem2.hpp"

using namespace std::string_view_literals;

// MAIN
// TODO: Replace with your actual code
auto main() -> int {
    std::println(stderr, "Week 3 Problem 2 assignment");

    std::println("Vehicle demonstration:");

    std::println("Volkswagen Campervan:");
    hutzdog_cs2_week3::Vehicle campervan{"Light Blue", 20'000};
    std::println("{}", campervan);

    std::println("Panda:");
    hutzdog_cs2_week3::Car panda{"Gray", 5'000, 4};
    panda.printInfo();

    std::println("Ford F150");
    hutzdog_cs2_week3::Truck f150{"Hot Pink (Custom)", 25'000, 3'325};
    f150.printInfo();
}
