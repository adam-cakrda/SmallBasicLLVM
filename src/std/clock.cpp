#include "value.hpp"

extern "C" Primitive* clock_date_get() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    const int y = tm.tm_year + 1900;
    const int m = tm.tm_mon + 1;
    const int d = tm.tm_mday;
    // TODO: format date correctly
    const auto yyyymmdd = static_cast<double>(y * 10000 + m * 100 + d);
    return new Primitive(yyyymmdd);
}
