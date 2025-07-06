#pragma once

#include <iostream>
#include <string>
#include <ctime>

// ===========================
// Date Class
// ===========================
class Date {
private:
    int year;   // Year (e.g., 2025)
    int month;  // Month (1-12)
    int day;    // Day (1-31)

public:
    // === Constructors ===
    Date();                             // Default constructor
    Date(int y, int m, int d);          // Construct from year, month, day

    // === Accessors ===
    int getYear() const { return year; }
    int getMonth() const { return month; }
    int getDay() const { return day; }

    // === Mutators ===
    void setYear(int y);
    void setMonth(int m);
    void setDay(int d);

    // === Validation ===
    bool isValid() const;               // Check if the date is valid (e.g., no Feb 30)

    // === Utilities ===
    std::tm toTm() const;               // Convert to std::tm structure
    int diffDays(const Date& other) const;  // Difference in days (this - other)
    double yearFraction(const Date& other) const; // Actual/365 year fraction
    long getSerialDate() const;         // Excel-style serial date
    void serialToDate(int serial);      // Convert from serial to date

    // === Date Arithmetic ===
    Date addDays(int days) const;       // Add N calendar days
    Date addMonths(int months) const;   // Add N calendar months
    Date addYears(int years) const;     // Add N calendar years

    static Date fromTenor(const std::string& tenorStr, const Date& asOf); // From tenor like "3M"
    static Date today();                // Get system date (today)

    // === Operators ===

    // Year fraction using ACT/365 convention (d1 - d2)
    friend double operator-(const Date& d1, const Date& d2);

    // Comparison operators
    friend bool operator==(const Date& lhs, const Date& rhs);
    friend bool operator<(const Date& lhs, const Date& rhs);
    friend bool operator>(const Date& lhs, const Date& rhs);
    friend bool operator<=(const Date& lhs, const Date& rhs);
    friend bool operator>=(const Date& lhs, const Date& rhs);

    // Stream operators
    friend std::ostream& operator<<(std::ostream& os, const Date& date); // Output as "YYYY-MM-DD"
    friend std::istream& operator>>(std::istream& is, Date& date);       // Input as "YYYY-MM-DD"
};