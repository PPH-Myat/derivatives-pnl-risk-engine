#include "european_trade.h"
#include "payoff.h"
#include "market.h"
#include "helper.h"
#include "tree_pricer.h"

#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <sstream>

using util::to_upper;

// === EuropeanOption ===

EuropeanOption::EuropeanOption()
    : optType(OptionType::Call),
    strike(0.0),
    notional(0.0),
    isLong_(true),
    rateCurve("USD-SOFR"),
    tradeType("EuropeanOption"),
    underlying("UNKNOWN"),
    tradeDate(),
    expiryDate() {
}

EuropeanOption::EuropeanOption(OptionType _optType,
    double _notional,
    double _strike,
    const Date& _tradeDate,
    const Date& _expiryDate,
    const std::string& _underlying,
    bool _isLong)
    : optType(_optType),
    strike(_strike),
    notional(_notional),
    isLong_(_isLong),
    rateCurve("USD-SOFR"),
    tradeDate(_tradeDate),
    expiryDate(_expiryDate),
    underlying(to_upper(_underlying)),
    tradeType("EuropeanOption") {
    if (_strike < 0)
        throw std::invalid_argument("Strike must be non-negative.");
    if (_expiryDate <= _tradeDate)
        throw std::invalid_argument("Expiry must be after trade date.");
    if (_underlying.empty())
        throw std::invalid_argument("Underlying cannot be empty.");
}

std::shared_ptr<Trade> EuropeanOption::clone() const {
    return std::make_shared<EuropeanOption>(*this);
}

const std::string& EuropeanOption::getType() const { return tradeType; }
const std::string& EuropeanOption::getUnderlying() const { return underlying; }
double EuropeanOption::getNotional() const { return notional; }
bool EuropeanOption::isLong() const { return isLong_; }
OptionType EuropeanOption::getOptionType() const { return optType; }
double EuropeanOption::getStrike() const { return strike; }

double EuropeanOption::payoff(double S) const {
    double raw = notional * PAYOFF::VanillaOption(optType, strike, S);
    return isLong_ ? raw : -raw;
}

double EuropeanOption::payoff(const Market& market) const {
    double S = market.getStockPrice(underlying);
    return payoff(S);
}

double EuropeanOption::valueAtNode(double, double, double continuation) const {
    return continuation;
}

double EuropeanOption::price(const Market& mkt) const {
    return pv(mkt);
}

double EuropeanOption::pv(const Market& mkt) const {
    CRRBinomialTreePricer pricer(50);
    return pricer.price(mkt, *this);
}

const Date& EuropeanOption::getExpiry() const { return expiryDate; }
const Date& EuropeanOption::getTradeDate() const { return tradeDate; }
const std::string& EuropeanOption::getRateCurve() const { return rateCurve; }

Date EuropeanOption::getVolTenor() const {
    return expiryDate;
}

// === EuroCallSpread ===

EuroCallSpread::EuroCallSpread(double _notional,
    double _strike1,
    double _strike2,
    const Date& _tradeDate,
    const Date& _expiry,
    const std::string& _underlying,
    bool _isLong)
    : strike1(_strike1), strike2(_strike2), notional(_notional),
    isLong_(_isLong), tradeDate(_tradeDate), expiryDate(_expiry),
    rateCurve("USD-SOFR"), tradeType("EuroCallSpread"),
    underlying(to_upper(_underlying)) {
    if (_strike1 >= _strike2)
        throw std::invalid_argument("strike1 must be less than strike2");
}

std::shared_ptr<Trade> EuroCallSpread::clone() const {
    return std::make_shared<EuroCallSpread>(*this);
}

const std::string& EuroCallSpread::getType() const { return tradeType; }
const std::string& EuroCallSpread::getUnderlying() const { return underlying; }
double EuroCallSpread::getNotional() const { return notional; }
bool EuroCallSpread::isLong() const { return isLong_; }

double EuroCallSpread::payoff(double S) const {
    double raw = notional * PAYOFF::CallSpread(strike1, strike2, S);
    return isLong_ ? raw : -raw;
}

double EuroCallSpread::payoff(const Market& market) const {
    double S = market.getStockPrice(underlying);
    return payoff(S);
}

double EuroCallSpread::valueAtNode(double S, double /*t*/, double continuation) const {
    return continuation;
}

double EuroCallSpread::price(const Market& mkt) const {
    return pv(mkt);
}

double EuroCallSpread::pv(const Market& mkt) const {
    CRRBinomialTreePricer pricer(50);
    return pricer.price(mkt, *this);
}

const Date& EuroCallSpread::getExpiry() const { return expiryDate; }
const Date& EuroCallSpread::getTradeDate() const { return tradeDate; }
const std::string& EuroCallSpread::getRateCurve() const { return rateCurve; }

double EuroCallSpread::getStrike() const {
    return (strike1 + strike2) / 2.0;
}

OptionType EuroCallSpread::getOptionType() const {
    return OptionType::Call;
}