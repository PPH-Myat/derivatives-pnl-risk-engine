#pragma once

#include "pricer.h"
#include "tree_product.h"

class BinomialTreePricer : public Pricer {
protected:
    int nTimeSteps;
    double u, d, p, currentSpot;
    std::vector<double> states;

    virtual void modelSetup(double S0, double sigma, double rate, double dt) = 0;

    double getSpot(int ti, int si) const;
    double getProbUp() const;
    double getProbDown() const;

public:
    explicit BinomialTreePricer(int N);

    // Pricing
    double price(const Market& mkt, std::shared_ptr<Trade> trade) const override;
    double priceTree(const Market& mkt, const TreeProduct& trade) const;
};

// CRR Tree
class CRRBinomialTreePricer : public BinomialTreePricer {
public:
    explicit CRRBinomialTreePricer(int N);
    void modelSetup(double S0, double sigma, double rate, double dt) override;
};

// Jarrow-Rudd Tree
class JRRNBinomialTreePricer : public BinomialTreePricer {
public:
    explicit JRRNBinomialTreePricer(int N);
    void modelSetup(double S0, double sigma, double rate, double dt) override;
};