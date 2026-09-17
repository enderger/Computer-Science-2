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

#ifndef HUTZDOG_CS2_LIB_SEXP
#define HUTZDOG_CS2_LIB_SEXP

#include <cstdint>
#include <expected>
#include <format>
#include <iterator>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <variant>

namespace cs2_lib::sexp {
using namespace std::string_view_literals;

///
/// A span in the source code
///
struct Span {
    ///
    /// The name of the source code file
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::string_view file_name;

    ///
    /// The position of the lexer in the file
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    uint64_t begin_line, begin_column, end_line, end_column, index, length;

    constexpr auto operator==(const Span &other) const -> bool = default;
};

///
/// The shared error type for my S-expression parser
///
class Error {
  public:
    Error(Span span, std::string message)
        : span{span}, error_message{std::move(message)} {}

    ///
    /// Which subsystem the error is coming from
    ///
    [[nodiscard]] virtual auto subsystem() const -> std::string_view = 0;

    virtual ~Error() = default;

    [[nodiscard]] constexpr auto get_span() const -> const Span & {
        return this->span;
    }

    [[nodiscard]] constexpr auto get_message() const -> std::string_view {
        return this->error_message;
    }

    auto operator==(const Error &other) const -> bool {
        if (typeid(*this) != typeid(other)) {
            return false;
        }

        return this->equals(other);
    }

  protected:
    ///
    /// Protected comparison operator.
    /// `other` will always have a value of the casted type
    ///
    [[nodiscard]] virtual auto equals(const Error &other) const -> bool = 0;

  private:
    ///
    /// Where the error occurs
    ///
    Span span;

    ///
    /// The error message to send from this file
    ///
    std::string error_message;

    friend class std::formatter<Error>;
};

namespace lexer {
///
/// A type of token in the stream
///
enum class TokenType : uint8_t {
    ///
    /// The opening delimiter of the language
    ///
    LParen,

    ///
    /// The closing delimiter of the language
    ///
    RParen,

    ///
    /// A piece of (untyped) data
    ///
    Atom,

    ///
    /// The end of file token
    ///
    Eof,

    ///
    /// An error in the token stream
    ///
    Error
};

///
/// A singular token in the lexer
///
struct Token {
    ///
    /// The token type of the lexer
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    TokenType type;
    ///
    /// The data at the token's location
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    Span span;

    ///
    /// A view into the data of a token
    /// This is the (owned) error message for error tokens
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::variant<std::string_view, std::string> data;

    constexpr auto operator==(const Token &other) const -> bool = default;

    [[nodiscard]] constexpr auto get_data_owned() const -> std::string {
        if (const std::string_view *data =
                std::get_if<std::string_view>(&this->data)) {
            return std::string(*data);
        }
        if (const std::string *data = std::get_if<std::string>(&this->data)) {
            return {*data};
        }
        std::unreachable();
    }
    [[nodiscard]] constexpr auto get_data_borrowed() const -> std::string_view {
        if (const std::string_view *data =
                std::get_if<std::string_view>(&this->data)) {
            return *data;
        }
        if (const std::string *data = std::get_if<std::string>(&this->data)) {
            return *data;
        }
        std::unreachable();
    }
};

///
/// The shared error type for the lexer
///
struct LexerError : public Error {
    constexpr LexerError(Token &&tok) : Error(tok.span, tok.get_data_owned()) {}

    [[nodiscard]] constexpr auto subsystem() const
        -> std::string_view override {
        return "lexer"sv;
    }

  protected:
    [[nodiscard]] auto equals(const Error &other) const -> bool override {
        const auto otherLE = static_cast<const LexerError &>(other);
        return otherLE.get_span() == this->get_span() &&
               otherLE.get_message() == this->get_message();
    }
};

///
/// Settings for the lexer
///
struct Settings {};

///
/// A view into a stream of tokens
///
template <std::ranges::contiguous_range R>
class TokenStream : public std::ranges::view_interface<TokenStream<R>> {
  public:
    // INNER TYPES
    class Iterator {
      public:
        using iterator_concept = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::expected<Token, LexerError>;

        constexpr Iterator() = default;
        constexpr explicit Iterator(TokenStream<R> *inner) : inner{inner} {
            this->span.file_name = inner->file_name;
            this->current = this->lex();
        }

