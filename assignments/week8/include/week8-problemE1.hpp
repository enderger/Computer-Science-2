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
#include <algorithm>
#include <cstddef>

#ifndef HUTZDOG_CS2_ASSIGN_WEEK8_PROBLEME1
#define HUTZDOG_CS2_ASSIGN_WEEK8_PROBLEME1

namespace hutzdog_cs2_week8 {
class IntArray {
  public:
    constexpr IntArray(std::size_t size) : data{new int[size]}, size_{size} {}
    constexpr ~IntArray() { delete[] this->data; }

    constexpr IntArray(const IntArray &original)
        : data{new int[original.size()]}, size_{original.size()} {
        std::copy(original.data, original.data + original.size(), this->data);
    }

    constexpr auto operator=(const IntArray &original) -> IntArray & {
        this->size_ = original.size();

        if (this == &original) {
            return *this;
        }
        delete[] this->data;
        this->data = new int[this->size()];

        std::copy(original.data, original.data + original.size(), this->data);

        return *this;
    }

    auto operator[](std::size_t index) -> int &;

    void display() const;

    [[nodiscard]] constexpr auto size() const -> std::size_t {
        return this->size_;
    }

  private:
    int *data;
    std::size_t size_;
};
} // namespace hutzdog_cs2_week8

#endif // HUTZDOG_CS2_ASSIGN_WEEK8_PROBLEME1
