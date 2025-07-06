#pragma once

#include "trade.h"
#include "types.h"
#include "date.h"
#include <string>

class AmericanOption : public Trade {
public:
    AmericanOption();
    AmericanOption(OptionType optType,
        double notional,
        double strike,
        const Date& tradeDate,
        const Date& expiryDate,
        const std::string& underlying,
        bool isLong = true);

    std::shared_ptr<Trade> clone() const override;

    // Trade overrides
    const std::string& getType() const override;
    const std::string& getUnderlying() const override;
    double getNotional() const override;
    bool isLong() const override;

    double payoff(double S) const override;
    double payoff(const Market& market) const override;
    double valueAtNode(double S, double t, double continuationValue) const override;
    double price(const Market& mkt) const override;
    double pv(const Market& mkt) const override;

    // Recursion-safe variant
    double pv(const Market& mkt, bool useTree) const;

    const Date& getExpiry() const override;
    const Date& getTradeDate() const override;
    const std::string& getRateCurve() const override;
    OptionType getOptionType() const;
    double getStrike() const;
    Date getVolTenor() const;

private:
    OptionType optType;
    double strike;
    double notional;
    bool isLong_;

    std::string rateCurve;
    std::string tradeType;
    std::string underlying;

    Date tradeDate;
    Date expiryDate;
};

class AmerCallSpread : public Trade {
public:
    AmerCallSpread(double notional,
        double strike1,
        double strike2,
        const Date& tradeDate,
        const Date& expiryDate,
        const std::string& underlying,
        bool isLong = true);

    std::shared_ptr<Trade> clone() const override;

    // Trade overrides
    const std::string& getType() const override;
    const std::string& getUnderlying() const override;
    double getNotional() const override;
    bool isLong() const override;

    double payoff(double S) const override;
    double payoff(const Market& market) const override;
    double valueAtNode(double S, double t, double continuationValue) const override;
    double price(const Market& mkt) const override;
    double pv(const Market& mkt) const override;

    // Add this line:
    double pv(const Market& mkt, bool useTree) const;

    const Date& getExpiry() const override;
    const Date& getTradeDate() const override;
    const std::string& getRateCurve() const override;
    OptionType getOptionType() const override;
    double getStrike() const override;

private:
    double strike1, strike2;
    double notional;
    bool isLong_;

    std::string rateCurve;
    std::string tradeType;
    std::string underlying;

    Date tradeDate;
    Date expiryDate;
};
