#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <ctime>
#include <cmath>
#include <cctype>
#include <algorithm>

#include "date.h"

// ===== Utility Functions =====
static bool isLeapYear(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

static int daysInMonth(int year, int month) {
    static const int daysPerMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month == 2 && isLeapYear(year)) return 29;
    return daysPerMonth[month - 1];
}

// ===== Constructors & Setters =====
Date::Date() : year(0), month(0), day(0) {}

Date::Date(int y, int m, int d) {
    setYear(y);
    setMonth(m);
    setDay(d);
    if (!isValid()) throw std::invalid_argument("Invalid date constructed");
}

int Date::getYear() const { return year; }
int Date::getMonth() const { return month; }
int Date::getDay() const { return day; }

void Date::setYear(int y) { year = y; }
void Date::setMonth(int m) { month = m; }
void Date::setDay(int d) { day = d; }

// ===== Static Date Generators =====
Date Date::today() {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    return Date(now->tm_year + 1900, now->tm_mon + 1, now->tm_mday);
}

bool Date::isValid() const {
    if (year < 1900 || month < 1 || month > 12 || day < 1) return false;
    return day <= daysInMonth(year, month);
}

// ===== Date Arithmetic =====
std::tm Date::toTm() const {
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    return tm;
}

Date Date::addDays(int days) const {
    std::tm tm = toTm();
    time_t time = mktime(&tm);
    if (time == -1) throw std::runtime_error("Invalid date for mktime");

    time += static_cast<time_t>(days) * 86400;
    std::tm* newTm = localtime(&time);
    if (!newTm) throw std::runtime_error("localtime returned nullptr");

    return Date(newTm->tm_year + 1900, newTm->tm_mon + 1, newTm->tm_mday);
}

Date Date::addMonths(int months) const {
    int newMonth = month + months;
    int y = year + (newMonth - 1) / 12;
    int m = (newMonth - 1) % 12 + 1;

    int maxDay = daysInMonth(y, m);
    int newDay = std::min(day, maxDay);
    return Date(y, m, newDay);
}

Date Date::addYears(int yearsToAdd) const {
    int y = year + yearsToAdd;
    if (month == 2 && day == 29 && !isLeapYear(y)) {
        return Date(y, 2, 28);
    }
    return Date(y, month, day);
}

int Date::diffDays(const Date& other) const {
    std::tm t1 = this->toTm();
    std::tm t2 = other.toTm();
    time_t time1 = mktime(&t1);
    time_t time2 = mktime(&t2);

    if (time1 == -1 || time2 == -1) {
        throw std::runtime_error("Invalid date for day difference calculation");
    }

    double diffSeconds = difftime(time1, time2);
    return static_cast<int>(diffSeconds / 86400.0);
}

// ===== Year Fraction Calculation =====
double Date::yearFraction(const Date& other) const {
    return static_cast<double>(this->diffDays(other)) / 365.0; // Actual/365
}

// ===== Excel Serial Date Logic =====
long Date::getSerialDate() const {
    int daysSinceEpoch = 0;
    for (int y = 1900; y < year; ++y)
        daysSinceEpoch += isLeapYear(y) ? 366 : 365;

    static const int daysPerMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    bool leap = isLeapYear(year);
    for (int m = 1; m < month; ++m)
        daysSinceEpoch += (m == 2 && leap) ? 29 : daysPerMonth[m - 1];

    daysSinceEpoch += day;
    if (year > 1900) daysSinceEpoch += 1;

    return daysSinceEpoch;
}

void Date::serialToDate(int serial) {
    int daysSinceEpoch = serial - 2;
    int y = 1900, m = 1;
    static const int monthDays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

    while (true) {
        int daysInYear = isLeapYear(y) ? 366 : 365;
        if (daysSinceEpoch < daysInYear) break;
        daysSinceEpoch -= daysInYear;
        ++y;
    }

    while (true) {
        int dim = (m == 2 && isLeapYear(y)) ? 29 : monthDays[m - 1];
        if (daysSinceEpoch < dim) break;
        daysSinceEpoch -= dim;
        ++m;
    }

    year = y;
    month = m;
    day = daysSinceEpoch + 1;
}

// ===== Tenor Parsing =====
Date Date::fromTenor(const std::string& rawTenor, const Date& asOf) {
    std::string tenorStr;
    for (char ch : rawTenor) {
        if (std::isprint(static_cast<unsigned char>(ch)) && !std::isspace(static_cast<unsigned char>(ch))) {
            tenorStr += std::toupper(ch);
        }
    }

    if (tenorStr == "ON") return asOf.addDays(1);
    if (tenorStr == "TN") return asOf.addDays(2);
    if (tenorStr == "SN") return asOf.addDays(3);
    if (tenorStr == "SP") return asOf;

    if (tenorStr.empty()) {
        throw std::invalid_argument("Empty tenor string");
    }

    char unit = tenorStr.back();
    std::string numberPart = tenorStr.substr(0, tenorStr.size() - 1);

    int value;
    try {
        value = std::stoi(numberPart);
    }
    catch (...) {
        throw std::invalid_argument("Invalid tenor: " + rawTenor);
    }

    switch (unit) {
    case 'D': return asOf.addDays(value);
    case 'W': return asOf.addDays(7 * value);
    case 'M': return asOf.addMonths(value);
    case 'Y': return asOf.addYears(value);
    default:
        throw std::invalid_argument("Unknown tenor unit: " + std::string(1, unit));
    }
}

// ===== Operator Overloads =====
double operator-(const Date& d1, const Date& d2) {
    return d1.yearFraction(d2);
}

bool operator==(const Date& lhs, const Date& rhs) {
    return lhs.getYear() == rhs.getYear() &&
        lhs.getMonth() == rhs.getMonth() &&
        lhs.getDay() == rhs.getDay();
}

bool operator<(const Date& lhs, const Date& rhs) {
    if (lhs.getYear() != rhs.getYear()) return lhs.getYear() < rhs.getYear();
    if (lhs.getMonth() != rhs.getMonth()) return lhs.getMonth() < rhs.getMonth();
    return lhs.getDay() < rhs.getDay();
}

bool operator>(const Date& lhs, const Date& rhs) {
    return rhs < lhs;
}

bool operator<=(const Date& lhs, const Date& rhs) {
    return !(rhs < lhs);
}

bool operator>=(const Date& lhs, const Date& rhs) {
    return !(lhs < rhs);
}

// ===== Stream Operators =====
std::ostream& operator<<(std::ostream& os, const Date& date) {
    os << std::setfill('0') << std::setw(4) << date.getYear() << "-"
        << std::setw(2) << date.getMonth() << "-"
        << std::setw(2) << date.getDay();
    return os;
}

std::istream& operator>>(std::istream& is, Date& date) {
    int y, m, d;
    char dash1, dash2;

    if (!(is >> y >> dash1 >> m >> dash2 >> d) || dash1 != '-' || dash2 != '-') {
        is.setstate(std::ios::failbit);
        return is;
    }

    if (m < 1 || m > 12 || d < 1 || d > 31) {
        is.setstate(std::ios::failbit);
        return is;
    }

    date.setYear(y);
    date.setMonth(m);
    date.setDay(d);

    if (!date.isValid()) {
        is.setstate(std::ios::failbit);
    }

    return is;
}