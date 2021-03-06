#ifndef IV_DATE_UTILS_H_
#define IV_DATE_UTILS_H_
#include <ctime>
#include <cmath>
#include <cstring>
#include <iv/detail/cstdint.h>
#include <iv/detail/array.h>
#include <iv/platform.h>
#include <iv/platform_math.h>
#include <iv/conversions.h>
#include <iv/utils.h>
#include <iv/canonicalized_nan.h>
namespace iv {
namespace core {
namespace date {

static const int kHoursPerDay = 24;
static const int kMinutesPerHour = 60;
static const int kSecondsPerMinute = 60;

static const int kMsPerSecond = 1000;
static const int kMsPerMinute = kMsPerSecond * kSecondsPerMinute;
static const int kMsPerHour = kMsPerMinute * kMinutesPerHour;
static const int kMsPerDay = kMsPerHour * kHoursPerDay;

static const int64_t kEpochTime = 116444736000000000LL;

static const double kMaxLocal = 8640002592000000.0;
static const double kMaxTime = 8.64E15;

inline double Day(double t) {
  return std::floor(t / kMsPerDay);
}

inline int64_t Int64Day(int64_t time) {
  if (time < 0) {
    time -= (kMsPerDay - 1);
  }
  return time / kMsPerDay;
}

inline double TimeWithinDay(double t) {
  const double res = core::math::Modulo(t, kMsPerDay);
  if (res < 0) {
    return res + kMsPerDay;
  }
  return res;
}

inline int DaysInYear(int y) {
  return (((y % 100 == 0) ? y / 100 : y) % 4 == 0) ? 366 : 365;
}

inline double DaysFromYear(int y) {
  const int kLeapDaysBefore1971By4Rule = 1970 / 4;
  const int kExcludeLeapDaysBefore1971By100Rule = 1970 / 100;
  const int kLeapDaysBefore1971By400Rule = 1970 / 400;

  const double year_minus_one = y - 1;
  const double years_to_add_by_4 =
      std::floor(year_minus_one / 4.0) - kLeapDaysBefore1971By4Rule;
  const double years_to_exclude_by_100 =
      std::floor(year_minus_one / 100.0) - kExcludeLeapDaysBefore1971By100Rule;
  const double years_to_add_by_400 =
      std::floor(year_minus_one / 400.0) - kLeapDaysBefore1971By400Rule;
  return 365.0 * (y - 1970) +
      years_to_add_by_4 -
      years_to_exclude_by_100 +
      years_to_add_by_400;
}

inline double TimeFromYear(int y) {
  return kMsPerDay * DaysFromYear(y);
}

// 365.2425 days are average year in every 400 years.
// See http://en.wikipedia.org/wiki/Gregorian_calendar
inline int YearFromTime(double t) {
  const int about = static_cast<int>(
      std::floor(t / (kMsPerDay * 365.2425)) + 1970);
  const double time = TimeFromYear(about);
  if (time > t) {
    return about - 1;
  } else if (TimeFromYear(about+1) <= t) {
    return about + 1;
  } else {
    return about;
  }
}

inline int IsLeapYear(double t) {
  return (DaysInYear(YearFromTime(t)) == 366) ? 1 : 0;
}

inline int DayWithinYear(double t) {
  return static_cast<int>(Day(t) - DaysFromYear(YearFromTime(t)));
}

static const std::array<std::array<int, 12>, 2> kDaysMap = { {
  { {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31} },
  { {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31} }
} };

static const std::array<std::array<int, 12>, 2> kMonthMap = { {
  { {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334} },
  { {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335} }
} };

static const std::array<const char*, 12> kMonths = { {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
} };

inline int MonthFromTime(double t) {
  int within = DayWithinYear(t);
  const int leap = IsLeapYear(t);
  for (int i = 0; i < 12; ++i) {
    within -= kDaysMap[leap][i];
    if (within < 0) {
      return i;
    }
  }
  UNREACHABLE();
  return 0;  // makes compiler happy
}

inline const char* MonthToString(double t) {
  assert(0 <= MonthFromTime(t));
  assert(MonthFromTime(t) < static_cast<int>(kMonths.size()));
  return kMonths[MonthFromTime(t)];
}

inline int DateFromTime(double t) {
  int within = DayWithinYear(t);
  const int leap = IsLeapYear(t);
  for (int i = 0; i < 12; ++i) {
    within -= kDaysMap[leap][i];
    if (within < 0) {
      return within + kDaysMap[leap][i] + 1;
    }
  }
  UNREACHABLE();
  return 0;  // makes compiler happy
}

inline int MonthToDaysInYear(int month, int is_leap) {
  return kMonthMap[is_leap][month];
}

static const std::array<const char*, 7> kWeekDays = { {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
} };

inline int32_t WeekDayFromDay(int64_t day) {
  const int32_t res = (day + 4) % 7;
  if (res < 0) {
    return res + 7;
  }
  return res;
}

inline int WeekDay(double t) {
  const int res = static_cast<int>(core::math::Modulo((Day(t) + 4), 7));
  if (res < 0) {
    return res + 7;
  }
  return res;
}

inline const char* WeekDayToString(double t) {
  assert(0 <= WeekDay(t));
  assert(WeekDay(t) < static_cast<int>(kWeekDays.size()));
  return kWeekDays[WeekDay(t)];
}

inline int32_t GetLocalTZA() {
  const std::time_t current = std::time(nullptr);
  std::tm local;
  std::memcpy(&local, std::localtime(&current), sizeof(std::tm));  // NOLINT
  local.tm_sec = 0;
  local.tm_min = 0;
  local.tm_hour = 0;
  local.tm_mday = 1;
  local.tm_mon = 0;
  local.tm_wday = 0;
  local.tm_yday = 0;
  local.tm_isdst = 0;
  local.tm_year = 109;
  return static_cast<int32_t>((1230768000 - std::mktime(&local)) * 1000);
}

inline int32_t LocalTZA() {
  static const int32_t local_tza = GetLocalTZA();
  return local_tza;
}

double DaylightSavingTA(double t);

inline double DaylightSavingTAFallback(double utc) {
  // fallback
  // Daylight Saving Time
  // from    2 AM the first Sunday in April
  // through 2 AM the last Sunday in October
  assert(!core::math::IsNaN(utc));
  const int year = YearFromTime(utc);
  const int leap = IsLeapYear(utc);

  double start = TimeFromYear(year);
  double end = start;


  // goto April 1st
  start += static_cast<double>(MonthToDaysInYear(3, leap)) * kMsPerDay;
  // goto the first Sunday in April
  while (WeekDay(start) != 0) {
    start += kMsPerDay;
  }

  // goto Octobar 30th
  end += static_cast<double>((MonthToDaysInYear(9, leap) + 30)) * kMsPerDay;
  // goto the last Sunday in Octobar
  while (WeekDay(end) != 0) {
    end -= kMsPerDay;
  }

  const double target = utc - 2 * kMsPerHour;

  if (start <= target && target <= end) {
    return kMsPerHour;
  }
  return 0.0;
}

const char* LocalTimeZone(double t);

inline double LocalTime(double t) {
  return t + LocalTZA() + DaylightSavingTA(t);
}

inline double UTC(double t) {
  const double local = LocalTZA();
  return t - local - DaylightSavingTA(t - local);
}

inline int HourFromTime(double t) {
  const int res = static_cast<int>(
      core::math::Modulo(std::floor(t / kMsPerHour), kHoursPerDay));
  if (res < 0) {
    return res + kHoursPerDay;
  }
  return res;
}

inline int MinFromTime(double t) {
  const int res = static_cast<int>(
      core::math::Modulo(std::floor(t / kMsPerMinute), kMinutesPerHour));
  if (res < 0) {
    return res + kMinutesPerHour;
  }
  return res;
}

inline int SecFromTime(double t) {
  const int res = static_cast<int>(
      core::math::Modulo(std::floor(t / kMsPerSecond), kSecondsPerMinute));
  if (res < 0) {
    return res + kSecondsPerMinute;
  }
  return res;
}

inline int MsFromTime(double t) {
  const int res = static_cast<int>(core::math::Modulo(t, kMsPerSecond));
  if (res < 0) {
    return res + kMsPerSecond;
  }
  return res;
}

inline double MakeTime(double hour, double min, double sec, double ms) {
  if (!core::math::IsFinite(hour) ||
      !core::math::IsFinite(min) ||
      !core::math::IsFinite(sec) ||
      !core::math::IsFinite(ms)) {
    return core::kNaN;
  } else {
    return
        core::DoubleToInteger(hour) * kMsPerHour +
        core::DoubleToInteger(min) * kMsPerMinute +
        core::DoubleToInteger(sec) * kMsPerSecond +
        core::DoubleToInteger(ms);
  }
}

inline double DateToDays(int year, int month, int date) {
  if (month < 0) {
    month += 12;
    --year;
  }
  const double yearday = std::floor(DaysFromYear(year));
  const int monthday = MonthToDaysInYear(month,
                                         (DaysInYear(year) == 366 ? 1 : 0));
  assert((year >= 1970 && yearday >= 0) || (year < 1970 && yearday < 0));
  return yearday + monthday + date - 1;
}

inline double MakeDay(double year, double month, double date) {
  if (!core::math::IsFinite(year) ||
      !core::math::IsFinite(month) ||
      !core::math::IsFinite(date)) {
    return core::kNaN;
  } else {
    const int y = static_cast<int>(year);
    const int m = static_cast<int>(month);
    const int dt = static_cast<int>(date);
    const int ym = y + m / 12;
    const int mn = m % 12;
    return DateToDays(ym, mn, dt);
  }
}

inline double MakeDate(double day, double time) {
  const double res = day * kMsPerDay + time;
  if (std::abs(res) > kMaxLocal) {
    return core::kNaN;
  }
  return res;
}

inline double TimeClip(double time) {
  if (!core::math::IsFinite(time)) {
    return core::kNaN;
  }
  if (std::abs(time) > kMaxTime) {
    return core::kNaN;
  }
  return core::DoubleToInteger(time);
}

inline void YMDFromTime(int64_t time, int32_t* year, int32_t* month, int32_t* date) {
  const int32_t about_year = static_cast<int32_t>(
      std::floor(time / (kMsPerDay * 365.2425)) + 1970);
  const double expected = TimeFromYear(about_year);

  const int32_t y =
      (expected > time) ? about_year - 1 :
      (TimeFromYear(about_year + 1) <= time) ? about_year + 1 :
      about_year;
  *year = y;

  int64_t day = Day(time) - DaysFromYear(y);
  const bool leap = DaysInYear(y) == 366;
  for (int i = 0; i < 12; ++i) {
    day -= kDaysMap[leap][i];
    if (day < 0) {
      *month = i;
      *date = day + kDaysMap[leap][i] + 1;
      return;
    }
  }

  UNREACHABLE();
}

class DateInstance {
 public:
  enum {
    OK = 0,
    NG = 1,
    INVALIDATED = 2
  };

