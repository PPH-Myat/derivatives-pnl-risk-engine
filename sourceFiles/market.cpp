#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>

#include "market.h"

using namespace std;

// ===== Helper Functions =====

static string trim(const string& str) {
    auto start = str.find_first_not_of(" \t\r\n");
    auto end = str.find_last_not_of(" \t\r\n");
    if (start == string::npos || end == string::npos) return "";
    return str.substr(start, end - start + 1);
}

static string toUpper(const string& str) {
    string result = str;
    transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// ===== Constructors =====

Market::Market() {
    cout << "default market constructor is called by object@" << this << endl;
}

Market::Market(const Date& now) : asOf(now), name("test") {
    cout << "market constructor is called by object@" << this << endl;
}

Market::Market(const Market& other)
    : asOf(other.asOf), name(other.name),
    bondPrices(other.bondPrices), stockPrices(other.stockPrices) {
    for (const auto& kv : other.curves)
        curves[kv.first] = make_shared<RateCurve>(*kv.second);
    for (const auto& kv : other.vols)
        vols[kv.first] = make_shared<VolCurve>(*kv.second);
}

Market& Market::operator=(const Market& other) {
    if (this != &other) {
        asOf = other.asOf;
        name = other.name;
        bondPrices = other.bondPrices;
        stockPrices = other.stockPrices;
        curves.clear();
        vols.clear();
        for (const auto& kv : other.curves)
            curves[kv.first] = make_shared<RateCurve>(*kv.second);
        for (const auto& kv : other.vols)
            vols[kv.first] = make_shared<VolCurve>(*kv.second);
    }
    return *this;
}

Market::~Market() {
    //cout << "Market destructor is called" << endl;
}

// ===== Add Methods =====

void Market::addCurve(const string& Name, shared_ptr<RateCurve> curve) {
    curves[toUpper(Name)] = curve;
}

void Market::addVolCurve(const string& Name, shared_ptr<VolCurve> vol) {
    vols[toUpper(Name)] = vol;
}

void Market::addBondPrice(const string& bondName, double price) {
    bondPrices[toUpper(bondName)] = price;
}

void Market::addStockPrice(const string& stockName, double price) {
    stockPrices[toUpper(stockName)] = price;
}

// ===== Accessors =====

shared_ptr<RateCurve> Market::getCurve(const string& name) const {
    string key = toUpper(name);
    auto it = curves.find(key);
    if (it == curves.end()) {
        cerr << "[ERROR] Rate curve not found in market: " << key << endl;
        cerr << "Available rate curves are:\n";
        for (const auto& [k, _] : curves)
            cerr << "  - " << k << endl;
        throw runtime_error("Rate curve not found: " + key);
    }
    return it->second;
}

shared_ptr<VolCurve> Market::getVolCurve(const string& name) const {
    string key = toUpper(name);
    auto it = vols.find(key);
    if (it == vols.end()) {
        cerr << "[ERROR] Vol curve not found in market: " << key << endl;
        cerr << "Available vol curves are:\n";
        for (const auto& [k, _] : vols)
            cerr << "  - " << k << endl;
        throw runtime_error("Vol curve not found: " + key);
    }
    return it->second;
}

double Market::getStockPrice(const string& name) const {
    string key = toUpper(name);
    auto it = stockPrices.find(key);
    if (it == stockPrices.end())
        throw runtime_error("Stock price not found: " + key);
    return it->second;
}

double Market::getBondPrice(const string& name) const {
    string key = toUpper(name);
    auto it = bondPrices.find(key);
    if (it == bondPrices.end())
        throw runtime_error("Bond price not found: " + key);
    return it->second;
}

// ===== Shocks =====

void Market::shockPrice(const string& symbol, double bump) {
    string key = toUpper(symbol);
    auto it = stockPrices.find(key);
    if (it != stockPrices.end()) {
        it->second *= (1.0 + bump);
    }
    else {
        cerr << "[WARN] Market::shockPrice - Stock not found: " << key << endl;
    }
}

// ===== File Loaders =====

void Market::loadCurveFromFile(const string& filename) {
    auto curve = make_shared<RateCurve>();
    curve->loadFromFile(filename, asOf);
    curves[toUpper(curve->getName())] = curve;
}

void Market::loadVolFromFile(const string& filename) {
    auto vol = make_shared<VolCurve>();
    vol->loadFromFile(filename, asOf);
    vols[toUpper(vol->getName())] = vol;
}

void Market::loadStockPriceFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open())
        throw runtime_error("Cannot open stock price file: " + filename);

    string line;
    while (getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == string::npos) continue;
        string name = toUpper(trim(line.substr(0, colon)));
        string priceStr = trim(line.substr(colon + 1));
        try {
            double price = stod(priceStr);
            stockPrices[name] = price;
        }
        catch (...) {
            cerr << "[ERROR] Invalid stock price for " << name << ": " << priceStr << endl;
        }
    }
}

void Market::loadBondPriceFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open())
        throw runtime_error("Cannot open bond price file: " + filename);

    string line;
    while (getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == string::npos) continue;
        string name = toUpper(trim(line.substr(0, colon)));
        string priceStr = trim(line.substr(colon + 1));
        try {
            double price = stod(priceStr);
            bondPrices[name] = price;
        }
        catch (...) {
            cerr << "[ERROR] Invalid bond price for " << name << ": " << priceStr << endl;
        }
    }
}

// ===== Print =====

void Market::Print() const {
    cout << "Market as of: " << asOf << endl;

    cout << "--- Rate Curves ---" << endl;
    for (const auto& [_, curve] : curves)
        curve->display();

    cout << "--- Vol Curves ---" << endl;
    for (const auto& [_, vol] : vols)
        vol->display();

    cout << "--- Bond Prices ---" << endl;
    for (const auto& [k, v] : bondPrices)
        cout << k << ": " << v << endl;

    cout << "--- Stock Prices ---" << endl;
    for (const auto& [k, v] : stockPrices)
        cout << k << ": " << v << endl;
}

// ===== Stream Operators =====

ostream& operator<<(ostream& os, const Market& mkt) {
    os << mkt.asOf;
    return os;
}

istream& operator>>(istream& is, Market& mkt) {
    is >> mkt.asOf;
    return is;
}