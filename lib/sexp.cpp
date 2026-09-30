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
#include <optional>
#include <ranges>
#include <string>
#include <thread>
#include <variant>
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

static_assert(std::same_as<std::variant<std::string_view,
                                        cs2_lib::sexp::lexer::ErrorData>,
                           decltype(cs2_lib::sexp::lexer::Token::data)>,
              "Update the tests to reflect the new version of data");

namespace {
inline auto generate_full_alphabet() -> std::string {
    std::string data;
    // NOLINTNEXTLINE
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

TEST_CASE("test the lexer") {
    using namespace cs2_lib::sexp;
    lexer::Lexer lexer{cs2_lib::sexp::Settings{}};

    SUBCASE("hello world") {
        constexpr std::string_view file_name = "basic.vtyp"sv;
        constexpr std::string_view text = "(console.log \"Hello, world!\")"sv;
        constexpr Span::Factory spanf{file_name, text};

        const auto expected =
            std::to_array({lexer::Token{
                               lexer::TokenType::LParen,
                               spanf.make({
                                   .begin_pos = {.line = 1, .col = 0},
                                   .end_pos = {.line = 1, .col = 1},
                                   .index = 0,
                                   .length = 1,
                               }),
                               "("sv,
                           },
                           lexer::Token{
                               lexer::TokenType::Atom,
                               spanf.make({
                                   .begin_pos = {.line = 1, .col = 1},
                                   .end_pos = {.line = 1, .col = 12},
                                   .index = 1,
                                   .length = 11,
                               }),
                               "console.log"sv,
                           },
                           lexer::Token{
                               lexer::TokenType::Atom,
                               spanf.make({
                                   .begin_pos = {.line = 1, .col = 13},
                                   .end_pos = {.line = 1, .col = 28},
                                   .index = 13,
                                   .length = 15,
                               }),
                               "\"Hello, world!\""sv,
                           },
                           lexer::Token{lexer::TokenType::RParen,
                                        spanf.make({
                                            .begin_pos = {.line = 1, .col = 28},
                                            .end_pos = {.line = 1, .col = 29},
                                            .index = 28,
                                            .length = 1,
                                        }),
                                        ")"sv},
                           lexer::Token{lexer::TokenType::Eof,
                                        spanf.make({
                                            .begin_pos = {.line = 1, .col = 29},
                                            .end_pos = {.line = 1, .col = 29},
                                            .index = 29,
                                            .length = 0,
                                        }),
                                        ""sv}});

        auto actual = text | lexer(file_name) |
                      std::ranges::to<std::vector<lexer::Token>>();
        CHECK_MESSAGE(expected.size() == actual.size(),
                      "One side of the token stream hit EOF first: ",
                      (expected.size() < actual.size()) ? "expected"sv
                                                        : "actual"sv,
                      " side");
        for ([[maybe_unused]] auto [expected_tok, actual_tok] :
             std::views::zip(expected, actual)) {
            CHECK(expected_tok == actual_tok);
        }
    }

    SUBCASE("whitespace only") {
        struct WsCase {
            // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
            std::string_view text;

            // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
            Span eof_span;

            constexpr WsCase(std::string_view text, std::string_view file_name,
                             Span::Position pos, uint64_t index)
                : text{text}, eof_span{Span::Factory{file_name, text}.make({
                                  .begin_pos = pos,
                                  .end_pos = pos,
                                  .index = index,
                                  .length = 0,
                              })} {}
        };

        const WsCase test_case =
            GENERATE(WsCase(""sv, "empty_file.vtyp"sv, {1, 0}, 0),
                     WsCase(" \t\r\n", "whitespace_only.vtyp"sv, {2, 0}, 4),

                     WsCase("; lorem ipsum dolor sit amit"sv,
                            "comment_eof.vtyp"sv, {1, 28}, 28),
                     WsCase("; lorem ipsum dolor sit amit\n ;Foo bar baz"sv,
                            "two_comments.vtyp"sv, {2, 13}, 42),
                     WsCase("; lorem ipsum dolor sit amit\r\n ;Foo bar baz"sv,
                            "two_comments_cr.vtyp"sv, {2, 13}, 43),
                     WsCase(R"(; (foo "bar" . :baz))"sv,
                            "commented_out_code.vtyp"sv, {1, 20}, 20),
                     WsCase(R"(; (foo "bar" . "baz\"))"sv,
                            "commented_invalid.vtyp"sv, {1, 22}, 22),
                     WsCase(";"sv, "empty_comment.vtyp"sv, {1, 1}, 1),
                     WsCase("\v\f"sv, "exotic_whitespace"sv, {1, 2}, 2));
        const auto tokens = test_case.text |
                            lexer(test_case.eof_span.get_file_name()) |
                            std::ranges::to<std::vector<lexer::Token>>();
        CHECK(tokens.size() == 1);
        CHECK(tokens.begin()->span == test_case.eof_span);
    }

    SUBCASE("single tokens") {
        using namespace cs2_lib::sexp;
        struct StCase {
            // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
            std::string_view text;
            // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
            std::string_view file_name;
            // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
            Lexer lexer{Settings{}};
            // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
            lexer::Token token;

            constexpr StCase(
                std::string_view text, std::string_view file_name,
                lexer::TokenType tok_type, Span::Position end, uint64_t length,
                const std::optional<std::variant<
                    std::string_view, lexer::ErrorData>> &expected_data = {},
                uint64_t index = 0, Lexer lexer = {Settings{}})
                : text{text}, file_name{file_name}, lexer{std::move(lexer)},
                  token{tok_type,
                        Span::Factory{file_name, text}.make({
                            .begin_pos = {.line = 1, .col = 0},
                            .end_pos = end,
                            .index = index,
                            .length = length,
                        }),
                        expected_data.value_or(text)} {}
        };

        const StCase test_case = GENERATE(
            StCase("\""sv, "unterminated_string_empty.vtyp"sv,
                   lexer::TokenType::Error, {1, 1}, 1,
                   lexer::ErrorData{"unterminated string"}),
            StCase("\"abc"sv, "unterminated_string.vtyp"sv,
                   lexer::TokenType::Error, {1, 4}, 4,
                   lexer::ErrorData{"unterminated string"}),
            StCase("\"abc\\"sv, "unterminated_escape.vtyp"sv,
                   lexer::TokenType::Error, {1, 5}, 5,
                   lexer::ErrorData{"unterminated string"}),
            StCase(R"("abc\")"sv, "unterminated_quote_escape.vtyp"sv,
                   lexer::TokenType::Error, {1, 6}, 6,
                   lexer::ErrorData{"unterminated string"}),
            StCase("`\""sv, "unterminated_alternate_quotes.vtyp"sv,
                   lexer::TokenType::Error, {1, 2}, 2,
                   lexer::ErrorData{"unterminated string"}, 0,
                   Lexer{Settings{
                       .locale = std::locale::classic(),
                       .quote_characters = {'"', '`'},
                   }}),
            StCase("\"`\""sv, "terminated_alternate_quotes.vtyp"sv,
                   lexer::TokenType::Atom, {1, 3}, 3, "\"`\""sv, 0,
                   Lexer{Settings{
                       .locale = std::locale::classic(),
                       .quote_characters = {'"', '`'},
                   }}),
            StCase(R"("")"sv, "empty_string.vtyp"sv, lexer::TokenType::Atom,
                   {1, 2}, 2),
            StCase(R"("a\\")"sv, "escape_string.vtyp"sv, lexer::TokenType::Atom,
                   {1, 5}, 5),
            StCase(R"("a\\\\b")"sv, "multi_escape_string.vtyp"sv,
                   lexer::TokenType::Atom, {1, 8}, 8),
            StCase(R"("a\"b")"sv, "quote_escape_string.vtyp"sv,
                   lexer::TokenType::Atom, {1, 6}, 6),
            StCase("\"foo\nbar\""sv, "newline_string.vtyp"sv,
                   lexer::TokenType::Atom, {2, 4}, 9),
            StCase("a", "atom_eof.vtyp"sv, lexer::TokenType::Atom, {1, 1}, 1),

            StCase("a;Henlo"sv, "atom_comment.vtyp"sv, lexer::TokenType::Atom,
                   {1, 1}, 1, {"a"}),
            StCase(".3.14159"sv, "atom_dots"sv, lexer::TokenType::Atom, {1, 8},
                   8),
            StCase(".."sv, "atom_multiple_dots.vtyp"sv, lexer::TokenType::Atom,
                   {1, 2}, 2),
            StCase("."sv, "terminator_eof.vtyp"sv,
                   lexer::TokenType::ListTerminator, {1, 1}, 1),
            StCase(". "sv, "terminator_ws.vtyp"sv,
                   lexer::TokenType::ListTerminator, {1, 1}, 1, "."sv),

            StCase("("sv, "left_paren.vtyp"sv, lexer::TokenType::LParen, {1, 1},
                   1),
            StCase(")"sv, "right_paren.vtyp"sv, lexer::TokenType::RParen,
                   {1, 1}, 1),
            StCase("\xff\x80"sv, "high_exotic_character_atom.vtyp"sv,
                   lexer::TokenType::Atom, {1, 2}, 2));

        const auto tokens =
            test_case.text | test_case.lexer(test_case.file_name);
        auto token_iter = tokens.begin();
        CHECK(test_case.token == *token_iter);
        token_iter++;
        CHECK((*token_iter).type == lexer::TokenType::Eof);
    }

    SUBCASE("other small tests") {
        using namespace cs2_lib::sexp;
        struct SmallCase {
            struct ProtoToken {
                lexer::TokenType type;
                std::variant<std::string_view, lexer::ErrorData> data;
                Span::PositionSpan span;
            };

            std::string_view text, file_name; // NOLINT
            std::vector<lexer::Token> tokens; // NOLINT
            Lexer lexer{Settings{}};          // NOLINT

            constexpr SmallCase(std::string_view text,
                                std::string_view file_name,
                                std::vector<ProtoToken> tokens)
                : SmallCase(text, file_name, Lexer{Settings{}}, tokens) {}

            // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
            constexpr SmallCase(std::string_view text,
                                std::string_view file_name, Lexer lexer,
                                std::vector<ProtoToken> tokens)
                : text{text}, file_name{file_name}, lexer{std::move(lexer)} {
                tokens.reserve(tokens.size());
                const Span::Factory span_factory{file_name, text};
                for (ProtoToken &proto : tokens) {
                    this->tokens.emplace_back(std::move(proto.type),
                                              span_factory.make(proto.span),
                                              std::move(proto.data));
                }
            }
        };

        const SmallCase test_case =
            GENERATE(
                SmallCase("(a);comment\nb"sv, "comment_between_forms.vtyp",
                          {
                              {
                                  .type = lexer::TokenType::LParen,
                                  .data = "("sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 0},
                                          .end_pos = {.line = 1, .col = 1},
                                          .index = 0,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "a"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 1},
                                          .end_pos = {.line = 1, .col = 2},
                                          .index = 1,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::RParen,
                                  .data = ")"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 2},
                                          .end_pos = {.line = 1, .col = 3},
                                          .index = 2,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "b"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 2, .col = 0},
                                          .end_pos = {.line = 2, .col = 1},
                                          .index = 12,
                                          .length = 1,

                                      },
                              },
                              {
                                  .type = lexer::TokenType::Eof,
                                  .data = ""sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 2, .col = 1},
                                          .end_pos = {.line = 2, .col = 1},
                                          .index = 13,
                                          .length = 0,
                                      },
                              },
                          }),

                SmallCase(R"(abc"def"ghi)", "atom_then_quoted.vtyp"sv,
                          {
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "abc"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 0},
                                          .end_pos = {.line = 1, .col = 3},
                                          .index = 0,
                                          .length = 3,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "\"def\"",
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 3},
                                          .end_pos = {.line = 1, .col = 8},
                                          .index = 3,
                                          .length = 5,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "ghi"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 8},
                                          .end_pos = {.line = 1, .col = 11},
                                          .index = 8,
                                          .length = 3,
                                      },
                              },
                              {.type = lexer::TokenType::Eof,
                               .data = ""sv,
                               .span =
                                   {
                                       .begin_pos = {.line = 1, .col = 11},
                                       .end_pos = {.line = 1, .col = 11},
                                       .index = 11,
                                       .length = 0,
                                   }},
                          }),

                SmallCase(". ."sv, "double_list_terminator.vtyp"sv,
                          {
                              {
                                  .type = lexer::TokenType::ListTerminator,
                                  .data = "."sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 0},
                                          .end_pos = {.line = 1, .col = 1},
                                          .index = 0,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::ListTerminator,
                                  .data = "."sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 2},
                                          .end_pos = {.line = 1, .col = 3},
                                          .index = 2,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Eof,
                                  .data = ""sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 3},
                                          .end_pos = {.line = 1, .col = 3},
                                          .index = 3,
                                          .length = 0,
                                      },
                              },
                          }),

                SmallCase("(a .)"sv, "terminator_then_paren.vtyp"sv,
                          {
                              {
                                  .type = lexer::TokenType::LParen,
                                  .data = "("sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 0},
                                          .end_pos = {.line = 1, .col = 1},
                                          .index = 0,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "a"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 1},
                                          .end_pos = {.line = 1, .col = 2},
                                          .index = 1,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::ListTerminator,
                                  .data = "."sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 3},
                                          .end_pos = {.line = 1, .col = 4},
                                          .index = 3,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::RParen,
                                  .data = ")"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 4},
                                          .end_pos = {.line = 1, .col = 5},
                                          .index = 4,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Eof,
                                  .data = ""sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 5},
                                          .end_pos = {.line = 1, .col = 5},
                                          .index = 5,
                                          .length = 0,
                                      },
                              },
                          }),

                SmallCase("(a . b)"sv, "correctly_used_list_terminator.vtyp"sv,
                          {
                              {
                                  .type = lexer::TokenType::LParen,
                                  .data = "("sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 0},
                                          .end_pos = {.line = 1, .col = 1},
                                          .index = 0,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "a"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 1},
                                          .end_pos = {.line = 1, .col = 2},
                                          .index = 1,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::ListTerminator,
                                  .data = "."sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 3},
                                          .end_pos = {.line = 1, .col = 4},
                                          .index = 3,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "b"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 5},
                                          .end_pos = {.line = 1, .col = 6},
                                          .index = 5,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::RParen,
                                  .data = ")"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 6},
                                          .end_pos = {.line = 1, .col = 7},
                                          .index = 6,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Eof,
                                  .data = ""sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 7},
                                          .end_pos = {.line = 1, .col = 7},
                                          .index = 7,
                                          .length = 0,
                                      },
                              },
                          }),

                SmallCase("(a .;comment"sv, "terminator_then_comment.vtyp",
                          {
                              {
                                  .type = lexer::TokenType::LParen,
                                  .data = "("sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 0},
                                          .end_pos = {.line = 1, .col = 1},
                                          .index = 0,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "a"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 1},
                                          .end_pos = {.line = 1, .col = 2},
                                          .index = 1,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::ListTerminator,
                                  .data = "."sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 3},
                                          .end_pos = {.line = 1, .col = 4},
                                          .index = 3,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Eof,
                                  .data = ""sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 12},
                                          .end_pos = {.line = 1, .col = 12},
                                          .index = 12,
                                          .length = 0,
                                      },
                              },
                          }),

                SmallCase("`a`\"b\""sv, "back_to_back_quotes.vtyp"sv,
                          Lexer{Settings{.quote_characters = {'"', '`'}}},
                          {
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "`a`"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 0},
                                          .end_pos = {.line = 1, .col = 3},
                                          .index = 0,
                                          .length = 3,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "\"b\""sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 3},
                                          .end_pos = {.line = 1, .col = 6},
                                          .index = 3,
                                          .length = 3,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Eof,
                                  .data = ""sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 6},
                                          .end_pos = {.line = 1, .col = 6},
                                          .index = 6,
                                          .length = 0,
                                      },
                              },
                          }),

                SmallCase("\"foo bar\""sv, "back_to_back_quotes.vtyp"sv,
                          Lexer{Settings{.quote_characters = {}}},
                          {{
                               .type = lexer::TokenType::Atom,
                               .data = "\"foo"sv,
                               .span =
                                   {
                                       .begin_pos = {.line = 1, .col = 0},
                                       .end_pos = {.line = 1, .col = 4},
                                       .index = 0,
                                       .length = 4,
                                   },
                           },
                           {
                               .type = lexer::TokenType::Atom,
                               .data = "bar\""sv,
                               .span =
                                   {
                                       .begin_pos = {.line = 1, .col = 5},
                                       .end_pos = {.line = 1, .col = 9},
                                       .index = 5,
                                       .length = 4,
                                   },
                           },
                           {.type = lexer::TokenType::Eof,
                            .data = ""sv,
                            .span =
                                {
                                    .begin_pos = {.line = 1, .col = 9},
                                    .end_pos = {.line = 1, .col = 9},
                                    .index = 9,
                                    .length = 0,
                                }}}),

                SmallCase("(\0"sv, "null_byte_at_eof.vtyp"sv,
                          {
                              {
                                  .type = lexer::TokenType::LParen,
                                  .data = "("sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 0},
                                          .end_pos = {.line = 1, .col = 1},
                                          .index = 0,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "\0"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 1},
                                          .end_pos = {.line = 1, .col = 2},
                                          .index = 1,
                                          .length = 1,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Eof,
                                  .data = ""sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 2},
                                          .end_pos = {.line = 1, .col = 2},
                                          .index = 2,
                                          .length = 0,
                                      },
                              },
                          }),

                SmallCase("a\0b\"a\0b\""sv,
                          "null_inside_atom_and_string.vtyp"sv,
                          {
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "a\0b"sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 0},
                                          .end_pos = {.line = 1, .col = 3},
                                          .index = 0,
                                          .length = 3,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Atom,
                                  .data = "\"a\0b\""sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 3},
                                          .end_pos = {.line = 1, .col = 8},
                                          .index = 3,
                                          .length = 5,
                                      },
                              },
                              {
                                  .type = lexer::TokenType::Eof,
                                  .data = ""sv,
                                  .span =
                                      {
                                          .begin_pos = {.line = 1, .col = 8},
                                          .end_pos = {.line = 1, .col = 8},
                                          .index = 8,
                                          .length = 0,
                                      },
                              },
                          }),

                SmallCase(
                    "\"ok\" \"bad"sv, "ok_then_error_token.vtyp"sv,
                    {
                        {
                            .type = lexer::TokenType::Atom,
                            .data = "\"ok\"",
                            .span =
                                {
                                    .begin_pos = {.line = 1, .col = 0},
                                    .end_pos = {.line = 1, .col = 4},
                                    .index = 0,
                                    .length = 4,
                                },
                        },
                        {
                            .type = lexer::TokenType::Error,
                            .data = lexer::ErrorData{"unterminated string"},
                            .span =
                                {
                                    .begin_pos = {.line = 1, .col = 5},
                                    .end_pos = {.line = 1, .col = 9},
                                    .index = 5,
                                    .length = 4,
                                },
                        },
                        {
                            .type = lexer::TokenType::Eof,
                            .data = ""sv,
                            .span =
                                {
                                    .begin_pos = {.line = 1, .col = 9},
                                    .end_pos = {.line = 1, .col = 9},
                                    .index = 9,
                                    .length = 0,

                                },
                        },
                    }));

        const auto lexer = test_case.lexer(test_case.file_name);
        [[maybe_unused]] uint64_t len{};
        for (auto [expected, actual] :
             std::views::zip(test_case.tokens, test_case.text | lexer)) {
            CHECK(expected == actual);
            len++;
        }
        CHECK(len == test_case.tokens.size());
    }

    SUBCASE("miscellaneous tests") {
        SUBCASE("increment past end")
        CHECK_THROWS_AS(
            [&] {
                auto token_stream = ""sv | lexer("empty.vtyp");
                auto iter = token_stream.begin();
                iter++;
                iter++;
                (void)*iter;
            }(),
            InternalCompilerError);
    }
}

