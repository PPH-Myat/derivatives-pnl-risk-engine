#pragma once

#include <vector>
#include <memory>
#include <cmath>

#include "trade.h"
#include "tree_product.h"
#include "market.h"

// ============================
// Abstract Pricer Interface
// ============================
class Pricer {
public:
    virtual ~Pricer() = default;
    virtual double price(const Market& mkt, std::shared_ptr<Trade> trade);

protected:
    virtual double priceTree(const Market& mkt, const TreeProduct& trade) { return 0.0; }
};

// ============================
// Abstract Binomial Tree Pricer
// ============================
class BinomialTreePricer : public Pricer {
public:
    explicit BinomialTreePricer(int N) : nTimeSteps(N), states(N + 1) {}

protected:
    int nTimeSteps;
    std::vector<double> states;

    virtual void modelSetup(double S0, double sigma, double rate, double dt) = 0;
    virtual double getSpot(int ti, int si) const = 0;
    virtual double getProbUp() const = 0;
    virtual double getProbDown() const = 0;

    double priceTree(const Market& mkt, const TreeProduct& trade) override;
};

// ============================
// CRR Tree Pricer
// ============================
class CRRBinomialTreePricer : public BinomialTreePricer {
public:
    explicit CRRBinomialTreePricer(int N) : BinomialTreePricer(N) {}

protected:
    void modelSetup(double S0, double sigma, double rate, double dt) override;

    double getSpot(int ti, int si) const override {
        return currentSpot * std::pow(u, ti - 2 * si);
    }

    double getProbUp() const override { return p; }
    double getProbDown() const override { return 1.0 - p; }

private:
    double u;           // up factor
    double d;           // down factor
    double p;           // probability of up move
    double currentSpot; // spot price
};

// ============================
// Jarrow-Rudd Tree Pricer
// ============================
class JRRNBinomialTreePricer : public BinomialTreePricer {
public:
    explicit JRRNBinomialTreePricer(int N) : BinomialTreePricer(N) {}

protected:
    void modelSetup(double S0, double sigma, double rate, double dt) override;

    double getSpot(int ti, int si) const override {
        return currentSpot * std::pow(u, ti - si) * std::pow(d, si);
    }

    double getProbUp() const override { return p; }
    double getProbDown() const override { return 1.0 - p; }

private:
    double u;           // Up factor
    double d;           // Down factor
    double p;           // Probability of up move
    double currentSpot; // Spot price
};