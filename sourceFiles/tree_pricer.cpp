#include <cmath>
#include <stdexcept>
#include <vector>

#include "tree_pricer.h"
#include "market.h"
#include "trade.h"
#include "tree_product.h"
#include "european_trade.h"
#include "american_trade.h" 

// ===========================
// BinomialTreePricer Base
// ===========================

BinomialTreePricer::BinomialTreePricer(int N)
    : nTimeSteps(N), u(0), d(0), p(0), currentSpot(0) {
}

double BinomialTreePricer::price(const Market& mkt, std::shared_ptr<Trade> trade) const {
    if (!trade) throw std::invalid_argument("Null trade pointer");

    auto treePtr = std::dynamic_pointer_cast<TreeProduct>(trade);
    if (!treePtr) {
        // === Safe fallback routing ===
        if (auto opt = std::dynamic_pointer_cast<EuropeanOption>(trade))
            return opt->pv(mkt, false);
        if (auto spread = std::dynamic_pointer_cast<EuroCallSpread>(trade))
            return spread->pv(mkt, false);
        if (auto amer = std::dynamic_pointer_cast<AmericanOption>(trade))
            return amer->pv(mkt, false);

        return trade->pv(mkt);  // fallback for unknown tree-incompatible trades
    }

    return priceTree(mkt, *treePtr) * (trade->isLong() ? 1.0 : -1.0);
}

double BinomialTreePricer::priceTree(const Market& mkt, const TreeProduct& trade) const {
    double T = (trade.getExpiry() - mkt.asOf) / 365.0;
    double dt = T / nTimeSteps;

    double S0 = mkt.getStockPrice(trade.getUnderlying());
    double sigma = mkt.getVolCurve("LOGVOL")->getVol(trade.getExpiry());
    double rate = mkt.getCurve("USD-SOFR")->getRate(trade.getExpiry());

    modelSetup(S0, sigma, rate, dt);

    std::vector<double> states(nTimeSteps + 1);

    // Terminal payoff
    for (int i = 0; i <= nTimeSteps; ++i) {
        states[i] = trade.payoff(getSpot(nTimeSteps, i));
    }

    // Backward induction
    for (int k = nTimeSteps - 1; k >= 0; --k) {
        for (int i = 0; i <= k; ++i) {
            double df = std::exp(-rate * dt);
            double continuation = df * (getProbUp() * states[i + 1] + getProbDown() * states[i]);
            states[i] = trade.valueAtNode(getSpot(k, i), dt * k, continuation);
        }
    }

    return states[0];
}

double BinomialTreePricer::getSpot(int ti, int si) const {
    return currentSpot * std::pow(u, si) * std::pow(d, ti - si);
}

double BinomialTreePricer::getProbUp() const { return p; }

double BinomialTreePricer::getProbDown() const { return 1.0 - p; }

// ===========================
// CRR Tree
// ===========================

CRRBinomialTreePricer::CRRBinomialTreePricer(int N)
    : BinomialTreePricer(N) {
}

void CRRBinomialTreePricer::modelSetup(double S0, double sigma, double rate, double dt) const {
    u = std::exp(sigma * std::sqrt(dt));
    d = 1.0 / u;
    p = (std::exp(rate * dt) - d) / (u - d);
    currentSpot = S0;
}

// ===========================
// Jarrow-Rudd Tree
// ===========================

JRRNBinomialTreePricer::JRRNBinomialTreePricer(int N)
    : BinomialTreePricer(N) {
}

void JRRNBinomialTreePricer::modelSetup(double S0, double sigma, double rate, double dt) const {
    u = std::exp((rate - 0.5 * sigma * sigma) * dt + sigma * std::sqrt(dt));
    d = std::exp((rate - 0.5 * sigma * sigma) * dt - sigma * std::sqrt(dt));
    p = (std::exp(rate * dt) - d) / (u - d);
    currentSpot = S0;
}