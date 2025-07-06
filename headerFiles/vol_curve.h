#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <memory>

#include "date.h"

// ===========================
// VolCurve Class
// ===========================
class VolCurve {
public:
    VolCurve();                                   // Default constructor

    explicit VolCurve(const std::string& _name); // Constructor that sets curve name

    // Add or update volatility at a given tenor date
    void addVol(const Date& tenor, double vol);

    // Retrieve volatility for a given tenor (with interpolation)
    double getVol(const Date& date) const;

    void shock(double delta);                // parallel shock all vols

    // Load curve data from file relative to a valuation date (asOf)
    void loadFromFile(const std::string& filename, const Date& asOf);
    // Display the curve for debugging or logging
    void display() const;
    // Get the name of this volatility curve
    std::string getName() const { return name; }

private:
    std::string name;           // Name of the volatility curve

    // Parallel vectors holding tenor dates and volatilities
    std::vector<Date> tenors;
    std::vector<double> vols;
};

