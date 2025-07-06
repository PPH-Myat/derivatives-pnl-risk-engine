#pragma once

#include <string>
#include <memory>
#include "date.h"
#include "Types.h"

class Market;

class Trade {
public:
    Trade() = default;

    Trade(const std::string& type, const Date& tradeDate)
        : tradeType(type), tradeDate(tradeDate) {
    }

    virtual ~Trade() = default;

    // === Core Pricing Interfaces ===
    virtual double price(const Market& market) const = 0;
    virtual double pv(const Market& market) const = 0;

    // === payoff Interfaces ===
    virtual double payoff(double marketPrice) const = 0;
    virtual double payoff(const Market& market) const = 0;

    // === Tree model override
    virtual double valueAtNode(double S, double t, double continuationValue) const = 0;

    // === Accessors ===
    virtual const std::string& getType() const { return tradeType; }
    virtual const std::string& getUnderlying() const = 0;
    virtual const std::string& getRateCurve() const = 0;
    virtual double getNotional() const = 0;
    virtual double getStrike() const = 0;
    virtual OptionType getOptionType() const = 0;
    virtual const Date& getTradeDate() const = 0;
    virtual const Date& getExpiry() const = 0;

    // === Position Direction ===
    virtual bool isLong() const { return isLong_; }
    virtual void setLong(bool val) { isLong_ = val; }

    // === Cloning Support ===
    virtual std::shared_ptr<Trade> clone() const = 0;

protected:
    std::string tradeType;
    Date tradeDate;
    bool isLong_ = true; // default to long
};