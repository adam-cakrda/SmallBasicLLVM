#include "value.hpp"

extern "C" Primitive* value_from_number(const double num) {
    return new Primitive(num);
}

extern "C" Primitive* value_from_string(const char* str) {
    return new Primitive(std::string(str));
}

extern "C" double value_to_number(const Primitive* val) {
    if (!val) return 0.0;

    if (val->type == Primitive::Type::Number) {
        return val->numberValue;
    } else if (val->type == Primitive::Type::String) {
        try {
            return std::stod(val->stringValue);
        } catch (...) {
            return 0.0;
        }
    }
    return 0.0;
}

extern "C" const char* value_to_string(Primitive* val) {
    if (!val) return "";

    if (val->type == Primitive::Type::String) {
        return val->stringValue.c_str();
    } else if (val->type == Primitive::Type::Number) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(10) << val->numberValue;
        std::string str = oss.str();

        if (str.find('.') != std::string::npos) {
            str.erase(str.find_last_not_of('0') + 1, std::string::npos);
            if (str.back() == '.') {
                str.pop_back();
            }
        }

        val->stringValue = str;
        return val->stringValue.c_str();
    }
    return "";
}

extern "C" Primitive* value_add(Primitive* left, Primitive* right) {
    if (!left || !right) return new Primitive(0.0);

    if (left->type == Primitive::Type::String ||
        right->type == Primitive::Type::String) {
        const std::string result = std::string(value_to_string(left)) +
                           std::string(value_to_string(right));
        return new Primitive(result);
    }

    return new Primitive(value_to_number(left) + value_to_number(right));
}

extern "C" Primitive* value_sub(const Primitive* left, const Primitive* right) {
    if (!left || !right) return new Primitive(0.0);
    return new Primitive(value_to_number(left) - value_to_number(right));
}

extern "C" Primitive* value_mul(const Primitive* left, const Primitive* right) {
    if (!left || !right) return new Primitive(0.0);
    return new Primitive(value_to_number(left) * value_to_number(right));
}

extern "C" Primitive* value_div(const Primitive* left, const Primitive* right) {
    if (!left || !right) return new Primitive(0.0);
    double divisor = value_to_number(right);
    if (divisor == 0.0) return new Primitive(0.0);
    return new Primitive(value_to_number(left) / divisor);
}

static int compare_values(Primitive* left, Primitive* right) {
    if (!left || !right) return 0;

    if (left->type == Primitive::Type::Number &&
        right->type == Primitive::Type::Number) {
        const double diff = left->numberValue - right->numberValue;
        if (diff < 0) return -1;
        if (diff > 0) return 1;
        return 0;
    }

    std::string leftStr = value_to_string(left);
    std::string rightStr = value_to_string(right);

    std::ranges::transform(leftStr, leftStr.begin(), ::tolower);
    std::ranges::transform(rightStr, rightStr.begin(), ::tolower);

    return leftStr.compare(rightStr);
}

extern "C" int value_eq(Primitive* left, Primitive* right) {
    return compare_values(left, right) == 0 ? 1 : 0;
}

extern "C" int value_neq(Primitive* left, Primitive* right) {
    return compare_values(left, right) != 0 ? 1 : 0;
}

extern "C" int value_lt(Primitive* left, Primitive* right) {
    return compare_values(left, right) < 0 ? 1 : 0;
}

extern "C" int value_gt(Primitive* left, Primitive* right) {
    return compare_values(left, right) > 0 ? 1 : 0;
}

extern "C" int value_lte(Primitive* left, Primitive* right) {
    return compare_values(left, right) <= 0 ? 1 : 0;
}

extern "C" int value_gte(Primitive* left, Primitive* right) {
    return compare_values(left, right) >= 0 ? 1 : 0;
}