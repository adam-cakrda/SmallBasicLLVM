#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <algorithm>

enum class ParamType {
    Number,
    String,
    Any
};

enum class ReturnType {
    Void,
    Number,
    String
};

struct FunctionInfo {
    std::vector<ParamType> params;
    ReturnType returnType;
};

using FunctionRegistry  = std::unordered_map<std::string, std::unordered_map<std::string, FunctionInfo>>;
struct PropertyInfo {
    ReturnType returnType;
    bool readOnly;
};

using PropertyRegistry  = std::unordered_map<std::string, std::unordered_map<std::string, PropertyInfo>>;

struct Registry {
    FunctionRegistry functions = {
        {"textwindow", {
            {"writeline", {{ParamType::Any}, ReturnType::Void}},
            {"write", {{ParamType::Any}, ReturnType::Void}},
            {"read", {{}, ReturnType::String}}
        }},
        {"math", {
            {"abs", {{ParamType::Number}, ReturnType::Number}}
        }}
    };

    PropertyRegistry properties = {
        {"textwindow", {
                {"title", {ReturnType::String, false}}
        }},
        {"clock", {
            {"date", {ReturnType::Number, true}}
        }},
    };

    static std::string toLower(const std::string& s) {
        std::string out;
        out.resize(s.size());
        std::ranges::transform(s, out.begin(), [](const unsigned char c){ return static_cast<char>(std::tolower(c)); });
        return out;
    }

    bool hasObject(const std::string& obj) const {
        const auto object = toLower(obj);

        return functions.contains(object) || properties.contains(object);
    }

    bool hasFunction(const std::string& obj, const std::string& function) const {
        const auto object = toLower(obj);
        const auto func = toLower(function);

        const auto it = functions.find(object);
        return it != functions.end() && it->second.contains(func);
    }

    bool hasProperty(const std::string& obj, const std::string& property) const {
        const auto object = toLower(obj);
        const auto prop = toLower(property);

        const auto it = properties.find(object);
        return it != properties.end() && it->second.contains(prop);
    }

    std::optional<FunctionInfo> getFunction(const std::string& obj, const std::string& function) const {
        const auto object = toLower(obj);
        const auto func = toLower(function);

        const auto it = functions.find(object);
        if (it == functions.end()) return std::nullopt;
        const auto jt = it->second.find(func);
        if (jt == it->second.end()) return std::nullopt;
        return jt->second;
    }

    std::optional<PropertyInfo> getProperty(const std::string& obj, const std::string& property) const {
        const auto object = toLower(obj);
        const auto prop = toLower(property);

        const auto it = properties.find(object);
        if (it == properties.end()) return std::nullopt;
        const auto jt = it->second.find(prop);
        if (jt == it->second.end()) return std::nullopt;
        return jt->second;
    }

    bool validateFunctionCall(const std::string& object, const std::string& func,
                              const std::vector<ParamType>& args) const {
        const auto info = getFunction(object, func);
        if (!info) return false;

        const auto& expected = info->params;
        if (args.size() != expected.size()) return false;

        for (size_t i = 0; i < args.size(); ++i) {
            if (expected[i] == ParamType::Any) continue;
            if (args[i] == expected[i]) continue;
            return false; // type mismatch
        }
        return true;
    }

    static std::string toString(const ParamType type) {
        switch (type) {
            case ParamType::Number: return "Number";
            case ParamType::String: return "String";
            case ParamType::Any:    return "Any";
        }
        return "Unknown";
    }

    static std::string toString(const ReturnType type) {
        switch (type) {
            case ReturnType::Void:   return "Void";
            case ReturnType::Number: return "Number";
            case ReturnType::String: return "String";
        }
        return "Unknown";
    }
};
