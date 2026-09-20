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
#include <exception>
#include <expected>
#include <format>
#include <iterator>
#include <locale>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_set>
#include <utility>
#include <variant>

// HACK: Clang doesn't implement stack traces, so I'm pulling this in instead
// in debug builds
#ifdef CS2LIB_DEBUG

#include <cpptrace/basic.hpp>
#include <cpptrace/cpptrace.hpp>
#include <cpptrace/formatting.hpp>

#endif // CS2LIB_DEBUG

namespace cs2_lib::sexp {
struct Span;
} // namespace cs2_lib::sexp

namespace cs2_lib::sexp {
using namespace std::string_view_literals;

namespace _impl {
inline auto format_ice(cs2_lib::sexp::Span span, std::string_view message)
    -> std::string;

#ifdef CS2LIB_DEBUG
const inline cpptrace::formatter strace_fmt =
    cpptrace::formatter{}.header("Stack trace:").snippets(true);

#endif
} // namespace _impl

///
/// Settings for the lexer and parser
///
struct Settings {
    ///
    /// The locale data for the application
    ///
    std::locale locale{std::locale::classic()};

    ///
    /// The quote character(s), used in cases where quotes are inconvenient
    ///
    std::unordered_set<char> quote_characters{'"'};
};

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
    /// The position of the span in the file
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    uint64_t begin_line, begin_column, end_line, end_column, index, length;

    ///
    /// The difference between two spans
    ///
    struct Diff {
        ///
        /// The line, column, and character difference between the position of
        /// this span and another Columns specifically add from the last
        /// newline, should one be present. This means that columns is the
        /// actual column number instead of an offset should the line offset
        /// not be 0.
        ///
        uint64_t lines, columns, characters;
    };

    constexpr auto operator==(const Span &other) const -> bool = default;

    ///
    /// Add a character to this span
    ///
    /// \param data the character to append
    ///
    constexpr void operator+=(unsigned char data) {
        if (data == '\n') {
            this->end_line++;
            this->end_column = 0;
        } else {
            this->end_column++;
        }

        this->length++;
    }

    ///
    /// Append a fixed difference to the span
    ///
    /// \param diff The fixed difference
    ///
    constexpr void operator+=(Diff diff) {
        this->end_line += diff.lines;
        this->end_column =
            diff.columns + (diff.lines == 0 ? this->end_column : 0);
        this->length += diff.characters;
    }

    ///
    /// End the current span, returning a new span. This takes an rvalue to the
    /// original span. This was done to make the "new span" part explicit.
    ///
    /// \return The moved span, to be reassigned.
    ///
    constexpr auto chop() && -> Span {
        this->begin_line = this->end_line;
        this->begin_column = this->end_column;
        this->index = this->index + this->length;
        this->length = 0;

        return *this;
    }

    ///
    /// Get the invalid span
    ///
    constexpr static auto invalid() -> Span {
        return Span{
            .file_name = "<UNKNOWN>",
            .begin_line = 1,
            .begin_column = 0,
            .end_line = 1,
            .end_column = 0,
            .index = 0,
            .length = 0,
        };
    }
};

///
/// The shared error type for my S-expression parser
///
class Error {
  public:
    // HACK: The preprocessor shenanigans get me compiler independent
    // stacktraces
    //       in debug builds while still having pure dependency-free releases
    Error(Span span, std::string message
#ifdef CS2LIB_DEBUG
          ,
          cpptrace::stacktrace trace = cpptrace::generate_trace()
#endif // CS2LIB_DEBUG
              )
        : span{span}, error_message{std::move(message)}
#ifdef CS2LIB_DEBUG
          ,
          strace{std::move(trace)}
#endif // CS2LIB_DEBUG
    {
    }

    // VIRTUAL METHODS
    ///
    /// Which subsystem the error is coming from
    ///
    [[nodiscard]] virtual auto subsystem() const -> std::string_view = 0;

    // CONSTRUCTORS / DESTRUCTORS
    virtual ~Error() = default;

    // ACCESSORS / MUTATORS
    [[nodiscard]] constexpr auto get_span() const -> const Span & {
        return this->span;
    }

    [[nodiscard]] constexpr auto get_message() const -> std::string_view {
        return this->error_message;
    }

    // OPERATORS
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

#ifdef CS2LIB_DEBUG
    ///
    /// The compiler stacktrace (or empty on release builds)
    ///
    cpptrace::stacktrace strace;
#endif // CS2LIB_DEBUG

    friend class std::formatter<Error>;
};

#ifdef CS2LIB_DEBUG

