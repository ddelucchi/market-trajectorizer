#include "mt/data/trading_calendar.hpp"

namespace mt {

real bars_to_days(const TradingCalendar& cal, real bars) {
    return (cal.bars_per_day > 0.0) ? (bars / cal.bars_per_day) : bars;
}

real days_to_bars(const TradingCalendar& cal, real days) {
    return days * cal.bars_per_day;
}

TradingCalendar default_calendar(TradingCalendar::Kind k) {
    TradingCalendar c; c.kind = k;
    switch (k) {
        case TradingCalendar::Kind::ContinuousBars:
            c.bars_per_day = 1.0;
            c.bar_step_seconds = 1.0;
            break;
        case TradingCalendar::Kind::EquitiesDaily:
            c.bars_per_day = 1.0;
            c.bar_step_seconds = 86400.0;
            break;
        case TradingCalendar::Kind::EquitiesRTH:
            c.bars_per_day = 390.0;
            c.bar_step_seconds = 60.0;
            break;
        case TradingCalendar::Kind::FuturesRTH:
            c.bars_per_day = 1380.0;
            c.bar_step_seconds = 60.0;
            break;
        case TradingCalendar::Kind::CryptoUTC:
            c.bars_per_day = 1440.0;
            c.bar_step_seconds = 60.0;
            break;
    }
    return c;
}

Timestamp index_to_timestamp(const TradingCalendar& cal, usize index) {
    return cal.epoch_ts + static_cast<Timestamp>(static_cast<real>(index) * cal.bar_step_seconds);
}

usize timestamp_to_index(const TradingCalendar& cal, Timestamp ts) {
    if (ts <= cal.epoch_ts) return 0;
    const real dt = static_cast<real>(ts - cal.epoch_ts);
    return static_cast<usize>(dt / cal.bar_step_seconds);
}

}  // namespace mt
