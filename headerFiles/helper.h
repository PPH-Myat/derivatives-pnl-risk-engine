#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <locale>
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <cmath>

#include "date.h"

namespace util {

    // ===========================
    // File I/O Utilities
    // ===========================

    inline void readFromFile(const std::string& fileName, std::string& header, std::vector<std::string>& output) {
        std::ifstream input_file(fileName);
        if (!input_file.is_open()) {
            std::cerr << "[ERROR] Could not open file: " << fileName << std::endl;
            return;
        }

        std::string line;
        if (getline(input_file, header)) {
            while (getline(input_file, line)) {
                output.push_back(line);
            }
        }

        input_file.close();
    }

    inline void outputToFile(const std::string& fileName, const std::vector<std::string>& output) {
        std::ofstream outFile(fileName);
        if (!outFile.is_open()) {
            std::cerr << "[ERROR] Could not write to file: " << fileName << std::endl;
            return;
        }

        for (const auto& line : output) {
            outFile << line << std::endl;
        }

        outFile.close();
    }

    inline bool fileExists(const std::string& filename) {
        std::ifstream f(filename);
        return f.good();
    }

    // ===========================
    // String Utilities
    // ===========================

    inline std::string to_lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](char c) { return std::tolower(static_cast<unsigned char>(c)); });
        return s;
    }

    inline std::string to_upper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](char c) { return std::toupper(static_cast<unsigned char>(c)); });
        return s;
    }

    inline std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
        std::vector<std::string> tokens;
        size_t start = 0, end;
        while ((end = s.find(delimiter, start)) != std::string::npos) {
            tokens.push_back(s.substr(start, end - start));
            start = end + delimiter.length();
        }
        tokens.push_back(s.substr(start));
        return tokens;
    }

    inline double safe_stod(const std::string& s, double fallback = 0.0) {
        try {
            return std::stod(s);
        }
        catch (...) {
            return fallback;
        }
    }

    inline int safe_stoi(const std::string& s, int fallback = 0) {
        try {
            return std::stoi(s);
        }
        catch (...) {
            return fallback;
        }
    }

    // ===========================
    // Schedule Generator
    // ===========================

    inline void genSchedule(double start, double end, double freq, std::vector<double>& schedule) {
        if (start >= end || freq <= 0 || freq > 1) {
            throw std::runtime_error("Invalid schedule parameters: start >= end or invalid freq");
        }

        double seed = end;
        while (seed > start) {
            schedule.push_back(seed);
            seed -= freq;
        }
        schedule.push_back(start);

        if (schedule.size() < 2) {
            throw std::runtime_error("Generated schedule is invalid (less than 2 dates)");
        }

        std::reverse(schedule.begin(), schedule.end());
    }

    // ===========================
    // Interpolation Utility
    // ===========================

    inline double linearInterp(double x0, double y0, double x1, double y1, double x) {
        if (x1 == x0) return y0;
        return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
    }

    // ===========================
    // Tenor/Frequency Helpers
    // ===========================

    inline double tenorToFrequency(const std::string& tenorStr) {
        std::string t = to_upper(tenorStr);
        if (t == "1Y") return 1.0;
        if (t == "6M") return 0.5;
        if (t == "3M") return 0.25;
        if (t == "1M") return 1.0 / 12.0;
        throw std::invalid_argument("Unknown tenor: " + tenorStr);
    }

    // ===========================
    // Date Helpers
    // ===========================

    inline Date parseDate(const std::string& s) {
        int y, m, d;
        if (sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d) != 3) {
            throw std::invalid_argument("Invalid date format: " + s);
        }
        return Date(y, m, d);
    }

    inline long getSerial(const Date& d) {
        return d.getSerialDate();
    }

    inline Date dateAddTenor(const Date& start, const std::string& tenorStr) {
        Date result = start;
        std::string t = to_upper(tenorStr);

        if (t == "ON" || t == "O/N") {
            result.serialToDate(start.getSerialDate() + 1);
            return result;
        }

        int number = std::stoi(t.substr(0, t.size() - 1));
        char unit = t.back();

        int y = start.getYear();
        int m = start.getMonth();
        int d = start.getDay();

        if (unit == 'Y') {
            y += number;
        }
        else if (unit == 'M') {
            m += number;
            while (m > 12) { m -= 12; y += 1; }
        }
        else if (unit == 'W') {
            result.serialToDate(start.getSerialDate() + number * 7);
            return result;
        }
        else {
            throw std::invalid_argument("Unsupported tenor: " + tenorStr);
        }

        // Correct day overflow for new month
        int daysInMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
        if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)))
            daysInMonth[1] = 29;

        if (d > daysInMonth[m - 1])
            d = daysInMonth[m - 1];

        return Date(y, m, d);
    }

    // ===========================
    // Debug Helper
    // ===========================

    template <typename T>
    inline void printVec(const std::vector<T>& vec, const std::string& label = "") {
        if (!label.empty()) std::cout << label << ": ";
        for (const auto& item : vec) {
            std::cout << item << " ";
        }
        std::cout << std::endl;
    }

} // namespace util