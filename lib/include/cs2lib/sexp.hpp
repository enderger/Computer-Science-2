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
#include <cstdlib>
#include <exception>
#include <format>
#include <iterator>
#include <locale>
#include <memory>
#include <optional>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
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

// HACK: This is needed to demangle certain names for better error reporting
#if defined(__GNUC__) || defined(__clang__)
#if __has_include(<cxxabi.h>)

#include <cxxabi.h>
#define CS2LIB_ITANIUM_ABI

#endif // __has_include(<cxxabi.h>)
#endif // defined(__GNUC__) || defined(__clang__)

namespace cs2_lib::sexp {
using namespace std::string_view_literals;

struct Span;

namespace _impl {
inline auto format_ice(cs2_lib::sexp::Span span, std::string_view message)
    -> std::string;

#ifdef CS2LIB_DEBUG
const inline cpptrace::formatter strace_fmt =
    cpptrace::formatter{}.header("Stack trace:").snippets(true);
#endif

[[nodiscard]] inline auto ex_typename_inner(const std::type_info &info)
    -> std::string {
#ifdef CS2LIB_ITANIUM_ABI
    int status = 0;
    std::unique_ptr<char, void (*)(void *)> owned{
        abi::__cxa_demangle(info.name(), nullptr, nullptr, &status), std::free};
    return (status == 0 && owned) ? std::string{owned.get()}
                                  : std::string{info.name()};
#else
    return std::string{info.name()};
#endif // CS2LIB_ITANIUM_ABI
}

[[nodiscard]] inline auto ex_typename(const std::exception &exc)
    -> std::string {
    return ex_typename_inner(typeid(exc));
}

[[nodiscard]] inline auto current_ex_typename() -> std::optional<std::string> {
#ifdef CS2LIB_ITANIUM_ABI
    const std::type_info *info = abi::__cxa_current_exception_type();
    if (info == nullptr) {
        return std::nullopt;
    }
    return ex_typename_inner(*info);
#else
    return std::nullopt;
#endif // CS2LIB_ITANIUM_ABI
}

} // namespace _impl

///
/// Settings for the lexer and parser
///
struct Settings {
    ///
    /// The locale data for the application
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::locale locale;

    ///
    /// The quote character(s), used in cases where quotes are inconvenient
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::unordered_set<char> quote_characters;

    Settings(const std::locale &locale = std::locale::classic(),
             std::unordered_set<char> quote_characters = {'"'})
        : locale{locale}, quote_characters{std::move(quote_characters)} {}
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
        this->index = this->end_index();
        this->length = 0;

        return *this;
    }

