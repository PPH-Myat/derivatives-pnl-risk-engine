#pragma once

#include <memory>
#include <vector>
#include <cmath>

#include "pricer.h"
#include "tree_product.h"

// ===========================
// Abstract Binomial Tree Pricer
// ===========================
class BinomialTreePricer : public Pricer {
protected:
    int nTimeSteps;
    mutable double u, d, p, currentSpot;

    virtual void modelSetup(double S0, double sigma, double rate, double dt) const = 0;

    double getSpot(int ti, int si) const;
    double getProbUp() const;
    double getProbDown() const;

public:
    explicit BinomialTreePricer(int N);
    double price(const Market& mkt, std::shared_ptr<Trade> trade) const override;
    double priceTree(const Market& mkt, const TreeProduct& trade) const;
};

// ===========================
// CRR Tree Pricer
// ===========================
class CRRBinomialTreePricer : public BinomialTreePricer {
public:
    explicit CRRBinomialTreePricer(int N);
    void modelSetup(double S0, double sigma, double rate, double dt) const override;
};

// ===========================
// Jarrow-Rudd Tree Pricer
// ===========================
class JRRNBinomialTreePricer : public BinomialTreePricer {
public:
    explicit JRRNBinomialTreePricer(int N);
    void modelSetup(double S0, double sigma, double rate, double dt) const override;
};