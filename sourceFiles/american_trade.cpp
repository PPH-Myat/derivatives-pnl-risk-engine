#include "american_trade.h"
#include "market.h"
#include "payoff.h"
#include "helper.h"
#include "tree_pricer.h"

#include <stdexcept>
#include <algorithm>
#include <sstream>

using util::to_upper;

// === AmericanOption ===

AmericanOption::AmericanOption()
    : optType(OptionType::Call),
    strike(0.0),
    notional(0.0),
    isLong_(true),
    rateCurve("USD-SOFR"),
    tradeType("AmericanOption"),
    underlying("UNKNOWN"),
    tradeDate(),
    expiryDate() {
}

AmericanOption::AmericanOption(OptionType _optType,
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
    tradeType("AmericanOption") {
    if (_strike < 0)
        throw std::invalid_argument("Strike must be non-negative.");
    if (_expiryDate <= _tradeDate)
        throw std::invalid_argument("Expiry must be after trade date.");
    if (_underlying.empty())
        throw std::invalid_argument("Underlying cannot be empty.");
}

std::shared_ptr<Trade> AmericanOption::clone() const {
    return std::make_shared<AmericanOption>(*this);
}

const std::string& AmericanOption::getType() const { return tradeType; }
const std::string& AmericanOption::getUnderlying() const { return underlying; }
double AmericanOption::getNotional() const { return notional; }
bool AmericanOption::isLong() const { return isLong_; }
OptionType AmericanOption::getOptionType() const { return optType; }
double AmericanOption::getStrike() const { return strike; }

double AmericanOption::payoff(double S) const {
    double raw = notional * PAYOFF::VanillaOption(optType, strike, S);
    return isLong_ ? raw : -raw;
}

double AmericanOption::payoff(const Market& market) const {
    double S = market.getStockPrice(underlying);
    return payoff(S);
}

double AmericanOption::valueAtNode(double S, double /*t*/, double continuationValue) const {
    return std::max(payoff(S), continuationValue);
}

double AmericanOption::price(const Market& mkt) const {
    return pv(mkt);
}

double AmericanOption::pv(const Market& mkt) const {
    CRRBinomialTreePricer pricer(50);
    return pricer.price(mkt, std::make_shared<AmericanOption>(*this));
}

const Date& AmericanOption::getExpiry() const { return expiryDate; }
const Date& AmericanOption::getTradeDate() const { return tradeDate; }
const std::string& AmericanOption::getRateCurve() const { return rateCurve; }

// === AmerCallSpread ===

AmerCallSpread::AmerCallSpread(double _notional, double _strike1, double _strike2,
    const Date& _tradeDate, const Date& _expiry,
    const std::string& _underlying, bool _isLong)
    : strike1(_strike1), strike2(_strike2), notional(_notional),
    isLong_(_isLong), tradeDate(_tradeDate), expiryDate(_expiry),
    rateCurve("USD-SOFR"), tradeType("AmerCallSpread"),
    underlying(to_upper(_underlying)) {
    if (_strike1 >= _strike2)
        throw std::invalid_argument("strike1 must be less than strike2");
}

std::shared_ptr<Trade> AmerCallSpread::clone() const {
    return std::make_shared<AmerCallSpread>(*this);
}

const std::string& AmerCallSpread::getType() const { return tradeType; }
const std::string& AmerCallSpread::getUnderlying() const { return underlying; }
double AmerCallSpread::getNotional() const { return notional; }
bool AmerCallSpread::isLong() const { return isLong_; }

double AmerCallSpread::payoff(double S) const {
    double raw = notional * PAYOFF::CallSpread(strike1, strike2, S);
    return isLong_ ? raw : -raw;
}

double AmerCallSpread::payoff(const Market& market) const {
    double S = market.getStockPrice(underlying);
    return payoff(S);
}

double AmerCallSpread::valueAtNode(double S, double /*t*/, double continuation) const {
    return std::max(payoff(S), continuation);
}

double AmerCallSpread::price(const Market& mkt) const {
    return pv(mkt);
}

double AmerCallSpread::pv(const Market& mkt) const {
    CRRBinomialTreePricer pricer(50);
    return pricer.price(mkt, std::make_shared<AmerCallSpread>(*this));
}

const Date& AmerCallSpread::getExpiry() const { return expiryDate; }
const Date& AmerCallSpread::getTradeDate() const { return tradeDate; }
const std::string& AmerCallSpread::getRateCurve() const { return rateCurve; }

double AmerCallSpread::getStrike() const {
    return (strike1 + strike2) / 2.0;
}

OptionType AmerCallSpread::getOptionType() const {
    return OptionType::Call;
}