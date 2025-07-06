#pragma once

#include <memory>
#include "pricer.h"
#include "market.h"
#include "trade.h"

class BlackScholesPricer : public Pricer {
public:
    double price(const Market& mkt, std::shared_ptr<Trade> trade) const override;
};
