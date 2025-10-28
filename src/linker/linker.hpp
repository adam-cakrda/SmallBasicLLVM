#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include "../diagnostic.hpp"

class Linker {
public:
    explicit Linker(DiagnosticReporter &diag);

    void link(const std::string &object, const std::string &output, const std::string &pathStd = "");

private:
    DiagnosticReporter& reporter;

    std::vector<std::string> compilers;

    std::string detect_compiler() const;
    std::string find_std(const std::string &path);
};
