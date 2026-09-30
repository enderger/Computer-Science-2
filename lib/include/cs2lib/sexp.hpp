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

class Span;

namespace _impl {
inline auto format_ice(std::optional<cs2_lib::sexp::Span> span,
                       std::string_view message) -> std::string;

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
    std::locale locale{std::locale::classic()};

    ///
    /// The quote character(s), used in cases where quotes are inconvenient
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::unordered_set<char> quote_characters{'"'};
};

///
/// A span in the source code
///
class Span {
  public:
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

    ///
    /// A single-character source code position
    ///
    struct Position {
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        uint64_t line{1}, col{0};

        constexpr auto operator==(const Position &other) const
            -> bool = default;
    };

    ///
    /// A span between positions
    ///
    struct PositionSpan {
        ///
        /// The begin position of the span in the file
        ///
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        Position begin_pos;

        ///
        /// The end position of the span in the file
        ///
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        Position end_pos;

        ///
        /// The index of the beginning of the token
        ///
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        uint64_t index;

        ///
        /// The index of the end of the token
        ///
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        uint64_t length;

        ///
        /// Get the end index of this position
        ///
        [[nodiscard]] constexpr auto end_index() const -> uint64_t {
            return this->index + this->length;
        }

        [[nodiscard]] constexpr auto operator==(const PositionSpan &other) const
            -> bool = default;

        ///
        /// Move to the end of this position
        ///
        [[nodiscard]] constexpr auto chop() && -> PositionSpan {
            this->begin_pos = this->end_pos;
            this->index = this->index + this->length;
            this->length = 0;
            return *this;
        }
    };

    ///
    /// A factory for creating spans. This exists to separate out the file data
    /// from the position data, even if in practice it requires two invocations
    /// and doesn't get used in favor of `chop`
    ///
    class Factory {
      public:
        ///
        /// Construct a factory for producing spans
        ///
        [[nodiscard]] constexpr Factory(
            // This turns off a warning about easily confused names. It isn't
            // founded NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
            std::string_view name, std::string_view data)
            : file_name{name}, file_data{data} {}

        ///
        /// Construct a new span
        ///
        [[nodiscard]] constexpr auto make(PositionSpan pos) const -> Span {
            return Span{this->file_name, this->file_data, pos};
        }

        ///
        /// Construct a new span
        ///
        [[nodiscard]] constexpr auto make(Position begin_pos, Position end_pos,
                                          uint64_t index, uint64_t length) const
            -> Span {
            return this->make({
                .begin_pos = begin_pos,
                .end_pos = end_pos,
                .index = index,
                .length = length,
            });
        }

        ///
        /// Construct a new invalid span
        ///
        [[nodiscard]] constexpr auto make_invalid() const -> Span {
            return this->make(Position{.line = 1, .col = 0},
                              Position{.line = 1, .col = 0}, 0, 0);
        }

        ///
        /// Get the file name associated with this factory
        ///
        [[nodiscard]] constexpr auto get_file_name() const -> std::string_view {
            return this->file_name;
        }

      private:
        ///
        /// The name of the source code file
        ///
        std::string_view file_name;

        ///
        /// A view into the whole file
        ///
        std::string_view file_data;
    };

    constexpr auto operator==(const Span &other) const -> bool {
        return this->position_span == other.position_span &&
               this->file_name == other.file_name;
    }

    ///
    /// Add a character to this span
    ///
    /// \param data the character to append
    ///
    constexpr void operator+=(unsigned char data) {
        if (data == '\n') {
            this->position_span.end_pos.line += 1;
            this->position_span.end_pos.col = 0;
        } else {
            this->position_span.end_pos.col++;
        }

        this->position_span.length++;
    }

    ///
    /// Append a fixed difference to the span
    ///
    /// \param diff The fixed difference
    ///
    constexpr void operator+=(Diff diff) {
        this->position_span.end_pos.line += diff.lines;
        this->position_span.end_pos.col =
            diff.columns +
            (diff.lines == 0 ? this->position_span.end_pos.col : 0);
        this->position_span.length += diff.characters;
    }

    ///
    /// End the current span, returning a new span. This takes an rvalue to the
    /// original span. This was done to make the "new span" part explicit.
    ///
    /// \return The moved span, to be reassigned.
    ///
    [[nodiscard]] constexpr auto chop() && -> Span {
        this->position_span = std::move(this->position_span).chop();
        return *this;
    }

