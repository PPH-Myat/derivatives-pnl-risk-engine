#include "risk_engine.h"
#include <future>
#include <iostream>
#include <stdexcept>

using namespace std;

void RiskEngine::computeRisk(const std::string& riskType, const std::shared_ptr<Trade>& trade, bool singleThread) {
    result.clear();

    if (singleThread) {
        if (riskType == "dv01") {
            for (const auto& kv : curveShocks) {
                const std::string& market_id = kv.first;
                const Market& mkt_up = kv.second.getMarketUp();
                const Market& mkt_down = kv.second.getMarketDown();
                double pv_up = trade->price(mkt_up);
                double pv_down = trade->price(mkt_down);
                double dv01 = (pv_up - pv_down) / 2.0;
                result.emplace(market_id, dv01);
            }
        }

        if (riskType == "vega") {
            for (const auto& kv : volShocks) {
                const std::string& market_id = kv.first;
                const Market& mkt_orig = kv.second.getOriginMarket();
                const Market& mkt_bumped = kv.second.getMarket();
                double pv_orig = trade->price(mkt_orig);
                double pv_up = trade->price(mkt_bumped);
                double vega = pv_up - pv_orig;
                result.emplace(market_id, vega);
            }
        }

        if (riskType == "price") {
            // TODO: implement price sensitivity
        }
    }
    else {
        // Async task lambda
        auto pv_task = [](std::shared_ptr<Trade> trade,
            const std::string& id,
            const Market& mkt_up,
            const Market& mkt_down) {
                double pv_up = trade->price(mkt_up);
                double pv_down = trade->price(mkt_down);
                double risk = (pv_up - pv_down) / 2.0;
                return std::make_pair(id, risk);
            };

        std::vector<std::future<std::pair<std::string, double>>> _futures;

        for (const auto& kv : curveShocks) {
            const std::string& market_id = kv.first;
            const Market& mkt_up = kv.second.getMarketUp();
            const Market& mkt_down = kv.second.getMarketDown();
            _futures.push_back(std::async(std::launch::async, pv_task, trade, market_id, mkt_up, mkt_down));
        }

        for (const auto& kv : volShocks) {
            const std::string& market_id = kv.first;
            const Market& mkt_orig = kv.second.getOriginMarket();
            const Market& mkt_bumped = kv.second.getMarket();
            _futures.push_back(std::async(std::launch::async, pv_task, trade, market_id, mkt_bumped, mkt_orig));
        }

        for (auto&& fut : _futures) {
            auto rs = fut.get();
            result.emplace(rs);
        }
    }
}