#pragma once

#include <string>
#include "date.h"
#include "trade.h"

// ===========================
// TreeProduct Abstract Base Class
// ===========================
// Used for American/European options and any instrument that requires tree pricing
class TreeProduct : public Trade {
public:
    TreeProduct() : Trade("TreeProduct", Date()) {}
    virtual ~TreeProduct() = default;

    // === Pure Virtuals ===
    virtual const Date& getExpiry() const = 0;
    virtual double ValueAtNode(double spot, double t, double continuation) const = 0;
    virtual double Payoff(double spot) const = 0;
    virtual double getNotional() const = 0;
    virtual const std::string& getUnderlying() const = 0;

    // Optional fallback: Pv() defaults to zero since pricing is handled externally
    double pv(const Market& mkt) const override {
        return 0.0;
    }

    const std::string& getType() const override {
        return tradeType;
    }
};