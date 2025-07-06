#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <cmath>

#include "vol_curve.h"
#include "date.h"

using namespace std;

// ===== Helper: Trim leading/trailing whitespace from string =====
static string trim(const string& str) {
    const auto strBegin = str.find_first_not_of(" \t\r\n");
    if (strBegin == string::npos) return "";

    const auto strEnd = str.find_last_not_of(" \t\r\n");
    return str.substr(strBegin, strEnd - strBegin + 1);
}

// ===== Constructors =====
// Default constructor uses default member initializations
VolCurve::VolCurve() = default;
// Constructor setting the curve name
VolCurve::VolCurve(const std::string& _name) : name(_name) {}

// ===== Add a volatility point =====
void VolCurve::addVol(const Date& tenor, double vol) {
    tenors.push_back(tenor);
    vols.push_back(vol);
}

// ===== Get volatility at tenor with linear interpolation =====
double VolCurve::getVol(const Date& tenor) const {
    if (tenors.empty())
        throw runtime_error("Vol curve is empty.");

    long targetSerial = tenor.getSerialDate();

    // Search for exact tenor or interpolate between nearest points
    for (size_t i = 0; i < tenors.size(); ++i) {
        if (tenors[i] == tenor)
            return vols[i];
        if (tenors[i] > tenor && i > 0) {
            double x0 = tenors[i - 1].getSerialDate();
            double x1 = tenors[i].getSerialDate();
            double x = tenor.getSerialDate();
            double v0 = vols[i - 1];
            double v1 = vols[i];

            // Linear interpolation formula
            return v0 + (v1 - v0) * (x - x0) / (x1 - x0);
        }
    }

    // If tenor beyond known tenors, return last vol (flat extrapolation)
    return vols.back();
}
// ===== Shock Vols =====
void VolCurve::shock(double delta) {
    for (auto& v : vols) v += delta;
}

// ===== Shock a specific tenor by delta =====
void VolCurve::shock(const Date& tenor, double delta) {
    for (size_t i = 0; i < tenors.size(); ++i) {
        if (tenors[i] == tenor) {
            vols[i] += delta;
            return;
        }
    }
    std::cerr << "[WARN] VolCurve::shock - Tenor not found: " << tenor << std::endl;
}

// ===== Load curve data from file =====
void VolCurve::loadFromFile(const std::string& filename, const Date& asOf) {
    // Name is fixed for vol curve files (can be improved to parse from file)
    name = "VOL";

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "[ERROR] Could not open file: " << filename << endl;
        throw runtime_error("Cannot open curve file.");
    }

    string line;
    while (getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == string::npos) continue;  // Skip malformed lines

        string term = trim(line.substr(0, colon));
        string volStr = trim(line.substr(colon + 1));

        // Remove percent signs if present
        volStr.erase(remove(volStr.begin(), volStr.end(), '%'), volStr.end());

        // Convert tenor string to Date relative to asOf
        Date tenorDate = Date::fromTenor(term, asOf);

        try {
            // Parse volatility value and convert from % to decimal
            double vol = stod(volStr) / 100.0;
            addVol(tenorDate, vol);
        }
        catch (const std::invalid_argument&) {
            cerr << "[ERROR] Invalid volatility: " << volStr << " in line: " << line << endl;
            continue;  // Skip invalid line but keep loading
        }
    }
}
// ===== Display curve =====
void VolCurve::display() const {
    cout << "VolCurve: " << name << endl;
    for (size_t i = 0; i < tenors.size(); ++i)
        cout << tenors[i] << ": " << vols[i] << endl;
}