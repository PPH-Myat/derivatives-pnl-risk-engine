#pragma once

#include <algorithm>
#include <stdexcept>

#include "types.h"

// ===========================
// PAYOFF Namespace
// ===========================
namespace PAYOFF
{
    // Vanilla option payoff: supports Call, Put, Binary Call, Binary Put
    inline double VanillaOption(OptionType optType, double strike, double S)
    {
        switch (optType) {
        case OptionType::Call:
            return std::max(S - strike, 0.0);
        case OptionType::Put:
            return std::max(strike - S, 0.0);
        case OptionType::BinaryCall:
            return S >= strike ? 1.0 : 0.0;
        case OptionType::BinaryPut:
            return S <= strike ? 1.0 : 0.0;
        default:
            throw std::runtime_error("Unsupported OptionType");
        }
    }

    // Normalized call spread payoff: linearly ramps from 0 to 1 between strike1 and strike2
    inline double CallSpread(double strike1, double strike2, double S)
    {
        if (S < strike1) return 0.0;
        else if (S > strike2) return 1.0;
        else return (S - strike1) / (strike2 - strike1);
    }

    // Exact dollar-value call spread payoff: difference between two vanilla calls
    inline double CallSpreadExact(double strike1, double strike2, double S)
    {
        return std::max(0.0, S - strike1) - std::max(0.0, S - strike2);
    }
}