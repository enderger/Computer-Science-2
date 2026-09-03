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
#include <string>

#ifndef HUTZDOG_CS2_ASSIGN_WEEK3_PROBLEM1
#define HUTZDOG_CS2_ASSIGN_WEEK3_PROBLEM1

namespace hutzdog_cs2_week3 {
///
/// An employee at Company
///
class Employee {
  protected:
    // NOTE: these are protected by spec, ignore the pedantic warnings
    ///
    /// The employee's name
    ///
    // NOLINTNEXTLINE
    std::string name;

    ///
    /// The employee's base salary
    ///
    // NOLINTNEXTLINE
    double baseSalary;

  public:
    ///
    /// Construct an `Employee` from its components
    /// \param name The name of the employee
    /// \param baseSalary The base salary of the employee
    ///
    Employee(std::string name, double baseSalary)
        : name{std::move(name)}, baseSalary{baseSalary} {}

    ///
    /// Print some info about the employee
    ///
    void printInfo() const;
};

///
/// A manager at Company
///
class Manager : public Employee {
  private:
    ///
    /// The manager's bonus
    ///
    double bonusPercentage;

  public:
    ///
    /// Construct a `Manager` from its components
    /// \param name The name of the manager
    /// \param baseSalary The base salary of the manager
    /// \param bonusPercentage The manager's bonus percentage
    ///
    Manager(std::string name, double baseSalary, double bonusPercentage)
        : Employee(std::move(name), baseSalary),
          bonusPercentage{bonusPercentage} {}

    ///
    /// Get the total compensation given to this manager
    /// \returns The manager's actual wage
    ///
    [[nodiscard]] constexpr auto totalCompensation() const -> double;
};

} // namespace hutzdog_cs2_week3

#endif // HUTZDOG_CS2_ASSIGN_WEEK3_PROBLEM1
