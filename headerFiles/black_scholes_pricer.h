#pragma once

#include <memory>
#include <cmath>
#include "pricer.h"
#include "market.h"
#include "european_trade.h"

// ===========================
// Utility: Cumulative Normal CDF
// ===========================
inline double norm_cdf(double x) {
    return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

// ===========================
// Black-Scholes Pricer Class
// ===========================
class BlackScholesPricer : public Pricer {
public:
    double price(const Market& mkt, std::shared_ptr<Trade> trade) const override;
};