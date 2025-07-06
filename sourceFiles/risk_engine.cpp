#include "ris_engine.h"
#include <future>
#include <iostream>
#include <stdexcept>

using namespace std;

void RiskEngine::computeRisk(const std::string& riskType, const std::shared_ptr<Trade>& trade, bool singleThread) {
    result.clear();

    if (singleThread) {
        if (riskType == "dv01") {
            for (auto& kv : curveShocks) {
                const std::string& market_id = kv.first;
                const Market& mkt_u = kv.second.getMarketUp();
                const Market& mkt_d = kv.second.getMarketDown();
                double pv_up = trade->Price(mkt_u);
                double pv_down = trade->Price(mkt_d);
                double dv01 = (pv_up - pv_down) / 2.0;
                result.emplace(market_id, dv01);
            }
        }

        if (riskType == "vega") {
            for (auto& kv : volShocks) {
                const std::string& market_id = kv.first;
                const Market& mkt = kv.second.getOriginMarket();
                const Market& mkt_s = kv.second.getMarket();
                double pv = trade->Price(mkt);
                double pv_up = trade->Price(mkt_s);
                double vega = pv_up - pv;
                result.emplace(market_id, vega);
            }
        }

        if (riskType == "price") {
            // to be added

        }
    }
    else {
        auto pv_task = [](std::shared_ptr<Trade> trade, const std::string& id, const Market& mkt_up, const Market& mkt_down) {
            double pv_up = trade->Price(mkt_up);
            double pv_down = trade->Price(mkt_down);
            double risk = (pv_up - pv_down) / 2.0;
            return std::make_pair(id, risk);
            };

        std::vector<std::future<std::pair<std::string, double>>> _futures;
        // calling the above function asynchronously and storing the result in future object
        for (auto& shock : curveShocks) {
            string market_id = shock.first;
            auto mkt_u = shock.second.getMarketUp();
            auto mkt_d = shock.second.getMarketDown();
            _futures.push_back(std::async(std::launch::async, pv_task, trade, market_id, mkt_u, mkt_d));
        }

        for (auto& shock : volShocks) {
            const std::string& market_id = shock.first;
            const Market& mkt = shock.second.getOriginMarket();
            const Market& mkt_s = shock.second.getMarket();
            _futures.push_back(std::async(std::launch::async, pv_task, trade, market_id, mkt, mkt_s));
        }

        for (auto&& fut : _futures) {
            auto rs = fut.get();
            result.emplace(rs);
        }

    }
}