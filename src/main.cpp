#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include <spdlog/spdlog.h>
#include <fmt/ranges.h>
#include <cxxopts.hpp>
#include <filesystem>

#include "lexer/token.hpp"
#include "lexer/lexer.hpp"
#include "diagnostic.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"
#include "codegen/codegen.hpp"

std::string readFile(const std::string &filePath);

void exportTokens(const std::vector<Token>& tokens, const std::string& outFile);

void exportAST(const ASTNode& ast, const std::string& outFile);

std::string getModuleName(const std::string& filename);

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

    cxxopts::Options options("SmallBasicLLVM", "LLVM Compiler for SmallBasic");
    options.add_options()
        ("export-tokens", "Export tokens to file", cxxopts::value<std::string>())
        ("export-ast", "Export AST to file", cxxopts::value<std::string>())
        ("o,output", "Output file", cxxopts::value<std::string>()->default_value("output.ll"))
        ("h,help", "Print usage");

    auto result = options.parse(argc - 1, argv + 1);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    spdlog::info(" --- SmallBasicLLVM Compiler {} ---", VERSION);

    std::string source = readFile(filename);
    DiagnosticReporter diag(source, filename);

    spdlog::info("[1/4] Lexing");

    Lexer lexer;
    const std::vector<Token> tokens = lexer.tokenize(source, diag);

    diag.printDiagnostics();
    if (diag.hasErrorsOccurred()) return 1;

    if (result.count("export-tokens")) {
        exportTokens(tokens, result["export-tokens"].as<std::string>());
    }

    spdlog::info("[2/4] Parsing");

    Parser parser(tokens, diag);
    auto ast = parser.parse();

    if (!ast) {
        diag.addError("Parsing failed!", SourceLocation(1, 1, 0));
        diag.printDiagnostics();
        return 1;
    }

    if (spdlog::get_level() == spdlog::level::debug) {
        spdlog::debug("AST:");
        ast->print(std::cout);
    }

    if (result.count("export-ast")) {
        exportAST(*ast, result["export-ast"].as<std::string>());
    }

    diag.printDiagnostics();
    if (diag.hasErrorsOccurred()) return 1;

    spdlog::info("[3/4] Analyzing");

    SemanticAnalyzer analyzer(diag);
    analyzer.analyze(*ast);

    diag.printDiagnostics();
    if (diag.hasErrorsOccurred()) return 1;

    spdlog::info("[4/4] Codegen");

    CodeGenerator codegen(diag);

    const std::string moduleName = getModuleName(filename);

    if (!codegen.generate(*ast, moduleName)) {
        diag.printDiagnostics();
        return 1;
    }

    diag.printDiagnostics();
    if (diag.hasErrorsOccurred()) return 1;

    auto outputFile = result["output"].as<std::string>();
    codegen.emit(outputFile);

    spdlog::info("Compiled {}.sb!", moduleName);
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

std::string getModuleName(const std::string& filename) {
    const std::filesystem::path p(filename);
    return p.stem().string();
}

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
}

void exportAST(const ASTNode& ast, const std::string& outFile) {
    std::ofstream file(outFile);
    if (!file.is_open()) {
        spdlog::error("Could not open file for writing AST: {}", outFile);
        return;
    }
    ast.print(file);
}