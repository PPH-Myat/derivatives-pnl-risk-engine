#pragma once

#include "tree_pricer.h"
#include "market.h"
#include "european_trade.h"

// Utility: cumulative normal distribution function
inline double norm_cdf(double x) {
    return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

// ===========================
// Black-Scholes Pricer Class
// ===========================
class BlackScholesPricer : public Pricer {
public:
    // Override base class pricing interface
    double price(const Market& mkt, Trade* trade) override;
};
