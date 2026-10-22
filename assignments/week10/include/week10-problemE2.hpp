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
#ifndef HUTZDOG_CS2_ASSIGN_WEEK10_PROBLEME2
#define HUTZDOG_CS2_ASSIGN_WEEK10_PROBLEME2

#include <array>
#include <cstddef>
#include <expected>
#include <string_view>
#include <variant>

#include "include/week10-problemE1.hpp"

namespace hutzdog_cs2_week10 {

using namespace std::string_view_literals;

enum class DelimiterType : std::size_t {
    PAREN,
    SQUARE,
    CURLY,

    COUNT,
};

[[nodiscard]] constexpr auto to_index(DelimiterType delim) -> std::size_t {
    return static_cast<std::size_t>(delim);
}

constexpr std::array<std::string_view, to_index(DelimiterType::COUNT)>
    DELIM_NAMES = {
        "parentheses"sv,
        "square brackets"sv,
        "curly brackets"sv,
};

[[nodiscard]] constexpr auto delim_to_string(DelimiterType type)
    -> std::string_view {
    return DELIM_NAMES[to_index(type)];
}

struct Delimiter {
    const DelimiterType type; // NOLINT
    char open;                // NOLINT
    char close;               // NOLINT

    // NOLINTNEXTLINE
    constexpr Delimiter(DelimiterType type, char open, char close)
        : type{type}, open{open}, close{close} {}

    static const std::array<Delimiter, to_index(DelimiterType::COUNT)>
        DELIMITERS;
};
constexpr std::array Delimiter::DELIMITERS{
    Delimiter{DelimiterType::PAREN, '(', ')'},
    Delimiter{DelimiterType::SQUARE, '[', ']'},
    Delimiter{DelimiterType::CURLY, '{', '}'},
};
static_assert([] -> bool {
    for (std::size_t idx = 0; idx < to_index(DelimiterType::COUNT); idx++) {
        if (idx != to_index(Delimiter::DELIMITERS[idx].type)) {
            return false;
        }
    }

    return true;
}());

struct UnmatchedDelimiter {
    std::optional<DelimiterType> expected_type;
    DelimiterType actual_type;
};

constexpr auto pair_delimiters(std::string_view text)
    -> std::expected<std::monostate, UnmatchedDelimiter> {
    Stack<std::vector<DelimiterType>> delimiters;

    for (char chara : text) {
        for (Delimiter delim : Delimiter::DELIMITERS) {
            if (chara == delim.open) {
                delimiters.push_back(delim.type);
                continue;
            }
            if (chara == delim.close) {
                if (!delimiters.peek_back() ||
                    delimiters.peek_back() != delim.type) {
                    return std::unexpected(UnmatchedDelimiter{
                        .expected_type = delimiters.peek_back(),
                        .actual_type = delim.type,
                    });
                }
                delimiters.push_back(delim.type);
                continue;
            }
        }
    }

    return {};
}
} // namespace hutzdog_cs2_week10

#endif // HUTZDOG_CS2_ASSIGN_WEEK10_PROBLEME2
