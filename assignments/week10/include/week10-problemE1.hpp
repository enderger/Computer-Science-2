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
#include <concepts>
#include <functional>
#include <optional>

#ifndef HUTZDOG_CS2_ASSIGN_WEEK10_PROBLEME1
#define HUTZDOG_CS2_ASSIGN_WEEK10_PROBLEME1

namespace hutzdog_cs2_week10 {

template <class C, class T = C::value_type>
concept IStack = requires(C &container, T data) {
    { container.push_back(data) } -> std::same_as<void>;
    { container.pop_back() } -> std::same_as<void>;
    { container.back() } -> std::same_as<T &>;
    { container.empty() } -> std::same_as<bool>;
} && std::move_constructible<T>;

template <class C, class T = C::value_type>
concept IEmplaceBack = requires(C &container, T data) {
    { container.emplace_back(data) } -> std::same_as<T &>;
};

template <class C, class T = C::value_type>
    requires IStack<C, T>
class Stack {
  public:
    constexpr Stack()
        requires std::constructible_from<C>
        : inner{C()} {}

    constexpr Stack(C &&inner)
        requires std::move_constructible<C>
        : inner{std::forward<C>(inner)} {}

    template <class... Args>
    constexpr void emplace_back(Args &&...args)
        requires IEmplaceBack<C, T>
    {
        this->inner.emplace_back(std::forward<Args>(args)...);
    }

    constexpr void push_back(T data) { this->inner.push_back(std::move(data)); }

    constexpr auto pop_back() -> std::optional<T> {
        if (this->inner.empty()) {
            return std::nullopt;
        }

        std::optional<T> back{std::move(this->inner.back())};
        this->inner.pop_back();
        return back;
    }

    constexpr auto peek_back() -> std::optional<std::reference_wrapper<T>> {
        if (this->inner.empty()) {
            return std::nullopt;
        }
        return this->inner.back();
    }

    constexpr auto empty() -> bool { return this->inner.empty(); }

  private:
    C inner;
};

} // namespace hutzdog_cs2_week10

#endif // HUTZDOG_CS2_ASSIGN_WEEK10_PROBLEME1
