#include <cmath>
#include <stdexcept>
#include <iostream>

#include "black_scholes_pricer.h"

// ===========================
// Black-Scholes Pricing Logic
// ===========================
double BlackScholesPricer::price(const Market& mkt, Trade* trade) {
    // Downcast to EuropeanOption (safe cast check)
    const EuropeanOption* opt = dynamic_cast<const EuropeanOption*>(trade);
    if (!opt)
        throw std::runtime_error("BlackScholesPricer only supports EuropeanOption.");

    double S = mkt.getStockPrice(opt->getUnderlying());
    double K = opt->getStrike();
    double T = (opt->getExpiry() - opt->getTradeDate());
    double r = mkt.getCurve("USD-SOFR").getRate(opt->getExpiry());
    double vol = mkt.getVolCurve("VOL").getVol(opt->getExpiry());

    if (T <= 0 || vol <= 0 || S <= 0 || K <= 0) {
        throw std::runtime_error("Invalid inputs for Black-Scholes.");
    }

    double d1 = (log(S / K) + (r + 0.5 * vol * vol) * T) / (vol * sqrt(T));
    double d2 = d1 - vol * sqrt(T);

    if (opt->getOptionType() == OptionType::Call) {
        return S * norm_cdf(d1) - K * exp(-r * T) * norm_cdf(d2);
    }
    else {
        return K * exp(-r * T) * norm_cdf(-d2) - S * norm_cdf(-d1);
    }
}
