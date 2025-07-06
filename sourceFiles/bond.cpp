#include "Types.h"
#include "bond.h"
#include "market.h"
#include "helper.h"
#include <stdexcept>
#include <cmath>
#include <iostream>

using util::dateAddTenor;
using util::to_upper;

Bond::Bond(std::string curveName,
    Date start,
    Date end,
    double _notional,
    double _rate,
    double _freq)
{
    tradeType = "Bond";
    underlying = to_upper(curveName);
    startDate = start;
    maturityDate = end;
    tradeDate = start;
    notional = _notional;
    couponRate = _rate;
    frequency = _freq;
    rateCurve = to_upper(curveName);
    isLong_ = true;

    generateSchedule();
}

void Bond::generateSchedule()
{
    if (startDate == maturityDate || frequency <= 0 || frequency > 1)
        throw std::runtime_error("Error: invalid bond schedule frequency or dates!");

    std::string tenorStr;
    if (std::abs(frequency - 0.25) < 1e-6) tenorStr = "3M";
    else if (std::abs(frequency - 0.5) < 1e-6) tenorStr = "6M";
    else tenorStr = "1Y";

    Date seed = startDate;
    while (seed < maturityDate) {
        bondSchedule.push_back(seed);
        seed = dateAddTenor(seed, tenorStr);
    }
    bondSchedule.push_back(maturityDate);

    if (bondSchedule.size() < 2)
        throw std::runtime_error("Error: generated bond schedule is invalid.");
}

double Bond::payoff(double marketPrice) const {
    return isLong_ ? notional * (marketPrice - 100.0)
        : notional * (100.0 - marketPrice);
}

double Bond::payoff(const Market& mkt) const {
    double marketPrice = mkt.getBondPrice(underlying);
    return payoff(marketPrice);
}

double Bond::pv(const Market& mkt) const {
    if (bondSchedule.empty())
        const_cast<Bond*>(this)->generateSchedule();

    double pv = 0.0;
    double coupon = notional * couponRate;
    auto rc = mkt.getCurve(rateCurve);
    Date valueDate = mkt.asOf;

    for (size_t i = 1; i < bondSchedule.size(); ++i) {
        Date dt = bondSchedule[i];
        if (dt < valueDate) continue;

        double tau = (bondSchedule[i] - bondSchedule[i - 1]) / 365.0; // ACT/365
        double df = rc->getDf(dt);
        pv += coupon * tau * df;
    }

    // Add discounted notional
    double dfFinal = rc->getDf(maturityDate);
    pv += notional * dfFinal;

    return isLong_ ? pv : -pv;
}

double Bond::price(const Market& mkt) const {
    return pv(mkt);
}

double Bond::valueAtNode(double, double, double continuation) const {
    return continuation; // Not early exercisable
}

const std::string& Bond::getType() const { return tradeType; }
const std::string& Bond::getUnderlying() const { return underlying; }
const std::string& Bond::getRateCurve() const { return rateCurve; }
const Date& Bond::getTradeDate() const { return tradeDate; }
const Date& Bond::getExpiry() const { return maturityDate; }
double Bond::getNotional() const { return notional; }
double Bond::getStrike() const { return couponRate; }

bool Bond::isLong() const {
    return isLong_;
}

std::shared_ptr<Trade> Bond::clone() const {
    return std::make_shared<Bond>(*this);
}

OptionType Bond::getOptionType() const {
    return OptionType::None;
}