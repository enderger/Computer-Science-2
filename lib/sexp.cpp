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
#include <array>
#include <ranges>
#include <string>
#include <thread>
#include <vector>

#include <doctest/doctest.h>

// Depth of the CS2LIB fuzzing
#ifndef CS2LIB_FULL_FUZZ_DEPTH
#define CS2LIB_FULL_FUZZ_DEPTH 3
#endif // CS2LIB_FULL_FUZZ_DEPTH

// Depth of the CS2LIB restricted fuzzing
//
#ifndef CS2LIB_CURATED_FUZZ_DEPTH
#define CS2LIB_CURATED_FUZZ_DEPTH 5
#endif // CS2LIB_CURATED_FUZZ_DEPTH

// The number of threads to spawn in the fuzz
#ifndef CS2LIB_FUZZ_THREADS
#define CS2LIB_FUZZ_THREADS 8
#endif

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
                        c == '\\' || c == '.' || c == ' ' || c == '\t' ||
                        c == '\r' || c == '\n' || c == '\v' || c == '\f' ||
                        c == '+' || c == '-' || c == '`' || c == '\'' ||
                        c == ',' || c == ';' ||
                        std::isalnum(c, std::locale::classic());
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

    [[maybe_unused]] uintptr_t alpha_len = category.alphabet.length();
    cs2_lib::sexp::Lexer lexer{cs2_lib::sexp::Settings{}};

    {
        const uintptr_t max_threads =
            std::max(std::thread::hardware_concurrency(), 1U);
        const uintptr_t thread_count =
            std::min(uintptr_t{CS2LIB_FUZZ_THREADS}, max_threads);
        const uintptr_t max = cs2_lib::util::pow(alpha_len, category.len);

        std::atomic<uintptr_t> cursor{0};
        constexpr uintptr_t CHUNK = 1 << 16;

        std::array<std::optional<std::jthread>, CS2LIB_FUZZ_THREADS> threads;
        for (uintptr_t thread{}; thread < thread_count; thread++) {
            threads[thread].emplace([&] -> void {
                [[maybe_unused]] std::string buf(category.len, '\0');
                for (uintptr_t base; (base = cursor.fetch_add(CHUNK)) < max;) {
                    for (auto chunk_idx = base;
                         chunk_idx < std::min(base + CHUNK, max); ++chunk_idx) {
                        [[maybe_unused]] const uint64_t index = chunk_idx;
                        INFO(index);
                        CHECK_NOTHROW([&] {
                            uint64_t pos = index;
                            for (uint8_t buf_pos = 0; buf_pos < category.len;
                                 buf_pos++) {
                                buf[buf_pos] =
                                    category.alphabet[pos % alpha_len];
                                pos /= alpha_len;
                            }

                            for (auto tok : buf | lexer("test.sexp")) {
                                static_cast<void>(tok);
                            }
                        }());
                    }
                }
            });
        }
    }
}

