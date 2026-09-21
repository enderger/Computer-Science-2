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
#include "cs2lib/sexp.hpp"
#include "cs2lib/util.hpp"

#include <algorithm>
#include <doctest/doctest.h>
#include <ranges>
#include <string>

// Depth of the CS2LIB fuzzing
//
// According to Claude, 3 finishes in about 2 minutes, 4 takes 9 hours, and 5
// is impractical on even the most powerful end of consumer grade
#ifndef CS2LIB_FULL_FUZZ_DEPTH
#define CS2LIB_FULL_FUZZ_DEPTH 3
#endif // CS2LIB_FULL_FUZZ_DEPTH

// Depth of the CS2LIB restricted fuzzing
//
// TODO: Analyze timing to find optimal setting
#ifndef CS2LIB_CURATED_FUZZ_DEPTH
#define CS2LIB_CURATED_FUZZ_DEPTH 5
#endif // CS2LIB_CURATED_FUZZ_DEPTH

namespace {
inline auto generate_full_alphabet() -> std::string {
    std::string data;
    std::generate_n(std::back_inserter(data), 256, [idx = 0]() mutable -> char {
        return static_cast<char>(idx++);
    });
    return data;
}

[[maybe_unused]] inline auto generate_curated_alphabet() -> std::string {
    return generate_full_alphabet()
           // A short name here is useful, as we already hit 80 columns quite
           // quickly
           // NOLINTNEXTLINE
           | std::views::filter([](char c) -> bool {
                 return c == '(' || c == ')' || c == '"' || c == '\0' ||
                        c == '\\' || c == '.' || std::isspace(c) ||
                        std::isalnum(c);
             }) |
           std::ranges::to<std::string>();
}

} // namespace

TEST_CASE("fuzz the lexer") {
    struct Category {
        std::string alphabet;
        uint8_t len;
    };

    Category full{generate_full_alphabet(), CS2LIB_FULL_FUZZ_DEPTH};
    Category curated{generate_curated_alphabet(), CS2LIB_CURATED_FUZZ_DEPTH};
    Category category = GENERATE(full, curated);

    [[maybe_unused]] std::string buf(category.len, '\0');
    [[maybe_unused]] uintptr_t alpha_len = category.alphabet.length();
    cs2_lib::sexp::Lexer lexer{cs2_lib::sexp::Settings{}};

    CHECK_NOTHROW([&] {
        for (uint64_t position = 0;
             position < cs2_lib::util::pow(alpha_len, category.len);
             position++) {
            uint64_t pos = position;
            for (uint8_t buf_idx = 0; buf_idx < category.len; ++buf_idx) {
                buf[buf_idx] = category.alphabet[pos % alpha_len];
                pos /= alpha_len;
            }
            for (auto tok : buf | lexer("test.sexp")) {
                static_cast<void>(tok);
            }
        }
    }());
}
