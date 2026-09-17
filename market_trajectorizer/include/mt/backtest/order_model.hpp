#pragma once
#include "mt/core/types.hpp"

namespace mt {

enum class OrderType { Market, Limit, Stop, StopLimit };

struct Order {
    Timestamp ts        = 0;
    int       side      = 0;     // -1 / 0 / +1
    real      qty       = 0.0;
    real      limit_px  = 0.0;
    real      stop_px   = 0.0;
    OrderType type      = OrderType::Market;
};

}  // namespace mt
