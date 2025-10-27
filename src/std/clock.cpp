#include "value.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

#include <cmath>

static std::tm get_local_time() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return tm;
}

static auto get_time_with_millis() {
    return std::chrono::system_clock::now();
}

extern "C" Primitive* clock_time_get() {
    const std::tm tm = get_local_time();
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
        << std::setfill('0') << std::setw(2) << tm.tm_min << ":"
        << std::setfill('0') << std::setw(2) << tm.tm_sec;
    return new Primitive(oss.str());
}

extern "C" Primitive* clock_date_get() {
    const std::tm tm = get_local_time();
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << tm.tm_mday << "."
        << std::setfill('0') << std::setw(2) << (tm.tm_mon + 1) << "."
        << (tm.tm_year + 1900);
    return new Primitive(oss.str());
}

// Clock.Year - returns current year
extern "C" Primitive* clock_year_get() {
    const std::tm tm = get_local_time();
    return new Primitive(static_cast<double>(tm.tm_year + 1900));
}

extern "C" Primitive* clock_month_get() {
    const std::tm tm = get_local_time();
    return new Primitive(static_cast<double>(tm.tm_mon + 1));
}

extern "C" Primitive* clock_day_get() {
    const std::tm tm = get_local_time();
    return new Primitive(static_cast<double>(tm.tm_mday));
}

extern "C" Primitive* clock_weekday_get() {
    const std::tm tm = get_local_time();
    const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    return new Primitive(days[tm.tm_wday]);
}

extern "C" Primitive* clock_hour_get() {
    const std::tm tm = get_local_time();
    return new Primitive(static_cast<double>(tm.tm_hour));
}

extern "C" Primitive* clock_minute_get() {
    const std::tm tm = get_local_time();
    return new Primitive(static_cast<double>(tm.tm_min));
}

extern "C" Primitive* clock_second_get() {
    const std::tm tm = get_local_time();
    return new Primitive(static_cast<double>(tm.tm_sec));
}

extern "C" Primitive* clock_millisecond_get() {
    const auto now = get_time_with_millis();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    return new Primitive(static_cast<double>(ms.count()));
}

extern "C" Primitive* clock_elapsedmilliseconds_get() {
    const auto us_since_1970 = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    const std::time_t t = std::time(nullptr);
    std::tm utc_tm{}, local_tm{};
#ifdef _WIN32
    gmtime_s(&utc_tm, &t);
    localtime_s(&local_tm, &t);
#else
    gmtime_r(&t, &utc_tm);
    localtime_r(&t, &local_tm);
#endif

    const double offset_ms = std::difftime(std::mktime(&local_tm), std::mktime(&utc_tm)) * 1000;

    constexpr long long ms_from_1900_to_1970 = 2208988800000LL;
    const double ms_since_1900 = static_cast<double>(us_since_1970) / 1000.0 + 
                                  ms_from_1900_to_1970 + offset_ms;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << std::round(ms_since_1900 * 100.0) / 100.0;
    std::string result = oss.str();
    
    if (const size_t pos = result.find('.'); pos != std::string::npos) {
        result[pos] = ',';
    }
    
    return new Primitive(result);
}
