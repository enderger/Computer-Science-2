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

#ifndef HUTZDOG_CS2_ASSIGN_WEEK9_PROBLEME2
#define HUTZDOG_CS2_ASSIGN_WEEK9_PROBLEME2

#include <concepts>
#include <format>
#include <memory>

namespace hutzdog_cs2_week9 {

template <class T> struct Node {
    T data;                              // NOLINT
    std::unique_ptr<Node> next{nullptr}; // NOLINT

    constexpr Node(T data, std::unique_ptr<Node> next)
        : data{std::move(data)}, next{std::move(next)} {}
};

template <std::move_constructible T> class Stack {
  public:
    constexpr void push(T value) {
        this->head =
            std::make_unique<Node<T>>(std::move(value), std::move(this->head));
    }

    template <class... Args> constexpr void emplace(Args &&...args) {
        this->head = std::make_unique<Node<T>>(T(std::forward<Args>(args)...),
                                               std::move(this->head));
    }

    [[nodiscard]] constexpr auto is_empty() const -> bool {
        return this->head == nullptr;
    }

    [[nodiscard]] constexpr auto pop() -> std::optional<T> {
        if (this->is_empty()) {
            return std::nullopt;
        }

        std::unique_ptr<Node<T>> next = std::move(this->head->next);
        T value = std::move(this->head->data);
        this->head = std::move(next);
        return value;
    }

  private:
    std::unique_ptr<Node<T>> head{nullptr};

    friend class std::formatter<Stack, char>;
};

} // namespace hutzdog_cs2_week9

template <class T>
    requires std::formattable<T, char>
struct std::formatter<hutzdog_cs2_week9::Stack<T>>
    : std::formatter<std::string> {
    template <class FormatCtx>
    auto format(const hutzdog_cs2_week9::Stack<T> &queue,
                FormatCtx &ctx) const {
        std::string res{"(Stack"};
        for (auto ptr = queue.head.get(); ptr != nullptr;
             ptr = ptr->next.get()) {
            std::format_to(std::back_inserter(res), " `{}`", ptr->data);
        }
        res += ")";

        return std::formatter<std::string>::format(res, ctx);
    }
};

#endif // HUTZDOG_CS2_ASSIGN_WEEK9_PROBLEME2
