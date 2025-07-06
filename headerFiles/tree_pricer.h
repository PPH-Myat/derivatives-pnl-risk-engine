#pragma once

#include <vector>
#include <cmath>
#include <memory>

#include "pricer.h"
#include "tree_product.h"
#include "market.h"

// ===========================
// Abstract Binomial Tree Pricer
// ===========================
class BinomialTreePricer : public Pricer {
public:
    explicit BinomialTreePricer(int N);

    double price(const Market& mkt, std::shared_ptr<Trade> trade) const override;
    double priceTree(const Market& mkt, const TreeProduct& trade) const;

protected:
    virtual void modelSetup(double S0, double sigma, double rate, double dt) const = 0;

    int nTimeSteps;
    mutable double u, d, p, currentSpot;

    double getSpot(int ti, int si) const;
    double getProbUp() const;
    double getProbDown() const;
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