///
/// An internal compiler error, stacktrace included in a debug build
///
class InternalCompilerError : public cpptrace::exception_with_message {
  public:
    InternalCompilerError(std::string_view message,
                          std::optional<Span> span = std::nullopt) noexcept
        : cpptrace::exception_with_message(
              _impl::format_ice(span.value_or(Span::invalid()), message)) {}
};

#else

///
/// An internal compiler error, compile in debug mode to get a
/// compiler stacktrace.
///
class InternalCompilerError : public std::exception {
  public:
    InternalCompilerError(std::string_view message,
                          std::optional<Span> span = std::nullopt)
        : message{_impl::format_ice(span.value_or(Span::invalid()), message)} {}

    constexpr auto what() const noexcept -> const char * {
        return this->message.data();
    }

  private:
    std::string message;
};

#endif // CS2LIB_DEBUG

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
    /// The list termination operator
    ///
    ListTerminator,

    ///
    /// The end of file token
    ///
    Eof,

    ///
    /// An error in the token stream
    ///
    Error,

    ///
    /// An internal compiler error, this is used for errors that MUST be encoded
    /// in the token stream before sending
    ///
    InternalCompilerError
};

///
/// A singular token in the lexer
///
struct Token {
    ///
    /// The token type itself
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
    /// This is the (owned) error message for error tokens (including ICE)
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::variant<std::string_view, std::string> data;

    ///
    /// Construct an owning token from a moved string
    ///
    constexpr Token(TokenType type, Span span, std::string &&data)
        : type{type}, span{span}, data{std::move(data)} {}

    ///
    /// Construct a non-owning token
    ///
    constexpr Token(TokenType type, Span span, std::string_view data)
        : type{type}, span{span}, data{data} {}

    ///
    /// Create an uninitialized token at a given span, these are a form of ICE
    ///
    constexpr Token(Span span)
        : type{TokenType::InternalCompilerError}, span{span},
          data{"Uninitialized token"sv} {}

    constexpr Token(Token &&other) noexcept
        : type{other.type}, span{other.span}, data{std::move(other.data)} {}

    constexpr Token(const Token &other) noexcept = default;

    constexpr auto operator=(Token &&other) noexcept -> Token & {
        this->type = other.type;
        this->span = other.span;
        this->data = std::move(other.data);

        return *this;
    }

    constexpr auto operator=(const Token &other) -> Token & = default;

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
    LexerError(const Token &tok) : Error(tok.span, tok.get_data_owned()) {}

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
/// A view into a stream of tokens
///
template <std::ranges::contiguous_range R>
    requires std::ranges::borrowed_range<R> &&
             std::same_as<std::ranges::range_value_t<R>, char>
class TokenStream : public std::ranges::view_interface<TokenStream<R>> {
  public:
    // INNER TYPES
    class Iterator {
      public:
        using iterator_concept = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::expected<Token, LexerError>;

        constexpr Iterator() = default;
        constexpr explicit Iterator(const TokenStream<R> *view)
            : data{view->inner}, settings{view->settings} {
            this->span.file_name = view->file_name;
            this->current = this->lex();
        }

