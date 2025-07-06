#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <chrono>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <iomanip>

#include "date.h"
#include "market.h"
#include "tree_pricer.h"
#include "european_trade.h"
#include "bond.h"
#include "swap.h"
#include "american_trade.h"
#include "black_scholes_pricer.h"

using namespace std;

// ===== Helper: Parse YYYY-MM-DD string to Date =====
Date parseDate(const string& s) {
    int y, m, d;
    if (sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d) != 3) {
        throw invalid_argument("Invalid date format: " + s);
    }
    return Date(y, m, d);
}

// ===== Task 2: Read trades from file and build Trade portfolio =====
void createPortfolioFromFile(const string& filename, vector<Trade*>& portfolio) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "[ERROR] Cannot open file: " << filename << endl;
        return;
    }

    string line;
    getline(file, line); // skip header

    while (getline(file, line)) {
        istringstream ss(line);
        string field;
        vector<string> fields;
        while (getline(ss, field, ';')) {
            fields.push_back(field);
        }

        if (fields.size() < 11) {
            cerr << "[WARN] Skipping malformed line: " << line << endl;
            continue;
        }

        try {
            string type = fields[1];
            Date tradeDate = parseDate(fields[2]);
            Date startDate = parseDate(fields[3]);
            Date endDate = parseDate(fields[4]);
            double notional = stod(fields[5]);
            string instrument = fields[6];
            double rate = stod(fields[7]);
            double strike = stod(fields[8]);
            double freqFraction = stod(fields[9]);
            string optType = fields[10];

            if (freqFraction <= 0.0) throw runtime_error("Invalid frequency fraction");
            int freq = static_cast<int>(round(1.0 / freqFraction));
            transform(instrument.begin(), instrument.end(), instrument.begin(), ::toupper);

            if (type == "swap") {
                portfolio.push_back(new Swap(tradeDate, startDate, endDate, notional, rate, freq));
            }
            else if (type == "bond") {
                portfolio.push_back(new Bond(instrument, tradeDate, startDate, endDate,
                    notional, freq, 100.0, rate));
            }
            else if (type == "european") {
                OptionType ot = (optType == "call") ? OptionType::Call : OptionType::Put;
                portfolio.push_back(new EuropeanOption(instrument, tradeDate, endDate, strike, ot));
            }
            else if (type == "american") {
                OptionType ot = (optType == "call") ? OptionType::Call : OptionType::Put;
                portfolio.push_back(new AmericanOption(instrument, tradeDate, endDate, strike, ot));
            }
        }
        catch (const exception& e) {
            cerr << "[ERROR] Trade line skipped: " << line << endl << "Reason: " << e.what() << endl;
        }
    }

    file.close();
}

