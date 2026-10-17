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
// NOTE: I am using parentheses in some places to initialize. This gets around
//       the messy handling of std::initializer_list with brackets.

#ifndef HUTZDOG_CS2_LIB_LIST
#define HUTZDOG_CS2_LIB_LIST

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <format>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <ranges>
#include <sstream>
#include <type_traits>
#include <utility>

#include "cs2lib/util.hpp"

namespace cs2_lib::list::_impl {
///
/// A single node in the linked list
///
template <class T> struct Node {
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    T element;

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::unique_ptr<Node> next{nullptr};

    template <class... Args>
    constexpr explicit Node(Args &&...args)
        requires std::constructible_from<T, Args...>
        : element(std::forward<Args>(args)...) {}

    constexpr Node(T element_, std::unique_ptr<Node> &&next_)
        : element(std::move(element_)), next{std::move(next_)} {}
};

} // namespace cs2_lib::list::_impl

namespace cs2_lib::list {

template <class T> class List;

///
/// An iterator over a list of nodes
///
template <class T, bool Const = true> class Iterator {
  public:
    using iterator_concept = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;

    constexpr Iterator() = default;
    constexpr Iterator(const Iterator<T, !Const> &other)
        requires Const
        : node(other.node) {}

    constexpr auto operator++() -> Iterator & {
        if (this->node == nullptr) {
            throw std::out_of_range("Tried to increment past the end of list");
        }

        this->node = this->node->next.get();
        return *this;
    }

    constexpr auto operator++(int) -> Iterator {
        const Iterator old = *this;
        ++*this;
        return old;
    }

    constexpr auto operator==(const Iterator &iter) const -> bool = default;

    constexpr auto
    operator==([[maybe_unused]] std::default_sentinel_t sentinel) const
        -> bool {
        return this->node == nullptr;
    }

    constexpr auto operator*() const
        -> std::conditional_t<Const, const T &, T &> {
        if (this->node == nullptr) {
            throw std::logic_error(
                "Attempted to dereference past the end of the stream");
        }
        return this->node->element;
    }

  private:
    _impl::Node<T> *node{nullptr};

    constexpr explicit Iterator(_impl::Node<T> *node) : node{node} {}

    friend class Iterator<T, true>;
    friend class List<T>;
};
static_assert(std::forward_iterator<Iterator<int>>);
static_assert(std::forward_iterator<Iterator<int, false>>);
static_assert(std::sentinel_for<std::default_sentinel_t, Iterator<int>>);
static_assert(std::convertible_to<Iterator<int, false>, Iterator<int>>);
static_assert(!std::convertible_to<Iterator<int>, Iterator<int, false>>);

///
/// A singly linked list of elements
///
template <class T> class List {
  public:
    constexpr List() = default;

    constexpr List(std::initializer_list<T> data)
        requires std::copy_constructible<T>
        : List() {
        std::ranges::for_each(
            data, [this](const auto &item) -> void { this->push_front(item); });
        this->reverse();
    }

    constexpr List(std::size_t size, T data)
        requires std::copy_constructible<T>
        : List() {
        for (std::size_t idx = 0; idx < size; ++idx) {
            this->head =
                std::make_unique<_impl::Node<T>>(data, std::move(this->head));
        }
    }

    template <std::ranges::input_range R>
        requires std::constructible_from<T,
                                         std::ranges::range_reference_t<R>> &&
                 std::move_constructible<T>
    constexpr List([[maybe_unused]] std::from_range_t from_range, R &&range)
        : List() {
        for (auto &&item : range) {
            this->emplace_front(std::forward<decltype(item)>(item));
        }
        this->reverse();
    }

    constexpr List(List &&other) noexcept = default;

    constexpr auto operator=(List &&other) noexcept -> List & {
        this->head.swap(other.head);
        return *this;
    }

    constexpr List(const List &other)
        requires std::copy_constructible<T>
        : List() {
        std::unique_ptr<_impl::Node<T>> *current = &this->head;

        for (const _impl::Node<T> *src = other.head.get(); src != nullptr;
             src = src->next.get()) {
            *current = std::make_unique<_impl::Node<T>>(src->element);
            current = &(*current)->next;
        }
    }

    constexpr auto operator=(const List &other) -> List &
        requires std::copy_constructible<T>
    {
        List tmp(other);
        this->head.swap(tmp.head);
        return *this;
    }

    constexpr ~List() {
        while (this->head) {
            std::unique_ptr next = std::move(this->head->next);
            this->head = std::move(next);
        }
    }

    [[nodiscard]] constexpr auto operator==(const List &other) const -> bool
        requires std::equality_comparable<T>
    {
        _impl::Node<T> *current_ptr = this->head.get();
        _impl::Node<T> *other_ptr = other.head.get();

        while (current_ptr != nullptr && other_ptr != nullptr) {
            if (current_ptr->element != other_ptr->element) {
                return false;
            }

            current_ptr = current_ptr->next.get();
            other_ptr = other_ptr->next.get();
        }

        return current_ptr == nullptr && other_ptr == nullptr;
    }

    ///
    /// Construct a value in place at the front
    ///
    template <class... Args>
    constexpr void emplace_front(Args &&...args)
        requires std::constructible_from<T, Args...>
    {
        std::unique_ptr node =
            std::make_unique<_impl::Node<T>>(std::forward<Args>(args)...);
        node->next = std::move(this->head);
        this->head = std::move(node);
    }

    ///
    /// Push a value onto the front of the list
    ///
    constexpr void push_front(T value) {
        this->head = std::make_unique<_impl::Node<T>>(std::move(value),
                                                      std::move(this->head));
    }

    ///
    /// Get the head element of the list
    ///
    constexpr auto car() -> std::optional<std::reference_wrapper<T>> {
        if (this->head == nullptr) {
            return std::nullopt;
        }
        return this->head->element;
    }

    ///
    /// Get the head element of the list
    ///
    [[nodiscard]] constexpr auto car() const
        -> std::optional<std::reference_wrapper<const T>> {
        if (this->head == nullptr) {
            return std::nullopt;
        }
        return this->head->element;
    }

    ///
    /// Get the tail elements of the list
    ///
    [[nodiscard]] constexpr auto cdr() -> std::ranges::view auto {
        auto iter = this->begin();
        if (iter == this->end()) {
            return std::ranges::subrange(this->begin(), this->end());
        }
        return std::ranges::subrange(std::next(this->begin()), this->end());
    }

    ///
    /// Get the tail elements of the list
    ///
    [[nodiscard]] constexpr auto cdr() const -> std::ranges::view auto {
        auto iter = this->begin();
        if (iter == this->end()) {
            return std::ranges::subrange(this->begin(), this->end());
        }
        return std::ranges::subrange(std::next(this->begin()), this->end());
    }

    ///
    /// O(n) index operation, this is the best you can get on a linked list
    ///
    template <class Self>
    [[nodiscard]] constexpr auto operator[](this Self &self, std::size_t idx)
        -> std::conditional_t<std::is_const_v<Self>, const T &, T &> {
        if (!std::in_range<std::ptrdiff_t>(idx)) {
            throw std::out_of_range(
                "Linked list index past the size of a ptrdiff");
        }
        const Iterator<T, std::is_const_v<Self>> result = std::ranges::next(
            self.begin(), static_cast<std::ptrdiff_t>(idx), self.end());
        if (result == self.end()) {
            throw std::out_of_range(
                std::format("Index out of bounds: {}", idx));
        }
        return *result;
    }

    ///
    /// Helper to get the Nth cdr of a list
    ///
    template <class Self>
    [[nodiscard]] constexpr auto cndr(this Self &self, std::size_t idx)
        -> std::ranges::view auto {
        if (!std::in_range<std::ptrdiff_t>(idx)) {
            throw std::out_of_range(
                "Linked list cndr past the size of a ptrdiff");
        }
        return self | std::views::drop(static_cast<std::ptrdiff_t>(idx));
    }

    ///
    /// Pop a value from the front of the list
    ///
    [[nodiscard]] constexpr auto pop_front() -> std::optional<T> {
        if (this->head == nullptr) {
            return std::nullopt;
        }

        T value = std::move(this->head->element);
        std::unique_ptr next{std::move(this->head->next)};
        this->head = std::move(next);
        return value;
    }

    ///
    /// Get the length of this list
    ///
    [[nodiscard]] constexpr auto length() const -> std::size_t {
        return std::ranges::distance(this->begin(), this->end());
    }

    ///
    /// Check if this list is empty
    ///
    [[nodiscard]] [[gnu::always_inline]] constexpr auto empty() const -> bool {
        return this->head == nullptr;
    }

    ///
    /// Reverse the list in place
    ///
    constexpr void reverse() {
        std::unique_ptr<_impl::Node<T>> item = std::move(this->head);
        this->head = nullptr;
        while (item != nullptr) {
            std::unique_ptr old_head = std::move(this->head);
            std::unique_ptr next = std::move(item->next);
            this->head = std::move(item);
            this->head->next = std::move(old_head);
            item = std::move(next);
        }
    }

    ///
    /// Start of a range of the elements in this list
    ///
    template <class Self>
    constexpr auto begin(this Self &self)
        -> Iterator<T, std::is_const_v<Self>> {
        return Iterator<T, std::is_const_v<Self>>(self.head.get());
    }

    ///
    /// End of a range of the elements in this list
    ///
    [[nodiscard]] constexpr auto end() const -> std::default_sentinel_t {
        return std::default_sentinel;
    }

  private:
    std::unique_ptr<_impl::Node<T>> head{nullptr};

    friend class Iterator<T>;
};
static_assert(std::ranges::viewable_range<List<int>>);
static_assert(std::is_nothrow_move_constructible_v<List<int>>);
static_assert(std::is_nothrow_move_assignable_v<List<int>>);

template <std::ranges::input_range R>
List(std::from_range_t, R &&) -> List<std::ranges::range_value_t<R>>;

} // namespace cs2_lib::list

template <cs2_lib::util::Printable T>
struct std::formatter<cs2_lib::list::List<T>>
    : public std::formatter<std::string> {
    template <class FormatCtx>
    auto format(const cs2_lib::list::List<T> &list, FormatCtx &ctx) const {
        return std::formatter<std::string>::format(
            std::ranges::fold_left(
                list, "(List",
                [](std::string acc, const T &element) -> std::string {
                    if constexpr (std::formattable<T, char>) {
                        std::format_to(std::back_inserter(acc), " {}", element);
                    } else {
                        std::ostringstream oss{};
                        oss << element;
                        std::format_to(std::back_inserter(acc), " {}",
                                       oss.str());
                    }
                    return acc;
                }) +
                ")",
            ctx);
    }
};

namespace cs2_lib::list {
template <cs2_lib::util::Printable T>
constexpr auto operator<<(std::ostream &stream,
                          const cs2_lib::list::List<T> &list)
    -> std::ostream & {
    stream << std::format("{}", list);
    return stream;
}
} // namespace cs2_lib::list

#endif // HUTZDOG_CS2_LIB_LIST
