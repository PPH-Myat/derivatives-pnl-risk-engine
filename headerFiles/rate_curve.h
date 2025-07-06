#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "date.h"

// ===========================
// RateCurve Class
// ===========================
class RateCurve {
public:
    RateCurve();                                 // Default constructor
    explicit RateCurve(const std::string& _name);  // Constructor with curve name

    // Add a (tenor date, rate) point to the curve
    void addRate(const Date& tenor, double rate);
    void shock(double delta);                         // parallel shock
    void shock(const Date& tenor, double delta);      // point shock

	// Retrieve the rate corresponding to a given tenor date (with interpolation)
    double getRate(const Date& tenor) const;
    double getDf(const Date& date) const;    // DF = exp(-r * T), T in years

    // Load curve data from file using asOf date for relative tenor calculation
    void loadFromFile(const std::string& filename, const Date& asOf);
    // Display the curve contents for debugging or logging
    void display() const;
    // Get the curve name identifier (e.g., "USD-SOFR")
    std::string getName() const { return name; }


private:
    std::string name;               // Name of the curve

    // Parallel vectors storing tenor dates and their corresponding interest rates
    std::vector<Date> tenorDates;
    std::vector<double> rates;
};