    ///
    /// Get the position data of this span (constant)
    ///
    [[nodiscard]] constexpr auto get_pos_span() const -> const PositionSpan & {
        return this->position_span;
    }

    ///
    /// Get the current file name
    ///
    [[nodiscard]] constexpr auto get_file_name() const
        -> const std::string_view & {
        return this->file_name;
    }

  private:
    ///
    /// The name of the source code file
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::string_view file_name;

    ///
    /// A view into the whole file
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::string_view file_data;

    ///
    /// The position of the span in the file
    ///
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    PositionSpan position_span;

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    Span(std::string_view file_name, std::string_view file_data,
         PositionSpan pos)
        : file_name{file_name}, file_data{file_data}, position_span{pos} {}

    friend class std::formatter<Span>;
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
          cpptrace::raw_trace trace = cpptrace::generate_raw_trace()
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
    cpptrace::raw_trace strace;
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
        : _impl::ICEBase(_impl::format_ice(span, std::move(message))) {}
#ifdef CS2LIB_DEBUG
    InternalCompilerError(std::optional<Span> span,
                          cpptrace::raw_trace &&strace, std::string &&message)
        : _impl::ICEBase(_impl::format_ice(span, std::move(message)),
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

    constexpr Token(TokenType type, Span span,
                    std::variant<std::string_view, ErrorData> &&data)
        : type{type}, span{span}, data{std::move(data)} {}

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
            : data{view->inner}, settings{view->settings},
              span{Span::Factory{view->file_name, view->inner}.make_invalid()} {
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
                this->current = std::nullopt;
                return;
            }

            this->current = this->lex();
        }

      private:
        std::string_view data;
        Settings settings;
        Span span;
        std::optional<Token> current;
        bool has_emitted_eof{false};

        constexpr auto is_atom_terminator(char data) -> bool {
            return data == '(' || data == ')' || data == ';' ||
                   this->settings.quote_characters.contains(data) ||
                   std::isspace(data, this->settings.locale);
        }

        constexpr auto get_slice() {
            const auto &pos_span = this->span.get_pos_span();
            return this->data.substr(pos_span.index, pos_span.length);
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
            const auto &pos_span = this->span.get_pos_span();
            while (this->data.length() > pos_span.index &&
                   (this->data[pos_span.index] == ';' ||
                    std::isspace(this->data[pos_span.index],
                                 this->settings.locale))) {
                const bool is_comment = this->data.length() > pos_span.index &&
                                        this->data[pos_span.index] == ';';
                while (is_comment &&
                       pos_span.end_index() < this->data.length() &&
                       this->data[pos_span.end_index()] != '\n') {
                    this->span += this->data[pos_span.end_index()];
                }

                while (pos_span.end_index() < this->data.length() &&
                       std::isspace(this->data[pos_span.end_index()],
                                    settings.locale)) {
                    this->span += this->data[pos_span.end_index()];
                }

                this->span = std::move(this->span).chop();
            }
        }

        [[nodiscard]] constexpr auto lex_quoted() -> Token {
            const auto &pos_span = this->span.get_pos_span();
            const char quote = this->data[pos_span.index];
            // NOTE: This has to be imperative, as an arbitrary sized window
            //       cannot capture a n+1 sized escape sequence chain. So,
            //       we do this the boring way.
            bool escape{false};
            bool complete{false};
            for (char chara : this->data.substr(pos_span.index + 1)) {
                this->span += chara;
                if (escape) {
                    escape = false;
                    continue;
                }

                if (chara == '\\') {
                    escape = true;
                } else if (chara == quote) {
                    complete = true;
                    break;
                }
            }

            if (!complete) {
                return Token{TokenType::Error, this->span,
                             ErrorData{"unterminated string"}};
            }
            return Token{TokenType::Atom, this->span, this->get_slice()};
        }

