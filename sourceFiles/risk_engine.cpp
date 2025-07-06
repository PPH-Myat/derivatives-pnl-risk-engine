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
// PriceDecorator (not used)
// ========================
PriceDecorator::PriceDecorator(const Market& mkt, const MarketShock& shock)
    : thisMarket(mkt) {
    // Optional: implement price shock if needed
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

    // priceShock currently unused
}

// ========================
// computeRisk
// ========================
void RiskEngine::computeRisk(string riskType, shared_ptr<Trade> trade, bool singleThread) {
    result.clear();

    if (singleThread) {
        if (riskType == "dv01") {
            for (const auto& [market_id, decorator] : curveShocks) {
                double pv_up = trade->price(decorator.getMarketUp());
                double pv_down = trade->price(decorator.getMarketDown());
                result[market_id] = (pv_up - pv_down);  // caller will divide by 2 × shock
            }
        }
        else if (riskType == "vega") {
            for (const auto& [market_id, decorator] : volShocks) {
                double pv_base = trade->price(decorator.getOriginMarket());
                double pv_bump = trade->price(decorator.getMarket());
                result[market_id] = (pv_bump - pv_base);  // caller will divide by shock
            }
        }
    }
    else {
        using Result = pair<string, double>;
        vector<future<Result>> tasks;

        if (riskType == "dv01") {
            for (const auto& [id, decorator] : curveShocks) {
                tasks.emplace_back(async(launch::async, [=]() {
                    double pv_up = trade->price(decorator.getMarketUp());
                    double pv_down = trade->price(decorator.getMarketDown());
                    return make_pair(id, (pv_up - pv_down));
                    }));
            }
        }
        else if (riskType == "vega") {
            for (const auto& [id, decorator] : volShocks) {
                tasks.emplace_back(async(launch::async, [=]() {
                    double pv_bump = trade->price(decorator.getMarket());
                    double pv_base = trade->price(decorator.getOriginMarket());
                    return make_pair(id, (pv_bump - pv_base));
                    }));
            }
        }

        for (auto& task : tasks) {
            auto [id, value] = task.get();
            result[id] = value;
        }
    }
}

// ========================
// getResult
// ========================
map<string, double> RiskEngine::getResult() const {
    return result;
}