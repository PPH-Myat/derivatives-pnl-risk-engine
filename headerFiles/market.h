#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>
#include "date.h"
#include "rate_curve.h"
#include "vol_curve.h"

class Market {
public:
    Date asOf;
    std::string name;

    Market();
    explicit Market(const Date& now);
    Market(const Market& other);             // Deep copy constructor
    Market& operator=(const Market& other);  // Copy assignment
    ~Market();

    // Add or update
    void addCurve(const std::string& name, std::shared_ptr<RateCurve> curve);
    void addVolCurve(const std::string& name, std::shared_ptr<VolCurve> vol);
    void addBondPrice(const std::string& bondName, double price);
    void addStockPrice(const std::string& stockName, double price);

    // Accessors
    std::shared_ptr<RateCurve> getCurve(const std::string& name) const;
    std::shared_ptr<VolCurve> getVolCurve(const std::string& name) const;
    double getStockPrice(const std::string& stockName) const;
    double getBondPrice(const std::string& bondName) const;
    const Date& getAsOf() const { return asOf; };
    void shockPrice(const std::string& symbol, double bump);


    // File loaders
    void loadCurveFromFile(const std::string& filename);
    void loadVolFromFile(const std::string& filename);
    void loadStockPriceFromFile(const std::string& filename);
    void loadBondPriceFromFile(const std::string& filename);

    void Print() const;

private:
    std::unordered_map<std::string, std::shared_ptr<RateCurve>> curves;
    std::unordered_map<std::string, std::shared_ptr<VolCurve>> vols;
    std::unordered_map<std::string, double> bondPrices;
    std::unordered_map<std::string, double> stockPrices;
};

std::ostream& operator<<(std::ostream& os, const Market& obj);
std::istream& operator>>(std::istream& is, Market& obj);
