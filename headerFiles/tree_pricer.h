#pragma once

#include <vector>
#include <cmath>
#include <memory>
#include <string>

#include "trade.h"
#include "tree_product.h"
#include "market.h"

// ===========================
// Abstract Pricer Interface
// ===========================
class Pricer {
public:
    virtual ~Pricer() = default;

    // Entry point for pricing a Trade object (can be TreeProduct or not)
    virtual double price(const Market& mkt, Trade* trade);

protected:
    // Internal pricing logic for tree-based products
    virtual double priceTree(const Market& mkt, const TreeProduct& trade); // Removed const
};

// ===========================
// Binomial Tree Pricer Base
// ===========================
class BinomialTreePricer : public Pricer {
public:
    explicit BinomialTreePricer(int N);
    ~BinomialTreePricer() override = default;

    double price(const Market& mkt, Trade* trade) override;
    double priceTree(const Market& mkt, const TreeProduct& trade) override; // Removed const

protected:
    virtual void modelSetup(double S0, double sigma, double rate, double dt);
    virtual double getSpot(int ti, int si) const;
    virtual double getProbUp() const;
    virtual double getProbDown() const;

    int nTimeSteps;
    double u = 0.0;
    double d = 0.0;
    double p = 0.0;
    double currentSpot = 0.0;

    std::vector<double> states;
};

// ===========================
// CRR Binomial Tree Model
// ===========================
class CRRBinomialTreePricer : public BinomialTreePricer {
public:
    explicit CRRBinomialTreePricer(int N);

protected:
    void modelSetup(double S0, double sigma, double rate, double dt) override;
    double getSpot(int ti, int si) const override;
};

// ===========================
// JRRN Binomial Tree Model
// ===========================
class JRRNBinomialTreePricer : public BinomialTreePricer {
public:
    explicit JRRNBinomialTreePricer(int N);

protected:
    void modelSetup(double S0, double sigma, double rate, double dt) override;
};