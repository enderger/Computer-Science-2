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

#ifndef HUTZDOG_CS2_ASSIGN_WEEK3_PROBLEM2
#define HUTZDOG_CS2_ASSIGN_WEEK3_PROBLEM2

namespace hutzdog_cs2_week3 {
///
/// A base vehicle, which may
///
class Vehicle {
  public:
    ///
    /// The colour of this car
    /// This is just my coding style
    ///
    const std::string color;

  private:
    ///
    /// The mileage on this vehicle
    ///
    int mileage;

  public:
    // CONSTRUCTORS / DESTRUCTORS
    ///
    /// Construct a new vehicle
    /// \param color The name of the colour of the car
    /// \param mileage The number of miles that this car has driven
    ///
    Vehicle(std::string color, int mileage)
        : color{std::move(color)}, mileage{mileage} {}

    ///
    /// Destroy a vehicle
    ///
    virtual ~Vehicle() = default;

    // GETTERS/SETTERS
    ///
    /// Get this vehicles mileage
    /// \returns The mileage on this vehicle
    ///
    [[nodiscard]] constexpr auto getMileage() const -> int {
        return this->mileage;
    }

    ///
    /// Add to the mileage on this vehicle
    /// \param miles The number of miles to add onto this vehicle
    ///
    constexpr void addMileage(int miles) { this->mileage += miles; }

    // PRINTERS
    ///
    /// Print information about this vehicle
    ///
    virtual void printInfo() const;

    // OPERATORS

    constexpr auto operator==(const Vehicle &other) const -> bool {
        if (typeid(*this) == typeid(other)) {
            return this->equals(other);
        }
        return false;
    }

  protected:
    // MEMBERS
    ///
    /// A hacky equality system propagated down to the child classes
    ///
    [[nodiscard]] constexpr virtual auto equals(const Vehicle &other) const
        -> bool {
        return this->color == other.color && this->mileage == other.mileage;
    }

  private:
    // FRIENDS
    friend class std::formatter<Vehicle>;
    // TODO: Implement the solution here
};

class Car : public Vehicle {
  public:
    ///
    /// The number of doors this car has
    ///
    const int numDoors;

    // CONSTRUCTORS
    ///
    /// Construct a `Car` from its details
    /// \param color The name of the colour of the car
    /// \param mileage The number of miles that this car has driven
    /// \param numDoors The number of doors this car has
    ///
    Car(std::string color, int mileage, int numDoors)
        : Vehicle(std::move(color), mileage), numDoors{numDoors} {}

    // PRINTERS
    void printInfo() const override;

  protected:
    [[nodiscard]] constexpr auto equals(const Vehicle &other) const
        -> bool override {
        if (!Vehicle::equals(other)) {
            return false;
        }

        const auto otherCar = static_cast<const Car &>(other);
        return this->numDoors == otherCar.numDoors;
    }

  private:
    friend class std::formatter<Car>;
};

class Truck : public Vehicle {
  public:
    ///
    /// The cargo capacity of this car
    ///
    const double cargoCapacity;

    // CONSTRUCTORS
    ///
    /// Construct a `Truck` from its details
    /// \param color The name of the colour of the car
    /// \param mileage The number of miles that this car has driven
    /// \param cargoCapacity The carrying capacity of this truck
    ///
    Truck(std::string color, int mileage, double cargoCapacity)
        : Vehicle(std::move(color), mileage), cargoCapacity{cargoCapacity} {}

    // PRINTERS
    void printInfo() const override;

  protected:
    [[nodiscard]] constexpr auto equals(const Vehicle &other) const
        -> bool override {
        if (!Vehicle::equals(other)) {
            return false;
        }

        const auto otherTruck = static_cast<const Truck &>(other);
        return this->cargoCapacity == otherTruck.cargoCapacity;
    }

  private:
    friend class std::formatter<Truck>;
};

} // namespace hutzdog_cs2_week3

template <> struct std::formatter<hutzdog_cs2_week3::Vehicle> {
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const hutzdog_cs2_week3::Vehicle &vehicle,
                std::format_context &ctx) const {
        return std::format_to(ctx.out(), "A {} vehicle with {} miles",
                              vehicle.color, vehicle.mileage);
    }
};

template <> struct std::formatter<hutzdog_cs2_week3::Car> {
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const hutzdog_cs2_week3::Car &vehicle,
                std::format_context &ctx) const {
        return std::format_to(ctx.out(), "A {} car with {} doors and {} miles",
                              vehicle.color, vehicle.numDoors,
                              vehicle.getMileage());
    }
};

template <> struct std::formatter<hutzdog_cs2_week3::Truck> {
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const hutzdog_cs2_week3::Truck &vehicle,
                std::format_context &ctx) const {
        return std::format_to(
            ctx.out(),
            "A {} truck with {} miles and a carrying capacity of {}lbs",
            vehicle.color, vehicle.getMileage(), vehicle.cargoCapacity);
    }
};
#endif // HUTZDOG_CS2_ASSIGN_WEEK3_PROBLEM2
