#pragma once
#include "mt/core/types.hpp"

namespace mt {

struct TradingCalendar {
    enum class Kind {
        ContinuousBars,
        EquitiesDaily,
        EquitiesRTH,
        FuturesRTH,
        CryptoUTC
    };
    Kind kind = Kind::ContinuousBars;

    real bars_per_day = 1.0;
    Timestamp epoch_ts = 0;
    real bar_step_seconds = 60.0;
};

TradingCalendar default_calendar(TradingCalendar::Kind k);
real bars_to_days(const TradingCalendar& cal, real bars);
real days_to_bars(const TradingCalendar& cal, real days);
Timestamp index_to_timestamp(const TradingCalendar& cal, usize index);
usize timestamp_to_index(const TradingCalendar& cal, Timestamp ts);

}  // namespace mt
