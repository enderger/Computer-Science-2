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
#include <string_view>

#include "week3-problem3.hpp"
#include <cs2lib/hash.hpp>

using namespace cs2_lib::literals;
using namespace std::string_view_literals;

// MAIN
// Compile-time hash function based on a faster version of K&R's implementation

// TODO: Replace with your actual code
auto main(int argc, const char *argv[]) -> int {
    std::println(stderr, "Week 3 Problem 3 assignment"sv);

    if (argc < 2) {
        std::println("Invalid usage. Usage: {} <OPERATION> [ARGS]"sv, argv[0]);
        return -1;
    }

    switch (cs2_lib::knr_hash(argv[1])) {
    case "deposit"_knr_hash:
        throw std::runtime_error("TODO");
    case "withdraw"_knr_hash:
        throw std::runtime_error("TODO");
    case "balance"_knr_hash:
        throw std::runtime_error("TODO");
    default:
        std::println("Invalid command: '{}'"sv, argv[1]);
    }
}