        [[nodiscard]] constexpr auto lex_atom() -> Token {
            const auto &pos_span = this->span.get_pos_span();
            // NOTE: This is safe because substr(length) is defined as the
            //       empty string.
            const auto tail = this->data.substr(pos_span.index + 1);
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
            const auto &pos_span = this->span.get_pos_span();
            std::optional<Token> ret;

            this->drop_whitespace();
            if (pos_span.index >= this->data.length()) {
                return std::nullopt;
            }

            // NOTE: This is a custom implementation of `+=` that takes into
            //       account newlines and the span's debug information
            this->span += this->data[pos_span.index];

            const char cur = this->data[pos_span.index];
            if (this->settings.quote_characters.contains(cur)) {
                ret = this->lex_quoted();
            } else if (cur == '.' &&
                       (this->data.length() <= pos_span.index + 1 ||
                        Iterator::is_atom_terminator(
                            this->data[pos_span.index + 1]))) {
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
            return ret;
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

template <> class std::formatter<cs2_lib::sexp::Span> {
  public:
    enum class ParseMode : uint8_t {
        Default,
        Gnu
    } parse_mode{ParseMode::Default}; // NOLINT

    constexpr auto parse(std::format_parse_context &ctx) {
        if (std::string_view{ctx}.starts_with("gnu")) {
            this->parse_mode = ParseMode::Gnu;
            ctx.advance_to(ctx.begin() + 3);
        }
        return this->underlying.parse(ctx);
    }

    template <class FormatCtx>
    constexpr auto format(cs2_lib::sexp::Span span, FormatCtx &ctx) const {
        using Span = cs2_lib::sexp::Span;
        // NOLINTNEXTLINE(readability-identifier-length)
        const Span::PositionSpan &ps = span.get_pos_span();

        switch (this->parse_mode) {
        case ParseMode::Default:
            return this->underlying.format(
                std::format("(Span \"{}\" (begin {} {}) (end {} {}) (index {}) "
                            "(length {}))",
                            span.file_name, ps.begin_pos.line, ps.begin_pos.col,
                            ps.end_pos.line, ps.end_pos.col, ps.index,
                            ps.length),
                ctx);
        case ParseMode::Gnu:
            if (ps.begin_pos == ps.end_pos) {
                return this->underlying.format(
                    std::format("{}:{}.{}", span.file_name, ps.begin_pos.line,
                                ps.begin_pos.col + 1),
                    ctx);
            }

            // TODO: Count characters of the underlying stream
            const Span::Position end_pos =
                span.file_data[ps.end_index() - 1] == '\n'
                    ? Span::Position{.line = ps.end_pos.line - 1,
                                     .col =
                                         std::formatter<cs2_lib::sexp::Span>::
                                             get_newline_column(span.file_data,
                                                                ps.end_index())}
                    : Span::Position{.line = ps.end_pos.line,
                                     .col = ps.end_pos.col};
            return this->underlying.format(
                std::format("{}:{}.{}-{}.{}", span.file_name, ps.begin_pos.line,
                            ps.begin_pos.col + 1, end_pos.line, end_pos.col),
                ctx);
        }
        std::unreachable();
    }

  private:
    std::formatter<std::string> underlying;

    static constexpr auto get_newline_column(std::string_view src, size_t idx)
        -> size_t {
        if (idx == 0 || idx - 1 > src.size() || src[idx - 1] != '\n') {
            throw std::logic_error("Tried to find end index of newline without "
                                   "the previous character being a newline");
        }

        const std::string_view view = src.substr(0, idx - 1);
        return 1 + view.length() -
               std::ranges::distance(
                   std::ranges::find(view | std::views::reverse, '\n'),
                   view.rend());
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
            std::format(
                "{}: error: ({}) {}\n{}", err.span, err.subsystem(),
                err.error_message,
                cs2_lib::sexp::_impl::strace_fmt.format(err.strace.resolve())),
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
            type_name = "error"sv;
            break;
        case cs2_lib::sexp::lexer::TokenType::Eof:
            type_name = "EOF"sv;
            break;
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
            std::format("(Error \"{}\")", err.message), ctx);
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
                    return data.empty() ? ":empty" : std::format("'{}", data);
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
            std::format("(Token {} {} {})", tok.span, tok.type, formatted_data),
            ctx);
    }
};

constexpr auto operator<<(std::ostream &ost,
                          const cs2_lib::sexp::lexer::Token &tok)
    -> std::ostream & {
    return ost << std::format("{}", tok);
}

namespace cs2_lib::sexp::_impl {
auto format_ice(std::optional<cs2_lib::sexp::Span> span,
                std::string_view message) -> std::string {
    std::string span_text =
        span.has_value() ? std::format("{}: ", span.value()) : "";
    return std::format("{}error: Internal Compiler Error: {}", span_text,
                       message);
}
} // namespace cs2_lib::sexp::_impl
#endif // HUTZDOG_CS2_LIB_SEXP