    ///
    /// Get the index of the end of this span
    ///
    [[nodiscard]] constexpr auto end_index() const -> uint64_t {
        return this->index + this->length;
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

namespace _impl {
#ifdef CS2LIB_DEBUG

using ICEBase = cpptrace::exception_with_message;

#else

class ICEBase : public std::exception {
  public:
    explicit ICEBase(std::string &&message) noexcept
        : message{std::move(message)} {}
    [[nodiscard]] auto what() const noexcept -> const char * override {
        return this->message.c_str();
    }

  private:
    std::string message;
};

#endif // CS2LIB_DEBUG
} // namespace _impl

///
/// An internal compiler error, stacktrace included in a debug build
///
class InternalCompilerError : public _impl::ICEBase {
  public:
    InternalCompilerError(std::optional<Span> span, std::string &&message)
        : _impl::ICEBase(_impl::format_ice(span.value_or(Span::invalid()),
                                           std::move(message))) {}
#ifdef CS2LIB_DEBUG
    InternalCompilerError(std::optional<Span> span,
                          cpptrace::raw_trace &&strace, std::string &&message)
        : _impl::ICEBase(_impl::format_ice(span.value_or(Span::invalid()),
                                           std::move(message)),
                         std::move(strace)) {}
#endif

    InternalCompilerError(std::string &&message)
        : InternalCompilerError(std::nullopt, std::move(message)) {}

    template <class T, class... Args>
    InternalCompilerError(std::format_string<T, Args...> fmt, T &&arg1,
                          Args &&...args)
        : InternalCompilerError{std::nullopt,
                                std::format(fmt, std::forward<T>(arg1),
                                            std::forward<Args>(args)...)} {}

    template <class T, class... Args>
    InternalCompilerError(Span span, std::format_string<T, Args...> fmt,
                          T &&arg1, Args &&...args)
        : InternalCompilerError{std::make_optional(span),
                                std::format(fmt, std::forward<T>(arg1),
                                            std::forward<Args>(args)...)} {}
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
};

///
/// Special data implementation for the error tokens
///
struct ErrorData {
    ///
    /// The error message
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::string message;

#ifdef CS2LIB_DEBUG
    ///
    /// The span of the error
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    cpptrace::raw_trace trace;
#endif

    ErrorData(std::string message)
        : message{std::move(message)}
#ifdef CS2LIB_DEBUG
          ,
          trace{cpptrace::generate_raw_trace()}
#endif
    {
    }

    constexpr auto operator==(const ErrorData &other) const -> bool {
        return this->message == other.message;
    }
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
    /// This is either a view into the raw text of the token or error data
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::variant<std::string_view, ErrorData> data;

    constexpr Token(TokenType type, Span span, std::string_view data)
        : type{type}, span{span}, data{data} {}

    constexpr Token(TokenType type, Span span, ErrorData &&error_data)
        : type{type}, span{span}, data{std::move(error_data)} {}

    constexpr Token(Token &&other) noexcept
        : type{other.type}, span{other.span}, data{std::move(other.data)} {}

    constexpr Token(const Token &other) = default;

    constexpr auto operator=(Token &&other) noexcept -> Token & {
        this->type = other.type;
        this->span = other.span;
        this->data = std::move(other.data);

        return *this;
    }

    constexpr auto operator=(const Token &other) -> Token & = default;

    constexpr auto operator==(const Token &other) const -> bool = default;

  private:
    friend class std::formatter<Token>;
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
        using value_type = Token;

        constexpr Iterator() = default;
        constexpr explicit Iterator(const TokenStream<R> *view)
            : data{view->inner}, settings{view->settings} {
            this->span.file_name = view->file_name;
            this->current = this->lex();
        }

        constexpr auto operator*() const -> Token {
            return this->current
                .or_else([this]() -> std::optional<Token> {
                    if (this->has_emitted_eof) {
                        throw InternalCompilerError{
                            this->span,
                            "dereferenced past the end of the token stream",
                        };
                    }

                    return std::make_optional(
                        Token{TokenType::Eof, this->span, ""sv});
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
            return data == '(' || data == ')' || data == ';' ||
                   this->settings.quote_characters.contains(data) ||
                   std::isspace(data, this->settings.locale);
        }

        constexpr auto get_slice() {
            return this->data.substr(this->span.index, this->span.length);
        }

        [[nodiscard]] auto lex() -> std::optional<Token> {
            try {
                return this->lex_inner();
            } catch (const InternalCompilerError &) {
                throw;
            } catch (const std::bad_alloc &) {
                throw;
            } catch (const std::exception &e) {
                throw InternalCompilerError{
                    this->span,
                    std::format(
                        "unhandled exception of type {} in the lexer: {}",
                        _impl::ex_typename(e), e.what()),
                };
            } catch (...) {
                std::optional<std::string> current_ex_ty =
                    _impl::current_ex_typename();
                if (current_ex_ty.has_value()) {
                    throw InternalCompilerError{
                        this->span,
                        "unknown thrown non-exception in the lexer of type {}",
                        current_ex_ty.value()};
                }
                throw InternalCompilerError{this->span,
                                            "unknown thrown non-exception in "
                                            "the lexer of unknown type"};
            }
        }

        constexpr void drop_whitespace() {
            while (this->data.length() > this->span.index &&
                   (this->data[this->span.index] == ';' ||
                    std::isspace(this->data[this->span.index],
                                 this->settings.locale))) {
                const bool is_comment =
                    this->data.length() > this->span.index &&
                    this->data[this->span.index] == ';';
                while (is_comment &&
                       this->span.end_index() < this->data.length() &&
                       this->data[this->span.end_index()] != '\n') {
                    this->span += this->data[this->span.end_index()];
                }

                while (this->span.end_index() < this->data.length() &&
                       std::isspace(this->data[this->span.end_index()],
                                    this->settings.locale)) {
                    this->span += this->data[this->span.end_index()];
                }

                this->span = std::move(this->span).chop();
            }
        }

        [[nodiscard]] constexpr auto lex_quoted() -> Token {
            const char quote = this->data[this->span.index];
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
                } else if (chara == quote) {
                    complete = true;
                    break;
                }
            }

            if (!complete) {
                return Token{TokenType::Error, this->span,
                             ErrorData{"unterminated string slice"}};
            }
            return Token{TokenType::Atom, this->span, this->get_slice()};
        }

        [[nodiscard]] constexpr auto lex_atom() -> Token {
            // NOTE: This is safe because substr(length) is defined as the
            //       empty string.
            const auto tail = this->data.substr(this->span.index + 1);
            const auto rest = std::ranges::distance(
                tail | std::views::take_while([this](char data) -> bool {
                    return !this->is_atom_terminator(data);
                }));

            this->span += Span::Diff{
                .lines = 0,
                .columns = static_cast<uint64_t>(rest),
                .characters = static_cast<uint64_t>(rest),
            };

            return Token{TokenType::Atom, this->span, this->get_slice()};
        }

        [[nodiscard]] constexpr auto lex_inner() -> std::optional<Token> {
            std::optional<Token> ret;

            this->drop_whitespace();
            if (this->span.index >= this->data.length()) {
                return std::nullopt;
            }

            // NOTE: This is a custom implementation of `==` that takes into
            //       account newlines and the span's debug information
            this->span += this->data[this->span.index];

            const char cur = this->data[this->span.index];
            if (this->settings.quote_characters.contains(cur)) {
                ret = this->lex_quoted();
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
            } else {
                ret = this->lex_atom();
            }

            this->span = std::move(this->span).chop();
            return {ret};
        }
    };
    static_assert(std::input_iterator<Iterator>);

    // CONSTRUCTORS
    constexpr TokenStream() = default;

    constexpr TokenStream(R &&inner, Settings settings,
                          std::string_view file_name)
        : settings{std::move(settings)},
          inner{std::views::all(std::forward<R>(inner))}, file_name{file_name} {
    }

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

template <>
class std::formatter<cs2_lib::sexp::lexer::TokenType>
    : public std::formatter<std::string> {
  public:
    template <class FormatCtx>
    constexpr auto format(const cs2_lib::sexp::lexer::TokenType &typ,
                          FormatCtx &ctx) const {
        std::string_view type_name;
        switch (typ) {
        case cs2_lib::sexp::lexer::TokenType::LParen:
            type_name = "left-parentheses"sv;
            break;
        case cs2_lib::sexp::lexer::TokenType::RParen:
            type_name = "right-parentheses"sv;
            break;
        case cs2_lib::sexp::lexer::TokenType::ListTerminator:
            type_name = "list-terminator"sv;
            break;
        case cs2_lib::sexp::lexer::TokenType::Atom:
            type_name = "atom"sv;
            break;
        case cs2_lib::sexp::lexer::TokenType::Error:
            type_name = "error";
            break;
        case cs2_lib::sexp::lexer::TokenType::Eof:
            type_name = "EOF";
            break;
        default:
            throw cs2_lib::sexp::InternalCompilerError(
                "No string representation for token type ID {}, add it to "
                "std::formatter<TokenType>",
                (uint16_t)typ);
        }

        return std::formatter<std::string>::format(
            std::format(":{}", type_name), ctx);
    }
};

template <>
class std::formatter<cs2_lib::sexp::lexer::ErrorData>
    : public std::formatter<std::string> {
  public:
    template <class FormatCtx>
    constexpr auto format(const cs2_lib::sexp::lexer::ErrorData &err,
                          FormatCtx &ctx) const {
        return std::formatter<std::string>::format(
            std::format("(Error {})", err.message), ctx);
    }
};

template <>
class std::formatter<cs2_lib::sexp::lexer::Token>
    : public std::formatter<std::string> {
  public:
    template <class FormatCtx>
    constexpr auto format(const cs2_lib::sexp::lexer::Token &tok,
                          FormatCtx &ctx) const {
        std::string formatted_data = std::visit(
            [](const auto &data) -> std::string {
                using T = std::decay_t<decltype(data)>;
                if constexpr (std::is_same_v<T, std::string_view>) {
                    return std::string(data);
                } else if constexpr (std::is_same_v<
                                         T, cs2_lib::sexp::lexer::ErrorData>) {
                    return std::format("{}", data);
                } else {
                    static_assert(false,
                                  "New type added to the Token data variant "
                                  "needs to be added to the formatter.");
                }
            },
            tok.data);
        return std::formatter<std::string>::format(
            std::format("(Token \"{}\" {} '{})", tok.span, tok.type,
                        formatted_data),
            ctx);
    }
};

constexpr auto operator<<(std::ostream &ost,
                          const cs2_lib::sexp::lexer::Token &tok)
    -> std::ostream & {
    return ost << std::format("{}", tok);
}

namespace cs2_lib::sexp::_impl {
auto format_ice(cs2_lib::sexp::Span span, std::string_view message)
    -> std::string {
    return std::format("{}: error: Internal Compiler Error: {}", span, message);
}
} // namespace cs2_lib::sexp::_impl
#endif // HUTZDOG_CS2_LIB_SEXP
