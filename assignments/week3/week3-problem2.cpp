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
#include <print>
#include <string>

#include <doctest/doctest.h>

#include "include/week3-problem2.hpp"

using namespace std::string_view_literals;

// IMPLEMENTATION
// Vehicle
void hutzdog_cs2_week3::Vehicle::printInfo() const {
    std::println("{}", *this);
}

// Car
void hutzdog_cs2_week3::Car::printInfo() const { std::println("{}", *this); }

// Truck
void hutzdog_cs2_week3::Truck::printInfo() const { std::println("{}", *this); }

// TESTS
// TODO: Implement the tests here
TEST_CASE("testing the Vehicle class") {
    const hutzdog_cs2_week3::Vehicle testVehicle1{"Red", 500};
    const hutzdog_cs2_week3::Vehicle testVehicle2{"Blue", 500};

    SUBCASE("test the constructor & equality operator") {
        REQUIRE(testVehicle1 == hutzdog_cs2_week3::Vehicle(testVehicle1));
        REQUIRE(testVehicle1 != testVehicle2);
    }

    SUBCASE("test vehicle mileage") {
        hutzdog_cs2_week3::Vehicle testMileageVehicle{testVehicle1};
        testMileageVehicle.addMileage(50);
        CHECK(testMileageVehicle ==
              hutzdog_cs2_week3::Vehicle(testVehicle1.color, 550));
    }

    SUBCASE("test vehicle info formatting") {
        CHECK(std::format("{}", testVehicle1) ==
              "A Red vehicle with 500 miles");
        CHECK(std::format("{}", testVehicle2) ==
              "A Blue vehicle with 500 miles");
    }
}

TEST_CASE("testing the Car class") {
    const hutzdog_cs2_week3::Car car1{"Purple", 1024, 4};
    const hutzdog_cs2_week3::Car car2{"Hot Pink", 512, 2};

    SUBCASE("test car equality & constructor") {
        REQUIRE(car1 == hutzdog_cs2_week3::Car(car1));
        REQUIRE(car1 != car2);
    }

    SUBCASE("test car description") {
        CHECK(std::format("{}", car1) ==
              "A Purple car with 4 doors and 1024 miles");
        CHECK(std::format("{}", car2) ==
              "A Hot Pink car with 2 doors and 512 miles");
    }
}

TEST_CASE("testing the Truck class") {
    const hutzdog_cs2_week3::Truck truck1{"Space Gray", 2048, 2'006};
    const hutzdog_cs2_week3::Truck truck2{"Olive", 4192, 2'006};

    SUBCASE("test truck equality & constructor") {
        REQUIRE(truck1 == hutzdog_cs2_week3::Truck(truck1));
        REQUIRE(truck1 != truck2);
    }

    SUBCASE("test truck description") {
        CHECK(std::format("{}", truck1) ==
              "A Space Gray truck with 2048 miles and a carrying capacity of "
              "2006lbs");
        CHECK(
            std::format("{}", truck2) ==
            "A Olive truck with 4192 miles and a carrying capacity of 2006lbs");
    }
}