// This test is based on extracting the first N digits of a base S number,
// where N and S are the length of the string and the length of the alphabet
// respectively, generating every string of N letters over an S-sized
// alphabet as base S digits.
//
// # EXAMPLE
// N = 4
// XS = ABCD (alphabet digits)
// X_1 = A (XS % S)
// XS_1 = BCD (alphabet digits) (XS / S)
// X_2 = B (XS_1 % S)
// ... Repeat for each digit
// YS = BBCD (XS + 1)
// ... Repeat for each letter
// ZS = <alphabet[0]>CCD (<alphabet[S-1]>BCD + 1)
// ... Repeat for each combination of letters
TEST_CASE("fuzz the lexer") {
    struct Category {
        std::string alphabet;
        uint8_t len;
    };

    Category full{generate_full_alphabet(), CS2LIB_FULL_FUZZ_DEPTH};
    Category curated{generate_curated_alphabet(), CS2LIB_CURATED_FUZZ_DEPTH};
    Category category = GENERATE(full, curated);

    [[maybe_unused]] size_t alpha_len = category.alphabet.length();
    cs2_lib::sexp::Lexer lexer{cs2_lib::sexp::Settings{}};

    {
        const size_t max_threads =
            std::max(std::thread::hardware_concurrency(), 1U);
        const size_t thread_count =
            std::min(size_t{CS2LIB_FUZZ_THREADS}, max_threads);
        const size_t max = cs2_lib::util::pow(alpha_len, category.len);

        std::atomic<size_t> cursor{0};
        constexpr size_t CHUNK = 1 << 16;

        struct FailureInfo {
            // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
            size_t first_index;

            // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
            size_t count{1};

            // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
            std::string data;

            constexpr FailureInfo(size_t first_index, std::string data)
                : first_index{first_index}, data{std::move(data)} {}
        };

        std::array<std::optional<std::jthread>, CS2LIB_FUZZ_THREADS> threads;
        for (size_t thread{}; thread < thread_count; thread++) {
            threads[thread].emplace([&] -> void { // NOLINT
                std::optional<FailureInfo> failure_info{};

                [[maybe_unused]] std::string buf(category.len, '\0');
                for (size_t base; (base = cursor.fetch_add(CHUNK)) < max;) {
                    for (size_t chunk_idx = base;
                         chunk_idx < std::min(base + CHUNK, max); ++chunk_idx) {
                        [[maybe_unused]] const uint64_t index = chunk_idx;

                        try {
                            uint64_t pos = index;
                            for (uint8_t buf_pos = 0; buf_pos < category.len;
                                 buf_pos++) {
                                buf[buf_pos] =
                                    category.alphabet[pos % alpha_len];
                                pos /= alpha_len;
                            }

                            for (auto tok : buf | lexer("test.sexp")) {
                                using cs2_lib::sexp::Span;
                                using cs2_lib::sexp::lexer::ErrorData;
                                using cs2_lib::sexp::lexer::TokenType;
                                const std::string_view src{buf};

                                // One cursor walks every byte of the input
                                // exactly once: first through the gap before
                                // each token, then through the token.
                                uint64_t cur_index = 0;
                                Span::Position cur_pos{.line = 1, .col = 0};
                                bool seen_eof = false;
                                const auto advance = [&](char chara) {
                                    if (chara == '\n') {
                                        cur_pos.line++;
                                        cur_pos.col = 0;
                                    } else {
                                        cur_pos.col++;
                                    }
                                    cur_index++;
                                };

                                for (const auto tok :
                                     src | lexer("test.sexp")) {
                                    const auto &pos = tok.span.get_pos_span();

                                    // Cheap integer checks first
                                    if (seen_eof) {
                                        throw std::string{"token after EOF"};
                                    }
                                    if (pos.index < cur_index) {
                                        throw std::format(
                                            "token at index {} overlaps the "
                                            "previous one (ends at {})",
                                            pos.index, cur_index);
                                    }
                                    if (pos.end_index() > src.size()) {
                                        throw std::format(
                                            "token ends at {}, past the "
                                            "input's end at {}",
                                            pos.end_index(), src.size());
                                    }

                                    // The gap may only contain whitespace and
                                    // ';' comments
                                    bool in_comment = false;
                                    while (cur_index < pos.index) {
                                        const char chara = src[cur_index];
                                        if (in_comment) {
                                            in_comment = chara != '\n';
                                        } else if (chara == ';') {
                                            in_comment = true;
                                        } else if (!std::isspace(
                                                       chara, std::locale::
                                                                  classic())) {
                                            throw std::format(
                                                "skipped byte {:#04x} at index "
                                                "{}",
                                                static_cast<unsigned char>(
                                                    chara),
                                                cur_index);
                                        }
                                        advance(chara);
                                    }

                                    if (pos.begin_pos != cur_pos) {
                                        throw std::format(
                                            "begin {}:{} but recounted {}:{}",
                                            pos.begin_pos.line,
                                            pos.begin_pos.col, cur_pos.line,
                                            cur_pos.col);
                                    }
                                    while (cur_index < pos.end_index()) {
                                        advance(src[cur_index]);
                                    }
                                    if (pos.end_pos != cur_pos) {
                                        throw std::format(
                                            "end {}:{} but recounted {}:{}",
                                            pos.end_pos.line, pos.end_pos.col,
                                            cur_pos.line, cur_pos.col);
                                    }

                                    // Per-type rules
                                    if (tok.type == TokenType::Eof) {
                                        seen_eof = true;
                                        if (pos.length != 0 ||
                                            pos.index != src.size()) {
                                            throw std::string{
                                                "EOF is not a zero-length "
                                                "token at the end"};
                                        }
                                    } else if (pos.length == 0) {
                                        throw std::string{
                                            "zero-length token that isn't EOF"};
                                    } else if (tok.type == TokenType::Error) {
                                        if (!std::holds_alternative<ErrorData>(
                                                tok.data)) {
                                            throw std::string{
                                                "error token without "
                                                "ErrorData"};
                                        }
                                    } else {
                                        // Data must be a view of exactly this
                                        // token's bytes: O(1)
                                        const auto *data =
                                            std::get_if<std::string_view>(
                                                &tok.data);
                                        if (data == nullptr ||
                                            data->data() !=
                                                src.data() + pos.index ||
                                            data->size() != pos.length) {
                                            throw std::string{
                                                "token data is not its slice "
                                                "of the input"};
                                        }
                                    }
                                }
                                if (!seen_eof) {
                                    throw std::string{
                                        "stream ended without an EOF token"};
                                }
                            }
                        } catch (
                            const cs2_lib::sexp::InternalCompilerError &ice) {
                            if (!failure_info.has_value()) {
                                failure_info.emplace(
                                    index, std::format("ICE: {}", ice.what()));
                            } else {
                                failure_info.value().count++;
                            }
                        } catch (const std::exception &ex) {
                            if (!failure_info.has_value()) {
                                failure_info.emplace(
                                    index,
                                    std::format("Exception: {}", ex.what()));
                            } else {
                                failure_info.value().count++;
                            }
                        } catch (const std::string &data) {
                            if (!failure_info.has_value()) {
                                failure_info.emplace(
                                    index, std::format("Test error: {}", data));
                            } else {
                                failure_info.value().count++;
                            }
                        } catch (...) {
                            if (!failure_info.has_value()) {
                                failure_info.emplace(index,
                                                     "Unknown exception");
                            } else {
                                failure_info.value().count++;
                            }
                        };
                    }
                }

                if (failure_info.has_value()) {
                    CHECK_MESSAGE(
                        !failure_info.has_value(),
                        std::format("Failure at index {} and {} others: {}",
                                    failure_info.value().first_index,
                                    failure_info.value().count - 1,
                                    failure_info.value().data));
                }
            });
        }
    }
}

