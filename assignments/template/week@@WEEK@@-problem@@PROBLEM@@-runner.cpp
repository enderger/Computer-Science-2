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

#include "include/week@@WEEK@@-problem@@PROBLEM@@.hpp"

using namespace std::string_view_literals;

// MAIN
// TODO: Replace with your actual code
auto main(int argc, const char *argv[]) -> int {
    std::println(stderr, "Week @@WEEK@@ Problem @@PROBLEM@@ assignment");
    switch (argc) {
        case 1:
            std::println("{}", hutzdog_cs2_week@@WEEK@@::get_greeting("World"));
            return 0;

        case 2:
            std::println("{}", hutzdog_cs2_week@@WEEK@@::get_greeting(std::string_view(argv[1])));
            return 0;

        default:
            std::println(
                stderr,
                "Invalid usage. Usage: {} [name]",
                argv[0]
            );
            return 1;
    }
}
