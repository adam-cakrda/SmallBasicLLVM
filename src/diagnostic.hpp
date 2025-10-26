#pragma once
#include <string>
#include <utility>
#include <vector>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

enum class DiagnosticLevel {
    Error,
    Warning,
    Note
};

struct SourceLocation {
    size_t line;
    size_t column;
    size_t length;

    SourceLocation(const size_t l, const size_t c, const size_t len = 1)
        : line(l), column(c), length(len) {}
};

struct Diagnostic {
    DiagnosticLevel level;
    std::string message;
    SourceLocation location;
    std::string hint;

    Diagnostic(const DiagnosticLevel lvl, std::string  msg, const SourceLocation &loc, std::string  h = "")
        : level(lvl), message(std::move(msg)), location(loc), hint(std::move(h)) {
#ifdef _WIN32
        // enable colors
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        GetConsoleMode(hErr, &dwMode);
        SetConsoleMode(hErr, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
    }
};

class DiagnosticReporter {
private:
    std::vector<Diagnostic> diagnostics;
    std::string source;
    std::string filename;
    bool hasErrors = false;

public:
    DiagnosticReporter(std::string  src, std::string  fname)
        : source(std::move(src)), filename(std::move(fname)) {}

    void addError(const std::string& message, SourceLocation location, const std::string& hint = "") {
        diagnostics.emplace_back(DiagnosticLevel::Error, message, location, hint);
        hasErrors = true;
    }

    void addWarning(const std::string& message, SourceLocation location, const std::string& hint = "") {
        diagnostics.emplace_back(DiagnosticLevel::Warning, message, location, hint);
    }

    void addNote(const std::string& message, SourceLocation location, const std::string& hint = "") {
        diagnostics.emplace_back(DiagnosticLevel::Note, message, location, hint);
    }

    [[nodiscard]] bool hasErrorsOccurred() const {
        return hasErrors;
    }

    [[nodiscard]] std::string getLine(const size_t lineNum) const {
        size_t currentLine = 1;
        size_t start = 0;

        for (size_t i = 0; i < source.length(); i++) {
            if (currentLine == lineNum) {
                size_t end = i;
                while (end < source.length() && source[end] != '\n') {
                    end++;
                }
                return source.substr(start, end - start);
            }
            if (source[i] == '\n') {
                currentLine++;
                start = i + 1;
            }
        }
        return "";
    }

    void printDiagnostics() const {
        for (const auto& diag : diagnostics) {
            printDiagnostic(diag);
        }

        if (hasErrors) {
            size_t errorCount = 0;
            for (const auto& diag : diagnostics) {
                if (diag.level == DiagnosticLevel::Error) {
                    errorCount++;
                }
            }

            std::cerr << "\033[1;31merror\033[0m: could not compile `" << filename << "` due to ";
            if (errorCount == 1) {
                std::cerr << "previous error";
            } else {
                std::cerr << errorCount << " previous errors";
            }
            std::cerr << std::endl;
        }
    }

private:
    void printDiagnostic(const Diagnostic& diag) const {
        std::string levelStr;
        std::string colorCode;

        switch (diag.level) {
            case DiagnosticLevel::Error:
                levelStr = "error";
                colorCode = "\033[1;31m"; // Bold red
                break;
            case DiagnosticLevel::Warning:
                levelStr = "warning";
                colorCode = "\033[1;33m"; // Bold yellow
                break;
            case DiagnosticLevel::Note:
                levelStr = "note";
                colorCode = "\033[1;36m"; // Bold cyan
                break;
        }

        std::cerr << colorCode << levelStr << "\033[0m: " << diag.message << std::endl;
        std::cerr << "  \033[1;34m-->\033[0m " << filename << ":"
                  << diag.location.line << ":" << diag.location.column << std::endl;

        std::string line = getLine(diag.location.line);
        size_t lineNumWidth = std::to_string(diag.location.line).length();

        std::cerr << std::string(lineNumWidth + 2, ' ') << "\033[1;34m|\033[0m" << std::endl;
        std::cerr << " \033[1;34m" << diag.location.line << " |\033[0m " << line << std::endl;

        std::cerr << std::string(lineNumWidth + 1, ' ') << " \033[1;34m|\033[0m "
                  << std::string(diag.location.column, ' ')
                  << colorCode << std::string(diag.location.length, '^') << "\033[0m";

        if (!diag.hint.empty()) {
            std::cerr << " " << diag.hint;
        }

        std::cerr << std::endl;
        std::cerr << std::string(lineNumWidth + 2, ' ') << "\033[1;34m|\033[0m" << std::endl;
    }
};
