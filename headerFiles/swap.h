#pragma once

#include "trade.h"
#include "date.h"
#include <vector>
#include <string>

class Market;

class Swap : public Trade {
public:
    Swap(std::string name,
        Date start,
        Date end,
        double _notional,
        double _rate,
        double _freq);

    // === Core Pricing Interface ===
    double price(const Market& mkt) const override;
    double pv(const Market& mkt) const override;
    double payoff(double r) const override;
    double payoff(const Market& mkt) const override;
    double valueAtNode(double S, double t, double continuation) const override;

    // === Trade Metadata ===
    const std::string& getType() const override;
    const std::string& getUnderlying() const override;
    const std::string& getRateCurve() const override;
    const Date& getTradeDate() const override;
    const Date& getExpiry() const override;
    double getNotional() const override;
    double getStrike() const override;

    // === Factory/Risk Copy Support ===
    std::shared_ptr<Trade> clone() const override;

    // === Internal Helpers ===
    double getAnnuity(const Market& mkt) const;
    void generateSchedule();

private:
    std::string underlying;
    Date startDate;
    Date maturityDate;
    double notional;
    double tradeRate;
    double frequency;
    std::string rateCurve;

    std::vector<Date> swapSchedule;
};