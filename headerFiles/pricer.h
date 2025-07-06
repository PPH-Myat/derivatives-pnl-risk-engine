#pragma once

#include <memory>
#include "market.h"
#include "trade.h"

class Pricer {
public:
    virtual double price(const Market& mkt, std::shared_ptr<Trade> trade) const = 0;
    virtual ~Pricer() = default;
};