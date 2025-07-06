#include <cmath>
#include <stdexcept>
#include <vector>
#include <iostream>

#include "tree_pricer.h"
#include "market.h"
#include "european_trade.h"

// ===========================
// BinomialTreePricer Constructor
// ===========================
BinomialTreePricer::BinomialTreePricer(int N)
    : nTimeSteps(N), u(0.0), d(0.0), p(0.0), currentSpot(0.0) {
    states.resize(nTimeSteps + 1);
}

// ===========================
// BinomialTreePricer Interface (must override Pricer)
// ===========================
double BinomialTreePricer::price(const Market& mkt, Trade* trade) {
    if (!trade)
        throw std::invalid_argument("[ERROR] Trade pointer is null.");

    auto* treePtr = dynamic_cast<TreeProduct*>(trade);
    if (treePtr)
        return priceTree(mkt, *treePtr);
    else
        return trade->price(mkt);
}

// ===========================
// Tree Pricing Core
// ===========================
double BinomialTreePricer::priceTree(const Market& mkt, const TreeProduct& trade) {
    double T = (trade.getExpiry() - mkt.asOf) / 365.0;
    double dt = T / nTimeSteps;

    std::string underlying = trade.getUnderlying();
    double S0 = mkt.getStockPrice(underlying);
    double sigma = mkt.getVolCurve("VOL")->getVol(trade.getExpiry());
    double rate = mkt.getCurve("USD-SOFR")->getRate(trade.getExpiry());

    modelSetup(S0, sigma, rate, dt);

    // Terminal payoffs
    for (int i = 0; i <= nTimeSteps; ++i)
        states[i] = trade.payoff(getSpot(nTimeSteps, i));

    // Backward induction
    for (int k = nTimeSteps - 1; k >= 0; --k) {
        for (int i = 0; i <= k; ++i) {
            double df = std::exp(-rate * dt);
            double continuation = df * (getProbUp() * states[i + 1] + getProbDown() * states[i]);
            double spot = getSpot(k, i);
            states[i] = trade.valueAtNode(spot, dt * k, continuation);
        }
    }

    return states[0];
}

// ===========================
// Fallback Tree Setup
// ===========================
void BinomialTreePricer::modelSetup(double S0, double sigma, double rate, double dt) {
    std::cerr << "[WARN] BinomialTreePricer::modelSetup fallback called. Override expected.\n";
    u = 1.001;
    d = 0.999;
    p = (std::exp(rate * dt) - d) / (u - d);
    currentSpot = S0;
}

double BinomialTreePricer::getSpot(int ti, int si) const {
    return currentSpot * std::pow(u, si) * std::pow(d, ti - si);
}

double BinomialTreePricer::getProbUp() const { return p; }
double BinomialTreePricer::getProbDown() const { return 1.0 - p; }

// ===========================
// CRR Binomial Tree
// ===========================
CRRBinomialTreePricer::CRRBinomialTreePricer(int N)
    : BinomialTreePricer(N) {
}

void CRRBinomialTreePricer::modelSetup(double S0, double sigma, double rate, double dt) {
    u = std::exp(sigma * std::sqrt(dt));
    d = 1.0 / u;
    p = (std::exp(rate * dt) - d) / (u - d);
    currentSpot = S0;
}

double CRRBinomialTreePricer::getSpot(int ti, int si) const {
    return currentSpot * std::pow(u, si) * std::pow(d, ti - si);
}

// ===========================
// JRRN Binomial Tree
// ===========================
JRRNBinomialTreePricer::JRRNBinomialTreePricer(int N)
    : BinomialTreePricer(N) {
}

void JRRNBinomialTreePricer::modelSetup(double S0, double sigma, double rate, double dt) {
    u = std::exp((rate - 0.5 * sigma * sigma) * dt + sigma * std::sqrt(dt));
    d = std::exp((rate - 0.5 * sigma * sigma) * dt - sigma * std::sqrt(dt));
    p = 0.5;
    currentSpot = S0;
}