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
    catch (const std::exception& e) {
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
    catch (const std::exception& e) {
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
    // Optional: implement stock/bond price shock logic if needed
}

const Market& PriceDecorator::getMarket() const { return thisMarket; }

// ========================
// RiskEngine Constructor
// ========================
RiskEngine::RiskEngine(const Market& market, double curve_shock, double vol_shock, double price_shock)
{
    Date bumpTenor = dateAddTenor(market.asOf, "1Y");

    MarketShock usdShock{ "USD-SOFR", { bumpTenor, curve_shock } };
    MarketShock sgdShock{ "SGD-SORA", { bumpTenor, curve_shock } };
    curveShocks.emplace("USD-SOFR", CurveDecorator(market, usdShock));
    curveShocks.emplace("SGD-SORA", CurveDecorator(market, sgdShock));

    MarketShock volShock{ "LOGVOL", { bumpTenor, vol_shock } };
    volShocks.emplace("LOGVOL", VolDecorator(market, volShock));

    // priceShock is currently not used
}

// ========================
// computeRisk
// ========================
void RiskEngine::computeRisk(string riskType, shared_ptr<Trade> trade, bool singleThread) {
    result.clear();

    if (singleThread) {
        if (riskType == "dv01") {
            for (const auto& kv : curveShocks) {
                const string& market_id = kv.first;
                const Market& mkt_up = kv.second.getMarketUp();
                const Market& mkt_down = kv.second.getMarketDown();
                double pv_up = trade->price(mkt_up);
                double pv_down = trade->price(mkt_down);
                result[market_id] = (pv_up - pv_down) / 2.0;
            }
        }
        else if (riskType == "vega") {
            for (const auto& kv : volShocks) {
                const string& market_id = kv.first;
                const Market& mkt_orig = kv.second.getOriginMarket();
                const Market& mkt_up = kv.second.getMarket();
                double pv_base = trade->price(mkt_orig);
                double pv_bump = trade->price(mkt_up);
                result[market_id] = pv_bump - pv_base;
            }
        }
    }
    else {
        using Result = pair<string, double>;
        vector<future<Result>> tasks;

        for (const auto& kv : curveShocks) {
            tasks.emplace_back(async(launch::async,
                [&trade](string id, const Market& up, const Market& down) {
                    double pv_up = trade->price(up);
                    double pv_down = trade->price(down);
                    return make_pair(id, (pv_up - pv_down) / 2.0);
                }, kv.first, cref(kv.second.getMarketUp()), cref(kv.second.getMarketDown())));
        }

        for (const auto& kv : volShocks) {
            tasks.emplace_back(async(launch::async,
                [&trade](string id, const Market& up, const Market& base) {
                    double pv_up = trade->price(up);
                    double pv_base = trade->price(base);
                    return make_pair(id, pv_up - pv_base);
                }, kv.first, cref(kv.second.getMarket()), cref(kv.second.getOriginMarket())));
        }

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