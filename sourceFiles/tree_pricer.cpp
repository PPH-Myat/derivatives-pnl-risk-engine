#include <cmath>
#include <stdexcept>
#include "tree_pricer.h"

// Constructor
BinomialTreePricer::BinomialTreePricer(int N)
    : nTimeSteps(N), u(0), d(0), p(0), currentSpot(0) {
    states.resize(N + 1);
}

// Unified price() method
double BinomialTreePricer::price(const Market& mkt, std::shared_ptr<Trade> trade) const {
    if (!trade) throw std::invalid_argument("Null trade pointer");

    auto* treePtr = dynamic_cast<TreeProduct*>(trade.get());
    if (!treePtr) return trade->pv(mkt);

    return priceTree(mkt, *treePtr) * trade->getNotional();
}

// Core tree pricing
double BinomialTreePricer::priceTree(const Market& mkt, const TreeProduct& trade) const {
    double T = (trade.getExpiry() - mkt.asOf) / 365.0;
    double dt = T / nTimeSteps;

    double S0 = mkt.getStockPrice(trade.getUnderlying());
    double sigma = mkt.getVolCurve("LOGVOL")->getVol(trade.getExpiry());
    double rate = mkt.getCurve("USD-SOFR")->getRate(trade.getExpiry());

    modelSetup(S0, sigma, rate, dt);

    for (int i = 0; i <= nTimeSteps; ++i)
        states[i] = trade.payoff(getSpot(nTimeSteps, i));

    for (int k = nTimeSteps - 1; k >= 0; --k) {
        for (int i = 0; i <= k; ++i) {
            double df = std::exp(-rate * dt);
            double continuation = df * (getProbUp() * states[i + 1] + getProbDown() * states[i]);
            states[i] = trade.valueAtNode(getSpot(k, i), dt * k, continuation);
        }
    }

    return states[0];
}

// Utility
double BinomialTreePricer::getSpot(int ti, int si) const {
    return currentSpot * std::pow(u, si) * std::pow(d, ti - si);
}
double BinomialTreePricer::getProbUp() const { return p; }
double BinomialTreePricer::getProbDown() const { return 1.0 - p; }

// CRR setup
CRRBinomialTreePricer::CRRBinomialTreePricer(int N)
    : BinomialTreePricer(N) {
}

void CRRBinomialTreePricer::modelSetup(double S0, double sigma, double rate, double dt) {
    u = std::exp(sigma * std::sqrt(dt));
    d = 1.0 / u;
    p = (std::exp(rate * dt) - d) / (u - d);
    currentSpot = S0;
}

// JRRN setup
JRRNBinomialTreePricer::JRRNBinomialTreePricer(int N)
    : BinomialTreePricer(N) {
}

void JRRNBinomialTreePricer::modelSetup(double S0, double sigma, double rate, double dt) {
    u = std::exp((rate - 0.5 * sigma * sigma) * dt + sigma * std::sqrt(dt));
    d = std::exp((rate - 0.5 * sigma * sigma) * dt - sigma * std::sqrt(dt));
    p = (std::exp(rate * dt) - d) / (u - d);
    currentSpot = S0;
}
