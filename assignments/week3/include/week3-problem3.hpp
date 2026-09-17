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

#ifndef HUTZDOG_CS2_ASSIGN_WEEK3_PROBLEM3
#define HUTZDOG_CS2_ASSIGN_WEEK3_PROBLEM3

namespace hutzdog_cs2_week3 {
///
/// A basic bank account
///
class Account {
  private:
    ///
    /// The name of this account's owner
    /// This would be const, but considering the fact that I got a name change
    /// myself it only felt right to allow for it.
    ///
    std::string ownerName;

    ///
    /// The amount of money in the account
    ///
    double balance;

  public:
    Account(std::string ownerName, double balance)
        : ownerName{std::move(ownerName)}, balance{balance} {}

    ///
    /// Deposit an amount into this account
    /// \param amount The amount to deposit
    ///
    constexpr void deposit(double amount) { this->balance += amount; }

    ///
    /// Alias to deposit, I'm doing this mainly to demonstrate understanding of
    /// operator overloading.
    /// \param amount The amount to deposit
    ///
    constexpr void operator+=(double amount) { this->deposit(amount); }

    ///
    /// Withdraw an amount from this account
    /// \param amount The amount to withdraw
    ///
    constexpr void withdraw(double amount) { this->balance -= amount; }

    ///
    /// Alias to withdraw, I'm doing this mainly to demonstrate understanding of
    /// operator overloading.
    /// \param amount The amount to withdraw
    ///
    constexpr void operator-=(double amount) { this->withdraw(amount); }

    ///
    /// Get the user's balance
    /// \return The user's balance
    ///
    [[nodiscard]] constexpr auto getBalance() const -> double {
        return this->balance;
    }

    ///
    /// Get the account owner's name
    /// This wasn't in the spec but honestly it just makes sense
    /// \return The account owner's name
    ///
    [[nodiscard]] constexpr auto getOwnerName() const -> std::string_view {
        return this->ownerName;
    }

    auto operator==(const Account &other) const -> bool {
        return (typeid(this) == typeid(&other)) ? this->equals(other) : false;
    }

  protected:
    [[nodiscard]] constexpr virtual auto equals(const Account &other) const
        -> bool {
        return this->ownerName == other.ownerName &&
               this->balance == other.balance;
    }

  private:
    friend class std::formatter<Account>;
};

} // namespace hutzdog_cs2_week3

template <> struct std::formatter<hutzdog_cs2_week3::Account> {
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    // Yes I'm formatting to a S-expression. I didn't want to write XML and it's
    // more compact than JSON
    auto format(const hutzdog_cs2_week3::Account &acct,
                std::format_context &ctx) const {
        return std::format_to(ctx.out(),
                              "(Account (owner-name . \"{}\") (balance . {}))",
                              acct.ownerName, acct.balance);
    }
};

#endif // HUTZDOG_CS2_ASSIGN_WEEK3_PROBLEM3
