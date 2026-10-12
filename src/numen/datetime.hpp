#pragma once

#include "numen/numen.hpp"
#include "parser.hpp"
#include <chrono>

namespace numen::detail {

namespace {

// use the magnitude of the number to determine what timestamp it probably is
inline TimePoint fromUnknownEpoch(std::int64_t v) {
  using namespace std::chrono;
  const std::int64_t a = v < 0 ? -v : v;

  if (a < 100'000'000'000LL) // < 1e11
    return system_clock::time_point{seconds{v}};
  if (a < 100'000'000'000'000LL) // < 1e14
    return system_clock::time_point{milliseconds{v}};
  if (a < 100'000'000'000'000'000LL) // < 1e17
    return system_clock::time_point{duration_cast<system_clock::duration>(microseconds{v})};
  return system_clock::time_point{duration_cast<system_clock::duration>(nanoseconds{v})};
}
} // namespace

// calendar difference, always non-negative: callers order the operands
Duration subtractDates(const DateTime &lhs, const DateTime &rhs);

DateTime parseDateTime(const DateString &d, const tz::time_zone &userTz, TimePoint now);

// nanoseconds reach about 292 years either side of the epoch; past that duration
// arithmetic overflows. the bound keeps a day spare for a time of day
TimePoint checkedTimePoint(std::chrono::sys_seconds t);

template <typename T> DateTime fromEpoch(T duration, const tz::time_zone *tz = nullptr) {
  DateTime dt{.tz = tz};

  if constexpr (std::is_arithmetic_v<T>) {
    dt.time = fromUnknownEpoch(static_cast<std::int64_t>(duration));
  } else {
    dt.time = TimePoint{duration};
  }

  return dt;
}

// moves by whole calendar units, clamping to the last day of the month when
// the day does not exist there (jan 31 + 1 month is feb 28)
template <typename T> TimePoint shift(TimePoint t, T duration) {
  if (!duration.count()) return t;

  auto time = std::chrono::floor<std::chrono::days>(t);
  std::chrono::year_month_day ymd{time};
  auto tod = t - time;
  ymd += duration;

  if (!ymd.ok()) ymd = ymd.year() / ymd.month() / std::chrono::last;

  return checkedTimePoint(std::chrono::sys_days{ymd}) + tod;
}

} // namespace numen::detail
