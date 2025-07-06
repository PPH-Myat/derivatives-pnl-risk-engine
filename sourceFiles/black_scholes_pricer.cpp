#include <cmath>
#include <stdexcept>
#include <iostream>

#include "black_scholes_pricer.h"

// ===========================
// Black-Scholes Pricing Logic
// ===========================
double BlackScholesPricer::price(const Market& mkt, std::shared_ptr<Trade> trade) const {
    // Downcast to EuropeanOption
    const auto* opt = dynamic_cast<const EuropeanOption*>(trade.get());
    if (!opt)
        throw std::runtime_error("BlackScholesPricer only supports EuropeanOption.");

    double S = mkt.getStockPrice(opt->getUnderlying());
    double K = opt->getStrike();
    double T = (opt->getExpiry() - mkt.asOf) / 365.0;  // Use market date for consistency
    double r = mkt.getCurve("USD-SOFR")->getRate(opt->getExpiry());
    double vol = mkt.getVolCurve("LOGVOL")->getVol(opt->getExpiry());

    if (T <= 0 || vol <= 0 || S <= 0 || K <= 0)
        throw std::runtime_error("Invalid inputs for Black-Scholes.");

    double d1 = (std::log(S / K) + (r + 0.5 * vol * vol) * T) / (vol * std::sqrt(T));
    double d2 = d1 - vol * std::sqrt(T);

    double pv = 0.0;
    if (opt->getOptionType() == OptionType::Call)
        pv = S * norm_cdf(d1) - K * std::exp(-r * T) * norm_cdf(d2);
    else
        pv = K * std::exp(-r * T) * norm_cdf(-d2) - S * norm_cdf(-d1);

    // Notional and long/short adjustment
    return opt->isLong() ? pv * opt->getNotional() : -pv * opt->getNotional();
}