int main() {
    // ===== Task 1: Set valuation date =====
    Date valueDate;
    time_t t = time(nullptr);
    struct tm timeInfo;
    if (localtime_s(&timeInfo, &t) == 0) {
        valueDate.setYear(timeInfo.tm_year + 1900);
        valueDate.setMonth(timeInfo.tm_mon + 1);
        valueDate.setDay(timeInfo.tm_mday);
    }
    cout << "Valuation Date: " << valueDate << endl;

    // ===== Market object creation and copy test =====
    Market mkt0;
    Market mkt1(valueDate);
    Market mkt2(mkt1);
    Market mkt3;
    mkt3 = mkt2;

    // ===== Load market data from files =====
    const string basePath = "../../../resourceFiles/";
    mkt1.loadCurveFromFile(basePath + "curve.txt");
    mkt1.loadVolFromFile(basePath + "vol.txt");
    mkt1.loadStockPriceFromFile(basePath + "stockPrice.txt");
    mkt1.loadBondPriceFromFile(basePath + "bondPrice.txt");

    // ===== Task 2: Build portfolio =====
    vector<Trade*> myPortfolio;

    // Option A: Build portfolio from trade.txt file
    //createPortfolioFromFile(basePath + "trade.txt", myPortfolio);
    
    // Option B: Manual portfolio creation
 
    // ===== [1] Bond =====
    Date bondMaturity = valueDate.addYears(5);
    double bondNotional = 100000.0;
    int bondCouponFreq = 2;       // Semi-annual
    double bondTradePrice = 101.5;
    double bondCouponRate = 0.025; // 2.5%
    string bondName = "SGD-GOV";

    myPortfolio.push_back(new Bond(
        bondName,
        valueDate,        // tradeDate
        valueDate,        // startDate
        bondMaturity,
        bondNotional,
        bondCouponFreq,
        bondTradePrice,
        bondCouponRate
    ));

    // ===== [2] Swap =====
    Date swapMaturity = valueDate.addYears(5);
    double swapNotional = 1000000.0;
    double fixedRate = 0.045;
    int swapCouponFreq = 2;

    myPortfolio.push_back(new Swap(
        valueDate,        // tradeDate
        valueDate,        // startDate
        swapMaturity,
        swapNotional,
        fixedRate,
        swapCouponFreq
    ));

    // ===== [3] European Option =====
    string stockTicker = "AAPL";
    double spotPrice = mkt1.getStockPrice(stockTicker);
    Date optionExpiry = valueDate.addMonths(6);
    double strikeCall = spotPrice * 0.95;

    myPortfolio.push_back(new EuropeanOption(
        stockTicker,
        valueDate,        // tradeDate
        optionExpiry,
        strikeCall,
        OptionType::Call
    ));

    // ===== [4] American Option =====
    myPortfolio.push_back(new AmericanOption(
        stockTicker,
        valueDate,        // tradeDate
        optionExpiry,
        strikeCall,
        OptionType::Call
    ));

    // ===== Task 3: price portfolio (using CRR binomial tree for options) =====
    Pricer* treePricer = new CRRBinomialTreePricer(3);

    ofstream log("pricing_result.txt");
    if (!log.is_open()) {
        cerr << "[ERROR] Cannot open pricing_result.txt for writing." << endl;
        return 1;
    }

    cout << endl << "===== [Task 3] Portfolio Pricing Results =====" << endl;

    // Formatted header
    ostringstream header;
    header << left << setw(15) << "Trade Type" << "| "
        << left << setw(15) << "Underlying" << "| "
        << right << setw(10) << "PV" << " | "
        << right << setw(10) << "payoff(NPV)";
    cout << header.str() << endl;

    cout << string(60, '-') << endl;

    for (auto* trade : myPortfolio) {
        try {
            double pv = treePricer->price(mkt1, trade);
            double payoff = trade->payoff(mkt1);

            string type = trade->getType();
            string underlying = trade->getUnderlying();

            // Clean string
            underlying.erase(remove_if(underlying.begin(), underlying.end(),
                [](char c) { return !isalnum(c) && c != '-' && c != '_'; }), underlying.end());

            // Format row
            ostringstream line;
            line << left << setw(15) << type << "| "
                << left << setw(15) << underlying << "| "
                << right << setw(10) << fixed << setprecision(2) << pv << " | "
                << right << setw(10) << fixed << setprecision(2) << payoff;

            cout << line.str() << endl;

            log << "Trade Type: " << type
                << ", Underlying: " << underlying
                << ", PV: " << pv
                << ", payoff: " << payoff << endl;
        }
        catch (const exception& e) {
            cerr << "[ERROR] Pricing failed for " << trade->getType()
                << ": " << e.what() << endl;
        }
    }

    cout << string(60, '-') << endl;

    log.close();

    // ===== Task 4: Analyze pricing results =====
    cout << "\n===== [Task 4] Analyzing Pricing Results =====" << endl;

    EuropeanOption* euro = nullptr;
    for (auto* trade : myPortfolio) {
        if (trade->getType() == "EuropeanOption") {
            euro = dynamic_cast<EuropeanOption*>(trade);
            break;
        }
    }

	// Comparison between Binomial Tree and Black-Scholes for European options
    if (euro) {
        BlackScholesPricer bsPricer;
        double pvTree = treePricer->price(mkt1, euro);
        double pvBS = bsPricer.price(mkt1, euro);

        cout << "\n--- [Comparison A] Binomial Tree vs Black-Scholes ---" << endl;
        cout << "European Option | Underlying: " << euro->getUnderlying()
            << ", Strike: " << euro->getStrike()
            << ", Expiry: " << euro->getExpiry() << endl;
        cout << "  Binomial Tree PV = " << fixed << setprecision(4) << pvTree << endl;
        cout << "  Black-Scholes PV = " << fixed << setprecision(4) << pvBS << endl;
    }
    else {
        cout << "[WARN] No European option found for comparison." << endl;
    }

	//Comparison tween American and European options
    bool callDone = false, putDone = false;

    for (Trade* e : myPortfolio) {
        if (e->getType() != "EuropeanOption") continue;
        auto* eOpt = dynamic_cast<EuropeanOption*>(e);
        if (!eOpt) continue;

        for (Trade* a : myPortfolio) {
            if (a->getType() != "AmericanOption") continue;
            auto* aOpt = dynamic_cast<AmericanOption*>(a);
            if (!aOpt) continue;

            bool sameStrike = std::fabs(eOpt->getStrike() - aOpt->getStrike()) < 1e-6;
            bool sameExpiry = eOpt->getExpiry() == aOpt->getExpiry();
            bool sameUnderlying = eOpt->getUnderlying() == aOpt->getUnderlying();
            bool sameType = eOpt->getOptionType() == aOpt->getOptionType();

            if (sameStrike && sameExpiry && sameUnderlying && sameType) {
                double pvEuro = treePricer->price(mkt1, eOpt);
                double pvAmer = treePricer->price(mkt1, aOpt);
                string optLabel = (eOpt->getOptionType() == OptionType::Call) ? "Call" : "Put";

                cout << "\n--- [Comparison B] " << optLabel << " Option (American vs European) ---" << endl;
                cout << "Underlying: " << eOpt->getUnderlying()
                    << ", Strike: " << eOpt->getStrike()
                    << ", Expiry: " << eOpt->getExpiry() << endl;
                cout << "  European PV = " << fixed << setprecision(4) << pvEuro << endl;
                cout << "  American PV = " << fixed << setprecision(4) << pvAmer << endl;

                if (optLabel == "Call") callDone = true;
                if (optLabel == "Put") putDone = true;

                break;  // break inner loop: one match is enough
            }
        }

        if (callDone && putDone) break;  // break outer loop if both found
    }

    // ===== Cleanup =====
    for (auto* trade : myPortfolio) delete trade;
    myPortfolio.clear();
    delete treePricer;

    cout << "\nProject build successfully!" << endl;
    return 0;
}
