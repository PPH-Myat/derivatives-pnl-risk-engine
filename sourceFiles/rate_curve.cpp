#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>

#include "rate_curve.h"
#include "date.h"

using namespace std;

// ===== Helper: Trim leading and trailing whitespace from a string =====
static string trim(const string& str) {
    const auto strBegin = str.find_first_not_of(" \t\r\n");
    if (strBegin == string::npos) return "";  // no content

    const auto strEnd = str.find_last_not_of(" \t\r\n");
    return str.substr(strBegin, strEnd - strBegin + 1);
}

// ===== Constructors =====

// Default constructor uses default member initialization
RateCurve::RateCurve() = default;

// Constructor with curve name
RateCurve::RateCurve(const std::string& _name) : name(_name) {}

// ===== Add or update a rate at a given tenor =====
void RateCurve::addRate(const Date& tenor, double rate) {
    // Check if tenor already exists; if yes, overwrite the rate
    for (size_t i = 0; i < tenorDates.size(); ++i) {
        if (tenorDates[i] == tenor) {
            rates[i] = rate;
            return;
        }
    }
    // If tenor is new, append it with the rate
    tenorDates.push_back(tenor);
    rates.push_back(rate);
}

// ===== Get rate at a given tenor with linear interpolation =====
double RateCurve::getRate(const Date& tenor) const {
    if (tenorDates.empty())
        throw runtime_error("Rate curve is empty.");

    long targetSerial = tenor.getSerialDate();

    // Loop over tenor dates to find exact match or bracket for interpolation
    for (size_t i = 0; i < tenorDates.size(); ++i) {
        if (tenorDates[i] == tenor)
            return rates[i];

        // Interpolate if tenor lies between tenorDates[i-1] and tenorDates[i]
        if (tenorDates[i] > tenor && i > 0) {
            double x0 = tenorDates[i - 1].getSerialDate();
            double x1 = tenorDates[i].getSerialDate();
            double x = targetSerial;
            double r0 = rates[i - 1];
            double r1 = rates[i];

            // Linear interpolation formula
            return r0 + (r1 - r0) * (x - x0) / (x1 - x0);
        }
    }

    // If tenor beyond known points, return last known rate (flat extrapolation)
    return rates.back();
}

// ===== Get Discount Factor =====
double RateCurve::getDf(const Date& date) const {
    double r = getRate(date);
    double T = date.yearFraction(tenorDates.front());
    return std::exp(-r * T);
}

// ===== Apply Parallel Shock =====
void RateCurve::shock(double delta) {
    for (auto& r : rates) r += delta;
}

// ===== Shock a specific tenor date =====
void RateCurve::shock(const Date& tenor, double delta) {
    for (size_t i = 0; i < tenorDates.size(); ++i) {
        if (tenorDates[i] == tenor) {
            rates[i] += delta;
            return;
        }
    }
    std::cerr << "[WARN] Tenor date " << tenor << " not found in rate curve for shock." << std::endl;
}

// ===== Load curve data from a file =====
void RateCurve::loadFromFile(const std::string& filename, const Date& asOf) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "[ERROR] Could not open file: " << filename << endl;
        throw runtime_error("Cannot open curve file.");
    }

    // The first line is assumed to be the curve name
    getline(file, name);

    string line;
    while (getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == string::npos) continue;  // skip malformed lines

        string term = trim(line.substr(0, colon));         // tenor string (e.g. "3M")
        string rateStr = trim(line.substr(colon + 1));     // rate string (e.g. "5.56%")

        // Remove percent sign if present
        rateStr.erase(remove(rateStr.begin(), rateStr.end(), '%'), rateStr.end());

        try {
            // Convert rate string to double and convert from % to decimal
            double rate = stod(rateStr) / 100.0;

            // Convert tenor string to Date relative to asOf
            Date tenorDate = Date::fromTenor(term, asOf);

            // Add rate to curve
            addRate(tenorDate, rate);
        }
        catch (const std::invalid_argument&) {
            cerr << "[ERROR] Invalid rate: " << rateStr << " in line: " << line << endl;
            continue;  // skip invalid line but continue loading
        }
    }
}
// ===== Display curve data =====
void RateCurve::display() const {
    cout << "RateCurve: " << name << endl;
    for (size_t i = 0; i < tenorDates.size(); ++i)
        cout << tenorDates[i] << ": " << rates[i] << endl;
}
