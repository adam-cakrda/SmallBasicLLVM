#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <functional>

struct SmallBasicValue {
    enum class Type { Number, String, Array };

    Type type;
    double numberValue;
    std::string stringValue;
    std::unordered_map<std::string, std::shared_ptr<SmallBasicValue>> arrayData;

    SmallBasicValue() : type(Type::Number), numberValue(0.0) {}

    explicit SmallBasicValue(const double num)
        : type(Type::Number), numberValue(num) {}

    explicit SmallBasicValue(std::string str)
        : type(Type::String), numberValue(0.0), stringValue(std::move(str)) {}

    SmallBasicValue(const SmallBasicValue& other) : type(other.type), numberValue(other.numberValue),
          stringValue(other.stringValue), arrayData(other.arrayData) {}
} typedef Primitive;

extern std::vector<std::string> g_program_arguments;
extern std::unordered_map<std::string, std::unordered_map<std::string, std::function<SmallBasicValue()>>> g_property_getters;
extern std::unordered_map<std::string, std::unordered_map<std::string, std::function<void(const SmallBasicValue&)>>> g_property_setters;

extern "C" Primitive* value_from_number(double num);
extern "C" Primitive* value_from_string(const char* str);

extern "C" double value_to_number(const Primitive* val);
extern "C" const char* value_to_string(Primitive* val);

extern "C" Primitive* value_add(Primitive* left, Primitive* right);
extern "C" Primitive* value_sub(const Primitive* left, const Primitive* right);
extern "C" Primitive* value_mul(const Primitive* left, const Primitive* right);
extern "C" Primitive* value_div(const Primitive* left, const Primitive* right);

static int compare_values(Primitive* left, Primitive* right);
static int compare_arrays(Primitive* left, Primitive* right);
extern "C" int value_eq(Primitive* left, Primitive* right);
extern "C" int value_neq(Primitive* left, Primitive* right);
extern "C" int value_lt(Primitive* left, Primitive* right);
extern "C" int value_gt(Primitive* left, Primitive* right);
extern "C" int value_lte(Primitive* left, Primitive* right);
extern "C" int value_gte(Primitive* left, Primitive* right);

extern "C" Primitive* array_get(Primitive* array, Primitive* index);
extern "C" Primitive* array_set(Primitive* array, Primitive* index, Primitive* value);

extern "C" Primitive* property_get(const char* object, const char* property);
extern "C" void property_set(const char* object, const char* property, Primitive* value);

