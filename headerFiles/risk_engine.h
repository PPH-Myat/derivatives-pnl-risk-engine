#pragma once

#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <future>

#include "trade.h"
#include "market.h"

using namespace std;

struct MarketShock {
    string market_id;
    pair<Date, double> shock; //tenor and value
};

class CurveDecorator : public Market {
public:
    CurveDecorator(const Market& mkt, const MarketShock& curveShock)
        : thisMarketUp(mkt), thisMarketDown(mkt)
    {
        std::cout << "[CurveDecorator] created" << std::endl;
        auto curve_up = thisMarketUp.getCurve(curveShock.market_id);
        curve_up->shock(curveShock.shock.first, curveShock.shock.second);
        std::cout << "[CurveDecorator] tenor " << curveShock.shock.first
            << " shocked up by " << curveShock.shock.second << std::endl;

        auto curve_down = thisMarketDown.getCurve(curveShock.market_id);
        curve_down->shock(curveShock.shock.first, -curveShock.shock.second);
        std::cout << "[CurveDecorator] tenor " << curveShock.shock.first
            << " shocked down by " << -curveShock.shock.second << std::endl;
    }

    inline const Market getMarketUp() const { return thisMarketUp; }
    inline const Market getMarketDown() const { return thisMarketDown; }

private:
    Market thisMarketUp;
    Market thisMarketDown;
};

class VolDecorator : public Market {
public:
    VolDecorator(const Market& mkt, const MarketShock& volShock)
        : originMarket(mkt), thisMarket(mkt)
    {
        std::cout << "[VolDecorator] created" << std::endl;
        auto volcurve = thisMarket.getVolCurve(volShock.market_id);
        volcurve->shock(volShock.shock.first, volShock.shock.second);
        std::cout << "[VolDecorator] tenor " << volShock.shock.first
            << " shocked by " << volShock.shock.second << std::endl;
    }

    inline const Market& getOriginMarket() const { return originMarket; }
    inline const Market& getMarket() const { return thisMarket; }

private:
    Market originMarket;
    Market thisMarket;
};

class PriceDecorator : public Market {
public:
    PriceDecorator(const Market& mkt, const MarketShock& priceShock) : thisMarket(mkt)
    {
        cout << "stock price decorator is created" << endl;
        thisMarket.shockPrice(priceShock.market_id, priceShock.shock.second);
    }

    inline const Market& getMarket() const { return thisMarket; }

private:
    Market thisMarket;
};

class RiskEngine
{
public:

    RiskEngine(const Market& market, double curve_shock, double vol_shock, double price_shock)
    {
        // Curve shocks
        MarketShock usdShock{ "USD-SOFR", {Date(), curve_shock} };
        MarketShock sgdShock{ "SGD-SORA", {Date(), curve_shock} };

        curveShocks.emplace("USD-SOFR", CurveDecorator(market, usdShock));
        curveShocks.emplace("SGD-SORA", CurveDecorator(market, sgdShock));

        // Volatility shock
        MarketShock volShock{ "LOGVOL", {Date(), vol_shock} };
        volShocks.emplace("LOGVOL", VolDecorator(market, volShock));

        std::cout << "[RiskEngine] created with shocks" << std::endl;
    }


    void computeRisk(string riskType, std::shared_ptr<Trade> trade, bool singleThread);

    inline map<string, double> getResult() const {
        cout << " risk result: " << endl;
        return result;
    };

private:
    unordered_map<string, CurveDecorator> curveShocks; //tenor, shock
    unordered_map<string, VolDecorator> volShocks;
    unordered_map<string, PriceDecorator> priceShocks;

    map<string, double> result;

};