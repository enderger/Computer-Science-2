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

#include "include/week8-problemE1.hpp"

using namespace std::string_view_literals;

// MAIN
// TODO: Replace with your actual code
auto main() -> int {
    std::println(stderr, "Week 8 Problem E1 assignment");

    using hutzdog_cs2_week8::IntArray;
    IntArray a{4}; // NOLINT
    a[0] = 10;     // NOLINT
    a[1] = 20;     // NOLINT
    a[2] = 30;     // NOLINT
    a[3] = 40;     // NOLINT
    a.display();

    IntArray b{a}; // NOLINT
    a[0] = 99;     // NOLINT
    a.display();
    b.display();
}
