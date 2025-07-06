#pragma once

#include <memory>
#include <string>
#include <stdexcept>

#include "swap.h"
#include "bond.h"
#include "european_trade.h"
#include "american_trade.h"
#include "types.h"
#include "helper.h"

// ==========================================
// Abstract Base Class for All Trade Factories
// ==========================================
class TradeFactory {
public:
    virtual std::shared_ptr<Trade> createTrade(const std::string& underlying,
        const Date& start,
        const Date& end,
        double notional,
        double param1,       // Could be rate, price, or strike
        double freq,
        OptionType opt) const = 0;

    virtual ~TradeFactory() = default;
};

// ==========================================
// Swap Factory
// ==========================================
class SwapFactory : public TradeFactory {
public:
    std::shared_ptr<Trade> createTrade(const std::string& underlying,
        const Date& start,
        const Date& end,
        double notional,
        double rate,
        double freq,
        OptionType /*opt*/) const override {
        if (freq <= 0 || freq > 1)
            throw std::invalid_argument("Invalid swap frequency.");
        return std::make_shared<Swap>(underlying, start, end, notional, rate, freq);
    }
};

// ==========================================
// Bond Factory
// ==========================================
class BondFactory : public TradeFactory {
public:
    std::shared_ptr<Trade> createTrade(const std::string& underlying,
        const Date& start,
        const Date& end,
        double notional,
        double rate,
        double freq,
        OptionType /*opt*/) const override {

        std::string mappedCurve = util::to_upper(underlying);
        if (mappedCurve == "USD-GOV") mappedCurve = "USD-SOFR";
        if (mappedCurve == "SGD-GOV") mappedCurve = "SGD-SORA";

        return std::make_shared<Bond>(mappedCurve, start, end, notional, rate, freq);
    }
};

// ==========================================
// European Option Factory
// ==========================================
class EurOptFactory : public TradeFactory {
public:
    std::shared_ptr<Trade> createTrade(const std::string& underlying,
        const Date& start,
        const Date& end,
        double notional,
        double strike,
        double /*freq*/,
        OptionType opt) const override {
        return std::make_shared<EuropeanOption>(opt, notional, strike, start, end, underlying);
    }
};

// ==========================================
// American Option Factory
// ==========================================
class AmericanOptFactory : public TradeFactory {
public:
    std::shared_ptr<Trade> createTrade(const std::string& underlying,
        const Date& start,
        const Date& end,
        double notional,
        double strike,
        double /*freq*/,
        OptionType opt) const override {
        return std::make_shared<AmericanOption>(opt, notional, strike, start, end, underlying);
    }
};