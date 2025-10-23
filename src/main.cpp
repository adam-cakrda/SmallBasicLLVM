#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include <spdlog/spdlog.h>
#include <fmt/ranges.h>
#include <cxxopts.hpp>

#include "lexer/token.hpp"
#include "lexer/lexer.hpp"
#include "diagnostic.hpp"

std::string readFile(const std::string &filePath);

void exportTokens(const std::vector<Token>& tokens, const std::string& outFile) {
    std::ofstream file(outFile);
    if (!file.is_open()) {
        spdlog::error("Could not open file for writing tokens: {}", outFile);
        return;
    }
    for (const auto& token : tokens) {
        file << "[" << token.line << ":" << token.column << "] "
             << tokenTypeToString(token.type) << " : '" << token.value << "'\n";
    }
    spdlog::info("Tokens exported to {}", outFile);
}

int main(int argc, char** argv) {
#ifdef NDEBUG
    spdlog::set_level(spdlog::level::info);
#else
    spdlog::set_level(spdlog::level::debug);
#endif

    if (argc < 2) {
        spdlog::error("Usage: {} <source_file> [--export-tokens <file>] [--export-ast <file>] [--output <file>]", argv[0]);
        return 1;
    }

    std::string filename = argv[1];

    cxxopts::Options options("SmallBasicLLVM", "Compiler for SmallBasicLLVM");
    options.add_options()
        ("export-tokens", "Export tokens to file", cxxopts::value<std::string>())
        ("o,output", "Output file", cxxopts::value<std::string>()->default_value("output.ll"))
        ("h,help", "Print usage");

    auto result = options.parse(argc - 1, argv + 1);

    spdlog::info(" --- SmallBasicLLVM Compiler {} ---", VERSION);

    std::string source = readFile(filename);
    DiagnosticReporter diag(source, filename);

    Lexer lexer;
    const std::vector<Token> tokens = lexer.tokenize(source, diag);

    diag.printDiagnostics();
    if (diag.hasErrorsOccurred()) return 1;

    if (result.count("export-tokens")) {
        exportTokens(tokens, result["export-tokens"].as<std::string>());
    }

    spdlog::info("[1/4] Lexing successful!");

    return 0;
}

std::string readFile(const std::string &filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        spdlog::error("Could not open file: {}", filePath);
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}