TEST_CASE("test the lexer") {
    using namespace cs2_lib::sexp;
    lexer::Lexer lexer{cs2_lib::sexp::Settings{}};

    SUBCASE("basic tests") {
        const std::string_view file_name = "basic.vtyp";
        const std::string_view text = "(console.log \"Hello, world!\")";

        std::vector<lexer::Token> expected{
            lexer::Token{
                lexer::TokenType::LParen,
                Span{
                    .file_name = file_name,
                    .begin_line = 1,
                    .begin_column = 0,
                    .end_line = 1,
                    .end_column = 1,
                    .index = 0,
                    .length = 1,
                },
                "("sv,
            },
            lexer::Token{
                lexer::TokenType::Atom,
                Span{
                    .file_name = file_name,
                    .begin_line = 1,
                    .begin_column = 1,
                    .end_line = 1,
                    .end_column = 12,
                    .index = 1,
                    .length = 11,
                },
                "console.log"sv,
            },
            lexer::Token{
                lexer::TokenType::Atom,
                Span{
                    .file_name = file_name,
                    .begin_line = 1,
                    .begin_column = 13,
                    .end_line = 1,
                    .end_column = 28,
                    .index = 13,
                    .length = 15,
                },
                "\"Hello, world!\""sv,
            },
            lexer::Token{lexer::TokenType::RParen,
                         Span{
                             .file_name = file_name,
                             .begin_line = 1,
                             .begin_column = 28,
                             .end_line = 1,
                             .end_column = 29,
                             .index = 28,
                             .length = 1,
                         },
                         ")"sv},
            lexer::Token{lexer::TokenType::Eof,
                         Span{
                             .file_name = file_name,
                             .begin_line = 1,
                             .begin_column = 29,
                             .end_line = 1,
                             .end_column = 29,
                             .index = 29,
                             .length = 0,
                         },
                         ""sv}};

        auto actual = text | lexer(file_name) |
                      std::ranges::to<std::vector<lexer::Token>>();
        CHECK_MESSAGE(expected.size() == actual.size(),
                      "One side of the token stream hit EOF first: ",
                      (expected.size() < actual.size()) ? "expected" : "actual",
                      " side");
        for ([[maybe_unused]] auto [expected_tok, actual_tok] :
             std::views::zip(expected, actual)) {
            CHECK(expected_tok == actual_tok);
        }
    }

    SUBCASE("empty file with comment") {
        const std::string_view file_name{"empty_comment.vtyp"};
        const std::string_view text{"; lorem ipsum dolor sit amet"};
        const auto tokens = text | lexer(file_name) |
                            std::ranges::to<std::vector<lexer::Token>>();
        CHECK(tokens.size() == 1);
    }

    SUBCASE("single tokens") {
        using namespace cs2_lib::sexp;
        struct StCase {
            std::string_view text;
            lexer::Token token;
            Lexer test_lexer{Settings{}};
        };

        const std::string_view file_name{"string.vtyp"sv};
        const StCase unterminated_string{
            .text = "\"",
            .token = lexer::Token{
                lexer::TokenType::Error,
                Span{
                    .file_name = file_name,
                    .begin_line = 1,
                    .begin_column = 0,
                    .end_line = 1,
                    .end_column = 1,
                    .index = 0,
                    .length = 1,
                },
                lexer::ErrorData{"unterminated string slice"},
            }};
        const StCase unterminated_content{
            .text = "\"abc",
            .token = lexer::Token{
                lexer::TokenType::Error,
                Span{
                    .file_name = file_name,
                    .begin_line = 1,
                    .begin_column = 0,
                    .end_line = 1,
                    .end_column = 4,
                    .index = 0,
                    .length = 4,
                },
                lexer::ErrorData{"unterminated string slice"},
            }};
        const StCase unterminated_escape{
            .text = "\"abc\\",
            .token = lexer::Token{
                lexer::TokenType::Error,
                Span{
                    .file_name = file_name,
                    .begin_line = 1,
                    .begin_column = 0,
                    .end_line = 1,
                    .end_column = 5,
                    .index = 0,
                    .length = 5,
                },
                lexer::ErrorData{"unterminated string slice"},
            }};
        const StCase unterminated_quote_escape{
            .text = R"("abc\")",
            .token = lexer::Token{
                lexer::TokenType::Error,
                Span{
                    .file_name = file_name,
                    .begin_line = 1,
                    .begin_column = 0,
                    .end_line = 1,
                    .end_column = 6,
                    .index = 0,
                    .length = 6,
                },
                lexer::ErrorData{"unterminated string slice"},
            }};
        const StCase unterminated_alternate_quotes{
            .text = "`\"",
            .token =
                lexer::Token{
                    lexer::TokenType::Error,
                    Span{
                        .file_name = file_name,
                        .begin_line = 1,
                        .begin_column = 0,
                        .end_line = 1,
                        .end_column = 2,
                        .index = 0,
                        .length = 2,
                    },
                    lexer::ErrorData{"unterminated string slice"},
                },
            .test_lexer = Lexer{Settings{std::locale::classic(), {'"', '`'}}},
        };

        const StCase empty_string{.text = R"("")",
                                  .token = lexer::Token{
                                      lexer::TokenType::Atom,
                                      Span{
                                          .file_name = file_name,
                                          .begin_line = 1,
                                          .begin_column = 0,
                                          .end_line = 1,
                                          .end_column = 2,
                                          .index = 0,
                                          .length = 2,
                                      },
                                      R"("")"sv,
                                  }};
        const StCase escape_string{.text = R"("a\\")",
                                   .token = lexer::Token{
                                       lexer::TokenType::Atom,
                                       Span{
                                           .file_name = file_name,
                                           .begin_line = 1,
                                           .begin_column = 0,
                                           .end_line = 1,
                                           .end_column = 5,
                                           .index = 0,
                                           .length = 5,
                                       },
                                       R"("a\\")"sv,
                                   }};
        const StCase multiple_escapes_string{.text = R"("a\\\\b")",
                                             .token = lexer::Token{
                                                 lexer::TokenType::Atom,
                                                 Span{
                                                     .file_name = file_name,
                                                     .begin_line = 1,
                                                     .begin_column = 0,
                                                     .end_line = 1,
                                                     .end_column = 8,
                                                     .index = 0,
                                                     .length = 8,
                                                 },
                                                 R"("a\\\\b")"sv,
                                             }};
        const StCase quote_escape_string{.text = R"("a\"b")",
                                         .token = lexer::Token{
                                             lexer::TokenType::Atom,
                                             Span{
                                                 .file_name = file_name,
                                                 .begin_line = 1,
                                                 .begin_column = 0,
                                                 .end_line = 1,
                                                 .end_column = 6,
                                                 .index = 0,
                                                 .length = 6,
                                             },
                                             R"("a\"b")"sv,
                                         }};
        const StCase newline_string{.text = "\"foo\nbar\"",
                                    .token = lexer::Token{
                                        lexer::TokenType::Atom,
                                        Span{
                                            .file_name = file_name,
                                            .begin_line = 1,
                                            .begin_column = 0,
                                            .end_line = 2,
                                            .end_column = 4,
                                            .index = 0,
                                            .length = 9,
                                        },
                                        "\"foo\nbar\""sv,
                                    }};

        const StCase test_case = GENERATE(
            unterminated_string, unterminated_content, unterminated_escape,
            unterminated_quote_escape, unterminated_alternate_quotes,

            empty_string, escape_string, multiple_escapes_string,
            quote_escape_string, newline_string);

        const auto tokens = test_case.text | test_case.test_lexer(file_name);
        CHECK(*tokens.begin() == test_case.token);
    }
}
