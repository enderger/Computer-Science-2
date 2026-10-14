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
#ifndef HUTZDOG_CS2_ASSIGN_WEEK9_PROBLEME1
#define HUTZDOG_CS2_ASSIGN_WEEK9_PROBLEME1

#include <concepts>
#include <format>
#include <memory>
#include <optional>

namespace hutzdog_cs2_week9 {
template <std::move_constructible T> class Queue {
  public:
    constexpr Queue() = default;

    constexpr void enqueue(T &&value) {
        if (this->is_empty()) {
            this->head = std::make_unique<Node>(std::forward<T>(value));
            this->tail = this->head.get();
            return;
        }

        this->tail->next = std::make_unique<Node>(std::forward<T>(value));
        this->tail = this->tail->next.get();
    }
    constexpr void enqueue(const T &value)
        requires std::copy_constructible<T>
    {
        this->enqueue(std::move(T{value}));
    }

    template <class... Args> constexpr void emplace(Args &&...values) {
        if (this->is_empty()) {
            this->head = std::make_unique<Node>(
                std::move(T(std::forward<Args>(values)...)));
            this->tail = this->head.get();
            return;
        }

        this->tail->next =
            std::make_unique<Node>(std::move(T(std::forward<Args>(values)...)));
        this->tail = this->tail->next.get();
    }

    constexpr auto dequeue() -> std::optional<T> {
        if (this->is_empty()) {
            return std::nullopt;
        }

        T data = std::move(this->head->data);
        this->head = std::move(this->head->next);

        if (this->is_empty()) {
            this->tail = nullptr;
        }

        return data;
    }

    [[nodiscard]] constexpr auto is_empty() const -> bool {
        return this->head == nullptr;
    }

  private:
    struct Node {
        T data;                              // NOLINT
        std::unique_ptr<Node> next{nullptr}; // NOLINT

        constexpr Node(T &&data) { this->data = data; }
    };
    std::unique_ptr<Node> head{nullptr};
    Node *tail = head.get();

    friend class std::formatter<Queue>;
};
} // namespace hutzdog_cs2_week9

template <class T>
    requires std::formattable<T, char>
struct std::formatter<hutzdog_cs2_week9::Queue<T>>
    : std::formatter<std::string> {
    template <class FormatCtx>
    auto format(const hutzdog_cs2_week9::Queue<T> &queue,
                FormatCtx &ctx) const {
        std::string res{"(List"};
        for (auto ptr = queue.head.get(); ptr != nullptr;
             ptr = ptr->next.get()) {
            std::format_to(res, " `{}`", ptr->data);
        }
        res += ")";

        return std::formatter<std::string>::format(res, ctx);
    }
};

#endif // HUTZDOG_CS2_ASSIGN_WEEK9_PROBLEME1
