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
#include <format>
#include <string>
#include <string_view>

#include <doctest/doctest.h>

#include "include/week9-problemE1.hpp"

using namespace std::string_view_literals;

// IMPLEMENTATION

// TESTS
TEST_CASE("testing the Queue class") {
    struct Point { int x, y; Point() = default; Point(int x, int y) : x{x}, y{y} {}; auto operator==(const Point &other) const -> bool = default; }; // NOLINT
    hutzdog_cs2_week9::Queue<Point> queue{};
    queue.enqueue(Point{1, 1});
    queue.emplace(2, 2);

    CHECK(queue.dequeue() == Point{1, 1});
    CHECK(queue.dequeue() == Point{2, 2});
}
