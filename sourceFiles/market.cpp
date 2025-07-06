#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>  // for transform
#include <stdexcept>  // for runtime_error

#include "market.h"

using namespace std;

// ===== Helper functions =====

// Trim leading and trailing whitespace characters from a string
static string trim(const string& str) {
    auto start = str.find_first_not_of(" \t\r\n");
    auto end = str.find_last_not_of(" \t\r\n");
    if (start == string::npos || end == string::npos) return "";
    return str.substr(start, end - start + 1);
}

// Convert a string to uppercase for consistent key storage and lookup
static string toUpper(const string& str) {
    string result = str;
    transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// ===== Constructors =====

// Default constructor
Market::Market() {
    cout << "default market constructor is called by object@" << this << endl;
    // name is default-initialized string, no manual action needed
}

// Constructor with Date
Market::Market(const Date& now) : asOf(now), name("test") {
    cout << "market constructor is called by object@" << this << endl;
}

// Copy constructor
Market::Market(const Market& other) : asOf(other.asOf), name(other.name),
    bondPrices(other.bondPrices), stockPrices(other.stockPrices) {
    for (const auto& kv : other.curves)
        curves[kv.first] = std::make_shared<RateCurve>(*kv.second);
    for (const auto& kv : other.vols)
        vols[kv.first] = std::make_shared<VolCurve>(*kv.second);
}

// Copy assignment operator
Market& Market::operator=(const Market& other) {
    cout << "assignment constructor is called by object@" << this << endl;
    if (this != &other) {
        asOf = other.asOf;
        name = other.name;
        bondPrices = other.bondPrices;
        stockPrices = other.stockPrices;
        curves.clear();
        vols.clear();
        for (const auto& kv : other.curves)
            curves[kv.first] = std::make_shared<RateCurve>(*kv.second);
        for (const auto& kv : other.vols)
            vols[kv.first] = std::make_shared<VolCurve>(*kv.second);
    }
    return *this;
}

// Destructor
Market::~Market() {
    cout << "Market destructor is called" << endl;
}

// ===== Add data methods =====
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
    auto it = curves.find(toUpper(name));
    if (it == curves.end()) throw runtime_error("Rate curve not found: " + name);
    return it->second;
}

shared_ptr<VolCurve> Market::getVolCurve(const string& name) const {
    auto it = vols.find(toUpper(name));
    if (it == vols.end()) throw runtime_error("Vol curve not found: " + name);
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

// ===== Shock a stock price by percentage bump =====
void Market::shockPrice(const std::string& symbol, double bump) {
    std::string key = toUpper(symbol);
    auto it = stockPrices.find(key);
    if (it != stockPrices.end()) {
        it->second *= (1.0 + bump);
    }
    else {
        std::cerr << "[WARN] Market::shockPrice - Stock not found: " << key << std::endl;
    }
}

// ===== File loading methods =====

void Market::loadCurveFromFile(const string& filename) {
    auto curve = std::make_shared<RateCurve>();
    curve->loadFromFile(filename, asOf);
    curves[toUpper(curve->getName())] = curve;
}

void Market::loadVolFromFile(const string& filename) {
    auto vol = std::make_shared<VolCurve>();
    vol->loadFromFile(filename, asOf);
    vols[toUpper(vol->getName())] = vol;
}

void Market::loadStockPriceFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) throw runtime_error("Cannot open stock price file: " + filename);

    string line;
    while (getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == string::npos) continue;  // skip malformed lines

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
    if (!file.is_open()) throw runtime_error("Cannot open bond price file: " + filename);

    string line;
    while (getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == string::npos) continue;  // skip malformed lines

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

// ===== Display market data =====
void Market::Print() const {
    cout << "market asof: " << asOf << endl;

    cout << "--- Rate Curves ---" << endl;
    for (const auto& curve : curves)
        curve.second->display();

    cout << "--- Volatility Curves ---" << endl;
    for (const auto& vol : vols)
        vol.second->display();

    cout << "--- Bond Prices ---" << endl;
    for (const auto& bond : bondPrices)
        cout << bond.first << ": " << bond.second << endl;

    cout << "--- Stock Prices ---" << endl;
    for (const auto& stock : stockPrices)
        cout << stock.first << ": " << stock.second << endl;
}

// ===== Stream operators =====

ostream& operator<<(ostream& os, const Market& mkt) {
    os << mkt.asOf << endl;
    return os;
}

istream& operator>>(istream& is, Market& mkt) {
    is >> mkt.asOf;
    return is;
}