// main.cpp
#include <fstream>
#include <chrono>
#include <ctime>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "market.h"
#include "tree_pricer.h"
#include "risk_engine.h"
#include "factory.h"
#include "helper.h"

using namespace std;
using namespace util;

const string basePath = "../../../resourceFiles/";

struct TradeResult {
    size_t id = 0;
    string tradeInfo;
    double PV = 0.0;
    double DV01 = 0.0;
    double Vega = 0.0;
};

// ========== Load Trades ==========
void loadTrade(vector<shared_ptr<Trade>>& portfolio) {
    string header;
    vector<string> lines;
    readFromFile(basePath + "trade.txt", header, lines);

    cout << "[INFO] Loading trades..." << endl;
    for (size_t i = 0; i < lines.size(); ++i) {
        auto t = split(lines[i], ";");
        if (t.size() < 12) {
            cerr << "[WARN] Skipping malformed line: " << lines[i] << endl;
            continue;
        }

        try {
            string type = to_lower(t[1]);
            Date tradeDate = parseDate(t[2]);
            Date startDate = parseDate(t[3]);
            Date endDate = parseDate(t[4]);
            double notional = stod(t[5]);
            string underlying = t[6];
            if (underlying == "SGD-MAS-BILL") underlying = "SGD-SORA";
            double rate = stod(t[7]);
            double strike = stod(t[8]);
            double freq = stod(t[9]);
            string optionStr = to_lower(t[10]);
            string direction = to_lower(t[11]);

            OptionType optType = OptionType::None;
            if (optionStr == "call") optType = OptionType::Call;
            else if (optionStr == "put") optType = OptionType::Put;

            bool isLong = (direction == "long");

            shared_ptr<Trade> trade;
            if (type == "bond")
                trade = BondFactory().createTrade(underlying, startDate, endDate, notional, rate, freq, optType);
            else if (type == "swap")
                trade = SwapFactory().createTrade(underlying, startDate, endDate, notional, rate, freq, optType);
            else if (type == "european")
                trade = make_shared<EuropeanOption>(optType, notional, strike, tradeDate, endDate, underlying);
            else if (type == "american")
                trade = make_shared<AmericanOption>(optType, notional, strike, tradeDate, endDate, underlying);

            if (trade) {
                trade->setLong(isLong);
                portfolio.push_back(trade);
                cout << "[OK] Loaded trade " << i + 1 << ": " << trade->getType() << " " << underlying << endl;
            }
        }
        catch (const exception& e) {
            cerr << "[ERROR] Parsing failed: " << lines[i] << " => " << e.what() << endl;
        }
    }
}

// ========== Load Curves ==========
void loadIrCurve(Market& mkt, const string& fileName, const string& curveName) {
    auto curve = make_shared<RateCurve>(curveName);
    string header;
    vector<string> lines;
    readFromFile(basePath + fileName, header, lines);
    Date asOf = mkt.asOf;

    for (const auto& line : lines) {
        auto parts = split(line, ":");
        if (parts.size() < 2) continue;
        Date tenorDate = dateAddTenor(asOf, parts[0]);
        double rate = stod(parts[1]) / 100.0;
        curve->addRate(tenorDate, rate);
    }

    mkt.addCurve(curveName, curve);
}

void loadVolCurve(Market& mkt, const string& fileName, const string& curveName) {
    auto vol = make_shared<VolCurve>(curveName);
    string header;
    vector<string> lines;
    readFromFile(basePath + fileName, header, lines);
    Date asOf = mkt.asOf;

    for (const auto& line : lines) {
        auto parts = split(line, ":");
        if (parts.size() < 2) continue;
        Date tenorDate = dateAddTenor(asOf, parts[0]);
        double v = stod(parts[1]) / 100.0;
        vol->addVol(tenorDate, v);
    }

    mkt.addVolCurve(curveName, vol);
}

// ========== Output ==========
void outPutResult(const vector<TradeResult>& results) {
    vector<string> output;
    for (const auto& r : results) {
        output.push_back(to_string(r.id) + "; " + r.tradeInfo +
            "; PV:" + to_string(r.PV) +
            "; Delta:" + to_string(r.DV01) +
            "; Vega:" + to_string(r.Vega));
    }
    outputToFile("output.txt", output);
}

void readAndPrintOutput(const string& filePath) {
    ifstream in(filePath);
    if (!in.is_open()) {
        cerr << "[ERROR] Cannot open output file.\n";
        return;
    }

    cout << "\n============ Output File: " << filePath << " ============\n";
    string line;
    while (getline(in, line)) {
        cout << line << endl;
    }
    cout << "==========================================================\n" << endl;
}

// ========== Main ==========
int main() {
    time_t t = chrono::system_clock::to_time_t(chrono::system_clock::now());
    tm localTime;
    localtime_s(&localTime, &t);
    Date valueDate(localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday);

    auto mkt = make_shared<Market>(valueDate);
    loadIrCurve(*mkt, "usd_curve.txt", "USD-SOFR");
    loadIrCurve(*mkt, "sgd_curve.txt", "SGD-SORA");
    mkt->addCurve("USD-GOV", mkt->getCurve("USD-SOFR"));
    mkt->addCurve("SGD-GOV", mkt->getCurve("SGD-SORA"));
    loadVolCurve(*mkt, "vol.txt", "LOGVOL");

    mkt->addStockPrice("APPL", 652.0);
    mkt->addStockPrice("SP500", 5035.7);
    mkt->addStockPrice("STI", 3420.0);

    vector<shared_ptr<Trade>> portfolio;
    loadTrade(portfolio);

    auto pricer = make_shared<CRRBinomialTreePricer>(50);
    vector<TradeResult> results;

    double curve_shock = 0.0001, vol_shock = 0.01;

    for (size_t i = 0; i < portfolio.size(); ++i) {
        auto& trade = portfolio[i];
        TradeResult r;
        r.id = i + 1;
        r.tradeInfo = trade->getType() + " " + trade->getUnderlying();
        r.PV = pricer->price(*mkt, trade);

        RiskEngine engine(*mkt, curve_shock, vol_shock, 0.0);
        engine.computeRisk("dv01", trade, true);
        for (const auto& [_, v] : engine.getResult())
            r.DV01 += v / (2.0 * curve_shock);

        engine.computeRisk("vega", trade, true);
        for (const auto& [_, v] : engine.getResult())
            r.Vega += v / vol_shock;

        results.push_back(r);
    }

    outPutResult(results);
    cout << "Pricing and risk completed. Results written to output.txt\n";
    readAndPrintOutput("output.txt");
    return 0;
}