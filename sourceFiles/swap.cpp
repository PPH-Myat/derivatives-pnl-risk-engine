#include "trade.h"
#include "swap.h"
#include "market.h"
#include "helper.h"

#include <iostream>
#include <stdexcept>
#include <cmath>
#include <sstream>

using util::to_upper;
using util::dateAddTenor;

Swap::Swap(std::string name,
    Date start,
    Date end,
    double _notional,
    double _rate,
    double _freq)
{
    tradeType = "Swap";
    underlying = to_upper(name);
    startDate = start;
    maturityDate = end;
    tradeDate = start;
    notional = _notional;
    tradeRate = _rate;
    frequency = _freq;
    rateCurve = to_upper(name);  // e.g., "USD-SOFR"
    isLong_ = true;

    generateSchedule();
}

void Swap::generateSchedule()
{
    if (startDate == maturityDate || frequency <= 0.0 || frequency > 1.0)
        throw std::runtime_error("Error: invalid swap frequency or date range.");

    std::string tenorStr;
    if (std::abs(frequency - 0.25) < 1e-6)
        tenorStr = "3M";
    else if (std::abs(frequency - 0.5) < 1e-6)
        tenorStr = "6M";
    else
        tenorStr = "1Y";

    Date seed = startDate;
    while (seed < maturityDate) {
        swapSchedule.push_back(seed);
        seed = dateAddTenor(seed, tenorStr);
    }
    swapSchedule.push_back(maturityDate);

    if (swapSchedule.size() < 2)
        throw std::runtime_error("Error: generated schedule is invalid — check frequency and dates.");
}

double Swap::getAnnuity(const Market& mkt) const
{
    if (swapSchedule.empty())
        const_cast<Swap*>(this)->generateSchedule();  // acceptable if swapSchedule is not mutable

    double annuity = 0.0;
    Date valueDate = mkt.asOf;
    const auto& rc = mkt.getCurve(rateCurve);

    for (size_t i = 1; i < swapSchedule.size(); ++i) {
        Date dt = swapSchedule[i];
        if (dt < valueDate) continue;

        double tau = (swapSchedule[i] - swapSchedule[i - 1]) / 360.0;  // ACT/360
        double df = rc->getDf(dt);
        annuity += notional * tau * df;
    }

    return annuity;
}

double Swap::pv(const Market& mkt) const
{
    if (swapSchedule.empty())
        const_cast<Swap*>(this)->generateSchedule();

    Date valueDate = mkt.asOf;
    const auto& rc = mkt.getCurve(rateCurve);

    double df = rc->getDf(maturityDate);
    double fltPv = notional * (1.0 - df);  // Floating leg PV

    double fixPv = 0.0;
    for (size_t i = 1; i < swapSchedule.size(); ++i) {
        Date dt = swapSchedule[i];
        if (dt < valueDate) continue;

        double tau = (swapSchedule[i] - swapSchedule[i - 1]) / 360.0; // ACT/360
        df = rc->getDf(dt);
        fixPv += notional * tau * tradeRate * df;
    }

    double pv = fixPv + fltPv;
    return isLong_ ? pv : -pv;
}

double Swap::price(const Market& mkt) const {
    return pv(mkt);
}

double Swap::payoff(double r) const {
    return isLong_ ? (r - tradeRate) * notional : (tradeRate - r) * notional;
}

double Swap::payoff(const Market& mkt) const {
    return pv(mkt); // reuse cashflow logic
}

double Swap::valueAtNode(double, double, double continuation) const {
    return continuation; // Not early-exercisable
}

const std::string& Swap::getType() const { return tradeType; }
const std::string& Swap::getUnderlying() const { return underlying; }
const Date& Swap::getTradeDate() const { return tradeDate; }
const Date& Swap::getExpiry() const { return maturityDate; }
double Swap::getNotional() const { return notional; }
const std::string& Swap::getRateCurve() const { return rateCurve; }
double Swap::getStrike() const { return tradeRate; }

bool Swap::isLong() const {
    return isLong_;
}

std::shared_ptr<Trade> Swap::clone() const {
    return std::make_shared<Swap>(*this);
}

OptionType Swap::getOptionType() const {
    return OptionType::None;
}