  DateInstance() {
    SetValue(core::kNaN);
  }

  explicit DateInstance(double value) {
    SetValue(value);
  }

  void SetValue(double value) {
    value_ = value;
    invalidate_ = (core::math::IsNaN(value)) ? NG : INVALIDATED;
  }

  bool IsValid() const { return invalidate_ != NG; }

  double value() const {
    return value_;
  }

  int32_t year() const {
    CalcucateCache();
    return year_;
  }

  int32_t month() const {
    CalcucateCache();
    return month_;
  }

  int32_t date() const {
    CalcucateCache();
    return date_;
  }

  int32_t weekday() const {
    CalcucateCache();
    return weekday_;
  }

  int32_t hour() const {
    CalcucateCache();
    return hour_;
  }

  int32_t min() const {
    CalcucateCache();
    return min_;
  }

  int32_t sec() const {
    CalcucateCache();
    return sec_;
  }

  int32_t ms() const {
    CalcucateCache();
    return ms_;
  }

  const char* WeekDayString() const {
    return kWeekDays[weekday()];
  }

  const char* MonthString() const {
    return kMonths[month()];
  }

 private:
  void CalcucateCache() const {
    if (invalidate_ != INVALIDATED) {
      return;
    }

    const int64_t time = static_cast<int64_t>(value_);
    const int64_t day = Int64Day(time);
    YMDFromTime(value(), &year_, &month_, &date_);
    weekday_ = WeekDayFromDay(day);

    const int64_t time_in_day = time - kMsPerDay * day;
    hour_ = time_in_day / kMsPerHour;
    min_ = (time_in_day / kMsPerMinute) % kMinutesPerHour;
    sec_ = (time_in_day / kMsPerSecond) % kSecondsPerMinute;
    ms_ = time_in_day % kMsPerSecond;
    invalidate_ = OK;
  }

  double value_;
  mutable int32_t year_;
  mutable int32_t month_;
  mutable int32_t date_;
  mutable int32_t weekday_;
  mutable int32_t hour_;
  mutable int32_t min_;
  mutable int32_t sec_;
  mutable int32_t ms_;
  mutable int32_t invalidate_;
};

} } }  // namespace iv::core::date
#if defined(IV_OS_WIN)
#include <iv/date_utils_win.h>
#else
#include <iv/date_utils_posix.h>
#endif  // OS_WIN
#endif  // IV_DATE_UTILS_H_
