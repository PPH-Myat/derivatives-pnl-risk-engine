#pragma once

#include <memory>
#include <map>
#include <string>
#include "market.h"
#include "trade.h"

struct MarketShock {
    std::string market_id;
    std::pair<Date, double> shock;
};

class CurveDecorator {
public:
    CurveDecorator(const Market& mkt, const MarketShock& curveShock);
    const Market& getMarketUp() const;
    const Market& getMarketDown() const;

private:
    Market thisMarketUp;
    Market thisMarketDown;
};

class VolDecorator {
public:
    VolDecorator(const Market& mkt, const MarketShock& volShock);
    const Market& getMarket() const;
    const Market& getOriginMarket() const;

private:
    Market thisMarket;
    Market originMarket;
};

class PriceDecorator {
public:
    PriceDecorator(const Market& mkt, const MarketShock& shock);
    const Market& getMarket() const;

private:
    Market thisMarket;
};

class RiskEngine {
public:
    RiskEngine(const Market& market, double curve_shock, double vol_shock, double price_shock);

    void computeRisk(std::string riskType, std::shared_ptr<Trade> trade, bool singleThread = true);
    std::map<std::string, double> getResult() const;

private:
    double curveShockSize;
    double volShockSize;
    double priceShockSize;

    std::map<std::string, CurveDecorator> curveShocks;
    std::map<std::string, VolDecorator> volShocks;
    std::map<std::string, double> result;
};