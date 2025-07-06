#include <cmath>
#include <stdexcept>
#include "pricer.h"

// ===================================
// Generic Trade Pricer Entry Point
// ===================================
double Pricer::price(const Market& mkt, std::shared_ptr<Trade> trade)
{
    if (!trade)
        throw std::invalid_argument("[ERROR] Trade pointer is null.");

    auto* treePtr = dynamic_cast<TreeProduct*>(trade.get());
    if (treePtr) {
        return priceTree(mkt, *treePtr) * trade->getNotional();
    }

    return trade->pv(mkt);
}

// ===================================
// Binomial Tree Pricing Logic
// ===================================
double BinomialTreePricer::priceTree(const Market& mkt, const TreeProduct& trade)
{
    double T = (trade.getExpiry() - mkt.asOf) / 365.0;
    double dt = T / nTimeSteps;

    double S0 = mkt.getStockPrice(trade.getUnderlying());
    double sigma = mkt.getVolCurve("LOGVOL")->getVol(trade.getExpiry());
    double rate = mkt.getCurve("USD-SOFR")->getRate(trade.getExpiry());

    modelSetup(S0, sigma, rate, dt);

    // Terminal payoff
    for (int i = 0; i <= nTimeSteps; ++i) {
        states[i] = trade.payoff(getSpot(nTimeSteps, i));
    }

    // Backward induction
    for (int k = nTimeSteps - 1; k >= 0; --k) {
        for (int i = 0; i <= k; ++i) {
            double df = std::exp(-rate * dt);
            double continuation = df * (getProbUp() * states[i] + getProbDown() * states[i + 1]);
            states[i] = trade.valueAtNode(getSpot(k, i), dt * k, continuation);
        }
    }

    return states[0];
}

// ===================================
// CRR Binomial Tree Setup
// ===================================
void CRRBinomialTreePricer::modelSetup(double S0, double sigma, double rate, double dt)
{
    u = std::exp(sigma * std::sqrt(dt));
    d = 1.0 / u;
    p = (std::exp(rate * dt) - d) / (u - d);
    currentSpot = S0;
}

// ===================================
// Jarrow-Rudd Binomial Tree Setup
// ===================================
void JRRNBinomialTreePricer::modelSetup(double S0, double sigma, double rate, double dt)
{
    u = std::exp((rate - 0.5 * sigma * sigma) * dt + sigma * std::sqrt(dt));
    d = std::exp((rate - 0.5 * sigma * sigma) * dt - sigma * std::sqrt(dt));
    p = (std::exp(rate * dt) - d) / (u - d);
    currentSpot = S0;
}