#include <ranges>
#include <algorithm>

#include "value.hpp"

extern "C" SmallBasicValue* array_getitemcount(const SmallBasicValue* array) {
    if (!array || array->type != SmallBasicValue::Type::Array) {
        return new SmallBasicValue(0.0);
    }
    return new SmallBasicValue(static_cast<double>(array->arrayData.size()));
}

extern "C" SmallBasicValue* array_containsindex(SmallBasicValue* array, SmallBasicValue* index) {
    if (!array || !index) {
        return value_from_string("False");
    }

    if (array->type != SmallBasicValue::Type::Array) {
        return value_from_string("False");
    }

    const std::string indexStr = value_to_string(index);
    std::string indexLower = indexStr;
    std::ranges::transform(indexLower, indexLower.begin(), ::tolower);

    for (const auto &key: array->arrayData | std::views::keys) {
        std::string keyLower = key;
        std::ranges::transform(keyLower, keyLower.begin(), ::tolower);
        if (keyLower == indexLower) {
            return value_from_string("True");
        }
    }

    return value_from_string("False");
}

// Private

extern "C" Primitive* array_get(Primitive* array, Primitive* index) {
    if (!array || !index) return new Primitive(0.0);

    if (array->type != Primitive::Type::Array) {
        array->type = Primitive::Type::Array;
        array->arrayData.clear();
    }

    const std::string indexStr = value_to_string(index);
    std::string indexLower = indexStr;
    std::ranges::transform(indexLower, indexLower.begin(), ::tolower);

    for (auto& [key, val] : array->arrayData) {
        std::string keyLower = key;
        std::ranges::transform(keyLower, keyLower.begin(), ::tolower);
        if (keyLower == indexLower) {
            return new Primitive(*val);
        }
    }

    return new Primitive(std::string(""));
}

extern "C" Primitive* array_set(Primitive* array, Primitive* index, Primitive* value) {
    if (!index || !value) return array;

    if (!array) {
        array = new Primitive(0.0);
    }

    if (array->type != Primitive::Type::Array) {
        array->type = Primitive::Type::Array;
        array->arrayData.clear();
    }

    const std::string indexStr = value_to_string(index);
    std::string indexLower = indexStr;
    std::ranges::transform(indexLower, indexLower.begin(), ::tolower);

    for (auto& [key, val] : array->arrayData) {
        std::string keyLower = key;
        std::ranges::transform(keyLower, keyLower.begin(), ::tolower);
        if (keyLower == indexLower) {
            val = std::make_shared<Primitive>(*value);
            return array;
        }
    }

    array->arrayData[indexStr] = std::make_shared<Primitive>(*value);
    return array;
}