        constexpr auto operator*() const -> std::expected<Token, LexerError> {
            return this->current
                .transform(
                    [](const Token &tok) -> std::expected<Token, LexerError> {
                        if (tok.type == TokenType::Error) {
                            return std::expected<Token, LexerError>{
                                std::unexpect, tok};
                        }
                        if (tok.type == TokenType::InternalCompilerError) {
                            throw InternalCompilerError{tok.get_data_owned(),
                                                        tok.span};
                        }

                        return std::expected<Token, LexerError>{tok};
                    })
                .or_else([this]() -> std::optional<
                                      std::expected<Token, LexerError>> {
                    if (this->has_emitted_eof) {
                        throw InternalCompilerError{
                            "ICE: dereferenced past the end of the token stream"sv,
                            this->span,
                        };
                    }

                    return std::make_optional(std::expected<Token, LexerError>{
                        Token{TokenType::Eof, this->span, ""sv}});
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
        std::string_view data;
        Span span = Span::invalid();
        Settings settings;
        std::optional<Token> current;
        bool has_emitted_eof{false};

        constexpr auto is_atom_terminator(char data) -> bool {
            return data == '(' || data == ')' ||
                   this->settings.quote_characters.contains(data) ||
                   std::isspace(data, this->settings.locale);
        }

        constexpr auto get_slice() {
            return this->data.substr(this->span.index, this->span.length);
        }

        [[nodiscard]] constexpr auto lex() -> std::optional<Token> {
            Token ret{this->span};

            while (
                this->data.length() > this->span.index + this->span.length &&
                std::isspace(this->data[this->span.index + this->span.length],
                             this->settings.locale)) {
                this->span += this->data[this->span.index];
            }
            this->span = std::move(this->span).chop();

            if (this->span.index >= this->data.length()) {
                return std::nullopt;
            }

            // NOTE: This is a custom implementation of `==` that takes into
            //       account newlines and the span's debug information
            this->span += this->data[this->span.index];

            const char cur = this->data[this->span.index];
            if (this->settings.quote_characters.contains(cur)) {
                // NOTE: This has to be imperative, as an arbitrary sized window
                //       cannot capture a n+1 sized escape sequence chain. So,
                //       we do this the boring way.
                bool escape = false;
                bool complete = false;
                for (char chara : this->data.substr(this->span.index + 1)) {
                    this->span += chara;
                    if (escape) {
                        escape = false;
                    } else if (chara == '\\') {
                        escape = true;
                    } else if (chara == cur) {
                        complete = true;
                        break;
                    }
                }

                if (!complete) {
                    ret = Token{TokenType::Error, this->span,
                                "Unterminated string slice"sv};
                } else {
                    ret = Token{TokenType::Atom, this->span, this->get_slice()};
                }
            } else if (cur == '.' &&
                       (this->data.length() <= this->span.index + 1 ||
                        std::isspace(this->data[this->span.index + 1],
                                     this->settings.locale))) {
                ret = Token{TokenType::ListTerminator, this->span,
                            this->get_slice()};
            } else if (cur == '(') {
                ret = Token{TokenType::LParen, this->span, this->get_slice()};
            } else if (cur == ')') {
                ret = Token{TokenType::RParen, this->span, this->get_slice()};
            }

            this->span = std::move(this->span).chop();
            return {std::move(ret)};
        }
    };
    static_assert(std::input_iterator<Iterator>);

    // CONSTRUCTORS
    constexpr TokenStream() = default;

    constexpr TokenStream(R &&inner, Settings settings,
                          std::string_view file_name)
        : settings{std::move(settings)},
          inner{std::ranges::views::all(std::forward<R>(inner))},
          file_name{file_name} {}

    // ITERATORS
    [[nodiscard]] auto begin() const -> Iterator { return Iterator{this}; }

    [[nodiscard]] auto end() const -> std::default_sentinel_t { return {}; }

  private:
    Settings settings;
    std::string_view inner;
    std::string_view file_name;
};
static_assert(std::ranges::view<TokenStream<std::string_view>>);

///
/// A lexer for S-expressions
///
class Lexer {
  public:
    // CONSTRUCTORS
    Lexer(Settings settings) : settings{std::move(settings)} {}

    // INNER CLASSES
    class Closure : public std::ranges::range_adaptor_closure<Closure> {
      public:
        // CONSTRUCTORS
        Closure(Settings settings, std::string_view file_name)
            : settings{std::move(settings)}, file_name{file_name} {}

        // OPERATORS
        template <std::ranges::contiguous_range R>
            requires std::ranges::borrowed_range<R> &&
                     std::same_as<std::ranges::range_value_t<R>, char>
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
                                         Lexer{Settings{}}("f.sexp"sv))>);

} // namespace lexer
using lexer::Lexer;

} // namespace cs2_lib::sexp

template <>
class std::formatter<cs2_lib::sexp::Span> : public std::formatter<std::string> {
  public:
    template <class FormatCtx>
    constexpr auto format(cs2_lib::sexp::Span span, FormatCtx &ctx) const {
        return std::formatter<std::string>::format(
            std::format("{}:{}:{}-{}:{}", span.file_name, span.begin_line,
                        span.begin_column, span.end_line, span.end_column),
            ctx);
    }
};

template <>
class std::formatter<cs2_lib::sexp::Error>
    : public std::formatter<std::string> {
  public:
    template <class FormatCtx>
    constexpr auto format(const cs2_lib::sexp::Error &err,
                          FormatCtx &ctx) const {
#ifdef CS2LIB_DEBUG
        return std::formatter<std::string>::format(
            std::format("{}: error: ({}) {}\n{}", err.span, err.subsystem(),
                        err.error_message,
                        cs2_lib::sexp::_impl::strace_fmt.format(err.strace)),
            ctx);
#else
        return std::formatter<std::string>::format(
            std::format("{}: error: ({}) {}", err.span, err.subsystem(),
                        err.error_message),
            ctx);

#endif // CS2LIB_DEBUG
    }
};

namespace cs2_lib::sexp::_impl {
auto format_ice(cs2_lib::sexp::Span span, std::string_view message)
    -> std::string {
    return std::format("{}: error: Internal Compiler Error: {}", span, message);
}
} // namespace cs2_lib::sexp::_impl
#endif // HUTZDOG_CS2_LIB_SEXP