        constexpr auto operator*() const -> std::expected<Token, LexerError> {
            return this->current
                .transform(
                    [](const Token &tok) -> std::expected<Token, LexerError> {
                        if (tok.type == TokenType::Error) {
                            return std::expected<Token, LexerError>(
                                std::unexpect, Token(tok));
                        }

                        return {Token(tok)};
                    })
                .or_else([this]() -> std::optional<
                                      std::expected<Token, LexerError>> {
                    if (this->has_emitted_eof) {
                        throw std::logic_error(
                            "dereferenced past the end of the token stream");
                    }

                    return std::make_optional(
                        std::expected<Token, LexerError>(Token{
                            .type = TokenType::Eof,
                            .span = this->span,
                            .data = std::variant<std::string_view, std::string>(
                                ""sv),
                        }));
                })
                .value();
        }

        constexpr auto operator==(
            [[gnu::unused]] const std::default_sentinel_t &sentinel) const
            -> bool {
            return this->has_emitted_eof;
        }

        constexpr auto operator==(const Iterator &) const -> bool = default;

        constexpr auto operator++() -> Iterator & {
            this->operator++(0);
            return *this;
        }

        constexpr void operator++([[gnu::unused]] int) {
            if (!this->current.has_value() ||
                this->current->type == TokenType::Eof) {
                this->has_emitted_eof = true;
                this->current = {};
                return;
            }

            this->current = this->lex();
        }

      private:
        TokenStream<R> *inner = nullptr;
        Span span{
            .file_name = "<UNKNOWN>",
            .begin_line = 1,
            .begin_column = 0,
            .end_line = 1,
            .end_column = 0,
            .index = 0,
            .length = 0,
        };
        std::optional<Token> current;
        bool has_emitted_eof{false};

        constexpr auto lex() -> std::optional<Token> { return {}; }
    };
    static_assert(std::input_iterator<Iterator>);

    // CONSTRUCTORS
    constexpr TokenStream() = default;

    constexpr TokenStream(R &&inner, Settings settings,
                          std::string_view file_name)
        : settings{settings},
          inner{std::ranges::views::all(std::forward<R>(inner))},
          file_name{file_name} {}

    // ITERATORS
    [[nodiscard]] auto begin() -> Iterator { return Iterator{this}; }

    [[nodiscard]] auto end() const -> std::default_sentinel_t { return {}; }

  private:
    Settings settings;
    std::ranges::views::all_t<R> inner;
    std::string_view file_name;
};
template <> class TokenStream<std::string> {
  public:
    constexpr TokenStream() = delete;
    constexpr TokenStream(std::string &&inner) = delete;
};
static_assert(std::ranges::view<TokenStream<std::string_view>>);

///
/// A lexer for S-expressions
///
class Lexer {
  public:
    // CONSTRUCTORS
    Lexer(Settings settings) : settings{settings} {}

    // INNER CLASSES
    class Closure : public std::ranges::range_adaptor_closure<Closure> {
      public:
        // CONSTRUCTORS
        Closure(Settings settings, std::string_view file_name)
            : settings{settings}, file_name{file_name} {}

        // OPERATORS
        template <std::ranges::viewable_range R>
        constexpr auto operator()(R &&range) const noexcept -> std::ranges::view
            auto {
            return TokenStream<R>{std::forward<R>(range), this->settings,
                                  this->file_name};
        }

      private:
        // PRIVATE MEMBERS
        Settings settings;
        std::string_view file_name;
    };

    // OPERATORS
    constexpr auto operator()(std::string_view file_name) const noexcept
        -> Closure {
        return {this->settings, file_name};
    }

  private:
    Settings settings;
};
static_assert(std::ranges::view<decltype(std::string_view{} |
                                         Lexer{Settings{}}("f.sexp"))>);

} // namespace lexer
using lexer::Lexer;

} // namespace cs2_lib::sexp

template <>
class std::formatter<cs2_lib::sexp::Error>
    : public std::formatter<std::string> {
  public:
    template <class FormatCtx>
    constexpr auto format(const cs2_lib::sexp::Error &err,
                          FormatCtx &ctx) const {
        return std::formatter<std::string>::format(
            std::format("{}:{}:{}-{}:{}: error: ({}) {}", err.span.file_name,
                        err.span.begin_line, err.span.begin_column,
                        err.span.end_line, err.span.end_column, err.subsystem(),
                        err.error_message),
            ctx);
    }
};

#endif // HUTZDOG_CS2_LIB_SEXP
