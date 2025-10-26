#include <ranges>

#include "value.hpp"

extern "C" SmallBasicValue* array_count(SmallBasicValue* array) {
    if (!array || array->type != SmallBasicValue::Type::Array) {
        return new SmallBasicValue(0.0);
    }
    return new SmallBasicValue(static_cast<double>(array->arrayData.size()));
}

extern "C" int array_containsindex(SmallBasicValue* array, SmallBasicValue* index) {
    if (!array || !index || array->type != SmallBasicValue::Type::Array) {
        return 0;
    }

    const std::string indexStr = value_to_string(index);
    std::string indexLower = indexStr;
    std::ranges::transform(indexLower, indexLower.begin(), ::tolower);

    for (const auto &key: array->arrayData | std::views::keys) {
        std::string keyLower = key;
        std::ranges::transform(keyLower, keyLower.begin(), ::tolower);
        if (keyLower == indexLower) {
            return 1;
        }
    }

    return 0;
}

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
    
    return new Primitive(0.0);
}

extern "C" void array_set(Primitive* array, Primitive* index, Primitive* value) {
    if (!array || !index || !value) return;
    
    if (array->type != Primitive::Type::Array) {
        array->type = Primitive::Type::Array;
        array->arrayData.clear();
    }

    const std::string indexStr = value_to_string(index);
    array->arrayData[indexStr] = std::make_shared<Primitive>(*value);
}