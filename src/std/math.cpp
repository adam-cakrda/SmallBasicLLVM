#include "value.hpp"

extern "C" Primitive* math_abs(const Primitive* val) {
    if (!val) return new Primitive(0.0);
    return new Primitive(std::abs(value_to_number(val)));
}