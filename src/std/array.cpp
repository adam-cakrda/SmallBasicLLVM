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

extern "C" SmallBasicValue* array_getallindices(SmallBasicValue* array) {
    auto* result = new SmallBasicValue();
    result->type = SmallBasicValue::Type::Array;
    
    if (!array || array->type != SmallBasicValue::Type::Array) {
        return result;
    }

    std::vector<std::string> keys;
    for (const auto& key : array->arrayData | std::views::keys) {
        keys.push_back(key);
    }
    std::ranges::sort(keys);

    int index = 1;
    for (const auto& key : keys) {
        result->arrayData[std::to_string(index)] = std::make_shared<SmallBasicValue>(key);
        index++;
    }
    
    return result;
}

extern "C" SmallBasicValue* array_containsvalue(SmallBasicValue* array, SmallBasicValue* value) {
    if (!array || !value) {
        return value_from_string("False");
    }

    if (array->type != SmallBasicValue::Type::Array) {
        return value_from_string("False");
    }

    const std::string valueStr = value_to_string(value);
    std::string valueLower = valueStr;
    std::ranges::transform(valueLower, valueLower.begin(), ::tolower);

    for (const auto& val : array->arrayData | std::views::values) {
        const std::string currentStr = value_to_string(val.get());
        std::string currentLower = currentStr;
        std::ranges::transform(currentLower, currentLower.begin(), ::tolower);
        if (currentLower == valueLower) {
            return value_from_string("True");
        }
    }

    return value_from_string("False");
}

extern "C" SmallBasicValue* array_isarray(const SmallBasicValue* value) {
    if (!value) {
        return value_from_string("False");
    }

    if (value->type == SmallBasicValue::Type::Array) {
        return value_from_string("True");
    }

    return value_from_string("False");
}

// Old api

static std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<Primitive>>> g_legacy_arrays;

extern "C" void array_setvalue(SmallBasicValue* arrayName, SmallBasicValue* index, SmallBasicValue* value) {
    if (!arrayName || !index || !value) return;

    const std::string arrayNameStr = value_to_string(arrayName);
    std::string arrayNameLower = arrayNameStr;
    std::ranges::transform(arrayNameLower, arrayNameLower.begin(), ::tolower);

    const std::string indexStr = value_to_string(index);
    std::string indexLower = indexStr;
    std::ranges::transform(indexLower, indexLower.begin(), ::tolower);

    g_legacy_arrays[arrayNameLower][indexStr] = std::make_shared<Primitive>(*value);
}

extern "C" SmallBasicValue* array_getvalue(SmallBasicValue* arrayName, SmallBasicValue* index) {
    if (!arrayName || !index) return value_from_string("");

    const std::string arrayNameStr = value_to_string(arrayName);
    std::string arrayNameLower = arrayNameStr;
    std::ranges::transform(arrayNameLower, arrayNameLower.begin(), ::tolower);

    const std::string indexStr = value_to_string(index);
    std::string indexLower = indexStr;
    std::ranges::transform(indexLower, indexLower.begin(), ::tolower);

    auto arrayIt = g_legacy_arrays.find(arrayNameLower);
    if (arrayIt == g_legacy_arrays.end()) {
        return value_from_string("");
    }

    for (const auto& [key, val] : arrayIt->second) {
        std::string keyLower = key;
        std::ranges::transform(keyLower, keyLower.begin(), ::tolower);
        if (keyLower == indexLower) {
            return new Primitive(*val);
        }
    }

    return value_from_string("");
}

extern "C" void array_removevalue(SmallBasicValue* arrayName, SmallBasicValue* index) {
    if (!arrayName || !index) return;

    const std::string arrayNameStr = value_to_string(arrayName);
    std::string arrayNameLower = arrayNameStr;
    std::ranges::transform(arrayNameLower, arrayNameLower.begin(), ::tolower);

    const std::string indexStr = value_to_string(index);
    std::string indexLower = indexStr;
    std::ranges::transform(indexLower, indexLower.begin(), ::tolower);

    const auto arrayIt = g_legacy_arrays.find(arrayNameLower);
    if (arrayIt == g_legacy_arrays.end()) {
        return;
    }

    for (auto it = arrayIt->second.begin(); it != arrayIt->second.end(); ) {
        std::string keyLower = it->first;
        std::ranges::transform(keyLower, keyLower.begin(), ::tolower);
        if (keyLower == indexLower) {
            it = arrayIt->second.erase(it);
        } else {
            ++it;
        }
    }
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

