// Helpers around Order construction.  The struct itself lives in the header.

#include "mt/backtest/order_model.hpp"

namespace mt {

// Validate that an Order is internally consistent (no inverted stops, etc.).
// Returns false on any structural inconsistency.
bool validate_order(const Order& o) {
    if (o.side != -1 && o.side != 0 && o.side != 1) return false;
    if (o.qty < 0.0) return false;
    switch (o.type) {
        case OrderType::Market:    return true;
        case OrderType::Limit:     return o.limit_px > 0.0;
        case OrderType::Stop:      return o.stop_px  > 0.0;
        case OrderType::StopLimit: return o.limit_px > 0.0 && o.stop_px > 0.0;
    }
    return false;
}

}  // namespace mt