TEST_CASE("other tests") {
    SUBCASE("GNU span formatting") {
        using namespace cs2_lib::sexp;

        struct GnuTest {
            // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
            std::string expected;

            // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
            Span span;

            ///
            /// Create a GNU test
            /// \param expected_fmt The format string with a template in place
            /// of the file name
            ///
            constexpr GnuTest(
                std::format_string<std::string_view &> expected_fmt,
                Span::PositionSpan pos, std::string_view file_name,
                std::string_view file_contents)
                : expected{std::format(expected_fmt, file_name)},
                  span{Span::Factory{file_name, file_contents}.make(pos)} {}
        };

        const GnuTest test =
            GENERATE(GnuTest("{}:1.1-1.7",
                             {.begin_pos = {1, 0},
                              .end_pos = {1, 7},
                              .index = 0,
                              .length = 7},
                             "basic_formatting.vtyp"sv, "tacocat"sv),
                     GnuTest("{}:1.1-1.4",
                             {.begin_pos = {1, 0},
                              .end_pos = {2, 1},
                              .index = 0,
                              .length = 4},
                             "newline_ending.vtyp"sv, "No.\n"sv),
                     GnuTest("{}:1.1",
                             {.begin_pos = {1, 0},
                              .end_pos = {1, 0},
                              .index = 0,
                              .length = 0},
                             "empty_file.vtyp"sv, ""sv));

        CHECK(test.expected == std::format("{:gnu}", test.span));
    }
}
