#include "risk_engine.h"
#include "helper.h"
#include <future>
#include <iostream>
#include <stdexcept>

using namespace std;
using util::dateAddTenor;

// ========================
// CurveDecorator
// ========================
CurveDecorator::CurveDecorator(const Market& mkt, const MarketShock& shock)
    : thisMarketUp(mkt), thisMarketDown(mkt)
{
    const Date& tenor = shock.shock.first;
    if (tenor.getYear() <= 1900) {
        cerr << "[WARN] Invalid tenor passed to CurveDecorator: " << tenor << endl;
        return;
    }

    try {
        auto up = thisMarketUp.getCurve(shock.market_id);
        auto down = thisMarketDown.getCurve(shock.market_id);
        up->shock(tenor, +shock.shock.second);
        down->shock(tenor, -shock.shock.second);
    }
    catch (const exception& e) {
        cerr << "[WARN] CurveDecorator failed for " << shock.market_id << ": " << e.what() << endl;
    }
}

const Market& CurveDecorator::getMarketUp() const { return thisMarketUp; }
const Market& CurveDecorator::getMarketDown() const { return thisMarketDown; }

// ========================
// VolDecorator
// ========================
VolDecorator::VolDecorator(const Market& mkt, const MarketShock& shock)
    : originMarket(mkt), thisMarket(mkt)
{
    const Date& tenor = shock.shock.first;
    if (tenor.getYear() <= 1900) {
        cerr << "[WARN] Invalid tenor passed to VolDecorator: " << tenor << endl;
        return;
    }

    try {
        auto vol = thisMarket.getVolCurve(shock.market_id);
        vol->shock(tenor, shock.shock.second);
    }
    catch (const exception& e) {
        cerr << "[WARN] VolDecorator failed for " << shock.market_id << ": " << e.what() << endl;
    }
}

const Market& VolDecorator::getOriginMarket() const { return originMarket; }
const Market& VolDecorator::getMarket() const { return thisMarket; }

// ========================
// PriceDecorator (Optional)
// ========================
PriceDecorator::PriceDecorator(const Market& mkt, const MarketShock& shock)
    : thisMarket(mkt) {
    // Add logic if you plan to shock stock/bond prices
}

const Market& PriceDecorator::getMarket() const { return thisMarket; }

// ========================
// RiskEngine Constructor
// ========================
RiskEngine::RiskEngine(const Market& market, double curve_shock, double vol_shock, double price_shock)
    : curveShockSize(curve_shock), volShockSize(vol_shock), priceShockSize(price_shock)
{
    Date bumpTenor = dateAddTenor(market.asOf, "1Y");

    MarketShock usdShock{ "USD-SOFR", { bumpTenor, curve_shock } };
    MarketShock sgdShock{ "SGD-SORA", { bumpTenor, curve_shock } };
    curveShocks.emplace("USD-SOFR", CurveDecorator(market, usdShock));
    curveShocks.emplace("SGD-SORA", CurveDecorator(market, sgdShock));

    MarketShock volBump{ "LOGVOL", { bumpTenor, vol_shock } };
    volShocks.emplace("LOGVOL", VolDecorator(market, volBump));
}

// ========================
// computeRisk
// ========================

void RiskEngine::computeRisk(string riskType, shared_ptr<Trade> trade, bool singleThread)
{
    result.clear();

    if (singleThread) {
        if (riskType == "dv01") {
            for (auto& kv : curveShocks) {
                const string& market_id = kv.first;
                const Market& mkt_up = kv.second.getMarketUp();
                const Market& mkt_down = kv.second.getMarketDown();

                double pv_up = trade->pv(mkt_up);
                double pv_down = trade->pv(mkt_down);
                double dv01 = (pv_up - pv_down) / (2.0 * curveShockSize);  // Normalize

                result.emplace(market_id, dv01);
            }
        }

        if (riskType == "vega") {
            for (auto& kv : volShocks) {
                const string& market_id = kv.first;
                const Market& mkt_base = kv.second.getOriginMarket();
                const Market& mkt_bump = kv.second.getMarket();

                double pv_base = trade->pv(mkt_base);
                double pv_up = trade->pv(mkt_bump);
                double vega = (pv_up - pv_base) / volShockSize;  // Normalize

                result.emplace(market_id, vega);
            }
        }

        if (riskType == "price") {
            // Optional: implement stock/bond price shock logic here
        }
    }
    else {
        auto async_task = [](shared_ptr<Trade> trade, const string& id, const Market& up, const Market& down, double bumpSize) {
            double pv_up = trade->pv(up);
            double pv_down = trade->pv(down);
            return make_pair(id, (pv_up - pv_down) / (2.0 * bumpSize));
            };

        vector<future<pair<string, double>>> tasks;

        if (riskType == "dv01") {
            for (const auto& kv : curveShocks) {
                tasks.push_back(async(launch::async, async_task,
                    trade, kv.first, kv.second.getMarketUp(), kv.second.getMarketDown(), curveShockSize));
            }
        }

        if (riskType == "vega") {
            for (const auto& kv : volShocks) {
                tasks.push_back(async(launch::async, async_task,
                    trade, kv.first, kv.second.getMarket(), kv.second.getOriginMarket(), volShockSize));
            }
        }

        for (auto& fut : tasks) {
            auto [id, val] = fut.get();
            result.emplace(id, val);
        }
    }
}

// ========================
// getResult
// ========================
map<string, double> RiskEngine::getResult() const {
    return result;
}