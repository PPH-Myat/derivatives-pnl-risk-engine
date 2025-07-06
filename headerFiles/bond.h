#pragma once

#include "trade.h"
#include "date.h"
#include <string>
#include <vector>

class Market;

class Bond : public Trade {
public:
    Bond(std::string curveName,
        Date start,
        Date end,
        double _notional,
        double _rate,
        double _freq);

    double price(const Market& mkt) const override;
    double pv(const Market& mkt) const override;
    double payoff(double marketPrice) const override;
    double payoff(const Market& mkt) const override;
    double valueAtNode(double S, double t, double continuation) const override;

    const std::string& getType() const override;
    const std::string& getUnderlying() const override;
    const std::string& getRateCurve() const override;
    const Date& getTradeDate() const override;
    const Date& getExpiry() const override;
    double getNotional() const override;
    double getStrike() const override;

    std::shared_ptr<Trade> clone() const override;

    void generateSchedule();

private:
    std::string underlying;
    Date startDate;
    Date maturityDate;
    double notional;
    double couponRate;
    double frequency;
    std::string rateCurve;

    std::vector<Date> bondSchedule;
};