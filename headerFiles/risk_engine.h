#pragma once

#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <future>

#include "trade.h"
#include "market.h"

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

    const Market& getOriginMarket() const;
    const Market& getMarket() const;

private:
    Market originMarket;
    Market thisMarket;
};

class PriceDecorator {
public:
    PriceDecorator(const Market& mkt, const MarketShock& priceShock);

    const Market& getMarket() const;

private:
    Market thisMarket;
};

class RiskEngine {
public:
    RiskEngine(const Market& market, double curve_shock, double vol_shock, double price_shock);

    void computeRisk(std::string riskType, std::shared_ptr<Trade> trade, bool singleThread);
    std::map<std::string, double> getResult() const;

private:
    std::unordered_map<std::string, CurveDecorator> curveShocks;
    std::unordered_map<std::string, VolDecorator> volShocks;
    std::unordered_map<std::string, PriceDecorator> priceShocks;

    std::map<std::string, double> result;
};