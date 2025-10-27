#include "value.hpp"
#include <ranges>

std::vector<std::string> g_program_arguments;

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

        std::string strLower = val->stringValue;
        std::ranges::transform(strLower, strLower.begin(), ::tolower);
        
        if (strLower == "true") {
            return 1.0;
        } else if (strLower == "false") {
            return 0.0;
        }

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

static int compare_arrays(Primitive* left, Primitive* right) {
    if (left == right) return 0;

    if (left->arrayData.size() != right->arrayData.size()) return 1;

    for (const auto& [key, val] : left->arrayData) {
        auto it = right->arrayData.find(key);
        if (it == right->arrayData.end()) {
            return 1;
        }

        if (compare_values(val.get(), it->second.get()) != 0) {
            return 1;
        }
    }
    
    return 0;
}

static int compare_values(Primitive* left, Primitive* right) {
    if (!left || !right) return 0;

    if (left->type == Primitive::Type::Array && right->type == Primitive::Type::Array) {
        return compare_arrays(left, right);
    }

    if (left->type == Primitive::Type::Array || right->type == Primitive::Type::Array) {
        return 1;
    }

    if (left->type == Primitive::Type::Number &&
        right->type == Primitive::Type::Number) {
        const double diff = left->numberValue - right->numberValue;
        if (diff < 0) return -1;
        if (diff > 0) return 1;
        return 0;
    }

    const std::string leftStr = value_to_string(left);
    const std::string rightStr = value_to_string(right);

    std::string leftLower = leftStr;
    std::string rightLower = rightStr;
    std::ranges::transform(leftLower, leftLower.begin(), ::tolower);
    std::ranges::transform(rightLower, rightLower.begin(), ::tolower);

    if (leftLower == "true" && rightLower == "true") {
        return 0;
    }

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