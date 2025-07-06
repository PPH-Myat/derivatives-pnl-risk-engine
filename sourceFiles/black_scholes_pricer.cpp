#include "black_scholes_pricer.h"
#include "european_trade.h"
#include <cmath>
#include <stdexcept>

inline double norm_cdf(double x) {
    return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

double BlackScholesPricer::price(const Market& mkt, std::shared_ptr<Trade> trade) const {
    // Cast to EuropeanOption only
    auto opt = std::dynamic_pointer_cast<EuropeanOption>(trade);
    if (!opt)
        throw std::runtime_error("Black-Scholes pricer only supports EuropeanOption");

    // Fetch input data
    double S = mkt.getStockPrice(opt->getUnderlying());
    double K = opt->getStrike();
    double T = (opt->getExpiry() - mkt.asOf) / 365.0;
    double sigma = mkt.getVolCurve("LOGVOL")->getVol(opt->getVolTenor());
    double r = mkt.getCurve(opt->getRateCurve())->getRate(opt->getExpiry());

    // Handle edge case (e.g. expired)
    if (T <= 0.0 || sigma <= 0.0)
        return opt->payoff(S);

    // Compute Black-Scholes formula
    double sqrtT = std::sqrt(T);
    double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrtT);
    double d2 = d1 - sigma * sqrtT;
    double df = std::exp(-r * T);

    double bsPrice;
    if (opt->getOptionType() == OptionType::Call)
        bsPrice = S * norm_cdf(d1) - K * df * norm_cdf(d2);
    else
        bsPrice = K * df * norm_cdf(-d2) - S * norm_cdf(-d1);

    // Apply sign and notional
    double sign = opt->isLong() ? 1.0 : -1.0;
    double result = sign * opt->getNotional() * bsPrice;

    return result;
}