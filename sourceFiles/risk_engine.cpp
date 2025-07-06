#include "risk_engine.h"
#include <future>
#include <iostream>
#include <stdexcept>

using namespace std;

// ========================
// CurveDecorator
// ========================
CurveDecorator::CurveDecorator(const Market& mkt, const MarketShock& shock)
    : thisMarketUp(mkt), thisMarketDown(mkt)
{
    auto up = thisMarketUp.getCurve(shock.market_id);
    auto down = thisMarketDown.getCurve(shock.market_id);
    up->shock(shock.shock.first, +shock.shock.second);
    down->shock(shock.shock.first, -shock.shock.second);
}

const Market& CurveDecorator::getMarketUp() const { return thisMarketUp; }
const Market& CurveDecorator::getMarketDown() const { return thisMarketDown; }

// ========================
// VolDecorator
// ========================
VolDecorator::VolDecorator(const Market& mkt, const MarketShock& shock)
    : originMarket(mkt), thisMarket(mkt)
{
    auto vol = thisMarket.getVolCurve(shock.market_id);
    vol->shock(shock.shock.first, shock.shock.second);
}

const Market& VolDecorator::getOriginMarket() const { return originMarket; }
const Market& VolDecorator::getMarket() const { return thisMarket; }

// ========================
// PriceDecorator (stubbed, extend as needed)
// ========================
PriceDecorator::PriceDecorator(const Market& mkt, const MarketShock& shock)
    : thisMarket(mkt)
{
    // Optional: implement stock/bond price shock logic
}

const Market& PriceDecorator::getMarket() const { return thisMarket; }

// ========================
// RiskEngine Constructor
// ========================
RiskEngine::RiskEngine(const Market& market, double curve_shock, double vol_shock, double price_shock)
{
    MarketShock usdShock{ "USD-SOFR", { Date(), curve_shock } };
    MarketShock sgdShock{ "SGD-SORA", { Date(), curve_shock } };
    curveShocks.emplace("USD-SOFR", CurveDecorator(market, usdShock));
    curveShocks.emplace("SGD-SORA", CurveDecorator(market, sgdShock));

    MarketShock volShock{ "LOGVOL", { Date(), vol_shock } };
    volShocks.emplace("LOGVOL", VolDecorator(market, volShock));

    // priceShock is optional; implement PriceDecorator if needed
}

// ========================
// computeRisk Implementation
// ========================
void RiskEngine::computeRisk(string riskType, shared_ptr<Trade> trade, bool singleThread) {
    result.clear();

    if (singleThread) {
        if (riskType == "dv01") {
            for (const std::pair<const std::string, CurveDecorator>& kv : curveShocks) {
                const string& market_id = kv.first;
                const Market& mkt_up = kv.second.getMarketUp();
                const Market& mkt_down = kv.second.getMarketDown();
                double pv_up = trade->price(mkt_up);
                double pv_down = trade->price(mkt_down);
                result[market_id] = (pv_up - pv_down) / 2.0;
            }
        }
        else if (riskType == "vega") {
            for (const std::pair<const std::string, VolDecorator>& kv : volShocks) {
                const string& market_id = kv.first;
                const Market& mkt_orig = kv.second.getOriginMarket();
                const Market& mkt_up = kv.second.getMarket();
                double pv_base = trade->price(mkt_orig);
                double pv_bump = trade->price(mkt_up);
                result[market_id] = (pv_bump - pv_base);
            }
        }
    }
    else {
        using Result = std::pair<std::string, double>;

        auto task = [](shared_ptr<Trade> t, string id, const Market& up, const Market& down) -> Result {
            double pv_up = t->price(up);
            double pv_down = t->price(down);
            return make_pair(id, (pv_up - pv_down) / 2.0);
            };

        vector<future<Result>> tasks;

        for (const std::pair<const std::string, CurveDecorator>& kv : curveShocks)
            tasks.push_back(async(launch::async, task, trade, kv.first, kv.second.getMarketUp(), kv.second.getMarketDown()));

        for (const std::pair<const std::string, VolDecorator>& kv : volShocks)
            tasks.push_back(async(launch::async, task, trade, kv.first, kv.second.getMarket(), kv.second.getOriginMarket()));

        for (auto& f : tasks) {
            auto [id, val] = f.get();
            result[id] = val;
        }
    }
}

// ========================
// getResult
// ========================
map<string, double> RiskEngine::getResult() const {
    return result;
}