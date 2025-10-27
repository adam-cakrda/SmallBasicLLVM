#include "value.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <string>

extern "C" void program_delay(const Primitive* time) {
    const auto milliseconds = static_cast<long>(value_to_number(time));
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

extern "C" Primitive* program_getargument(const Primitive* index) {
    const int idx = static_cast<int>(value_to_number(index));

    if (idx < 1 || idx > static_cast<int>(g_program_arguments.size())) {
        return new Primitive("");
    }

    return new Primitive(g_program_arguments[idx - 1]);
}

extern "C" Primitive* program_argumentcount_get() {
    return new Primitive(static_cast<double>(g_program_arguments.size()));
}

extern "C" void program_end() {
   exit(0);
}

