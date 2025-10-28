#include "linker.hpp"
#include <spdlog/spdlog.h>
#include <cstdlib>

#ifdef _WIN32
    #include <windows.h>
#elif __linux__
    #include <unistd.h>
#endif

Linker::Linker(DiagnosticReporter &diag) : reporter(diag) {
#ifdef _WIN32
    compilers = {"clang++", "g++"}; // "link" (MSVC) not supported yet
#elif __linux__
    compilers = {"clang++", "g++", "c++"};
#endif
}

void Linker::link(const std::string &object, const std::string &output, const std::string &pathStd) {
    const std::string compiler = detect_compiler();
    const std::string stdPath = find_std(pathStd);

    const std::string command = compiler + " " + object + " " + stdPath + " -o " + output;
    const int result = std::system(command.c_str());

    if (result != 0) {
        std::exit(result);
    }
}

std::string Linker::detect_compiler() const {
    for (const auto &compiler : compilers) {
#ifdef _WIN32
        std::string command = compiler + " --version >nul 2>&1";
#elif __linux__
        std::string command = compiler + " --version >/dev/null 2>&1";
#endif
        if (std::system(command.c_str()) == 0) {
            return compiler;
        }
    }

    spdlog::error("No supported C++ compilers found in system. Recommended: gcc/clang");
    std::exit(1);
}

std::string Linker::find_std(const std::string &path) {
    std::filesystem::path stdPath;

    if (path.empty()) {
        char pBuf[512] = {};

#ifdef _WIN32
        GetModuleFileNameA(nullptr, pBuf, sizeof(pBuf));
#elif __linux__
        ssize_t count = readlink("/proc/self/exe", pBuf, sizeof(pBuf) - 1);
        if (count == -1) {
            spdlog::error("Cannot find program directory.");
            std::exit(1);
        }
        pBuf[count] = '\0';
#endif

        const std::filesystem::path folderPath = std::filesystem::path(pBuf).parent_path();
        stdPath = folderPath / "libSmallBasicLibrary.a";
    } else {
        stdPath = path;
        if (!std::filesystem::exists(stdPath)) {
            spdlog::error("{} doesn't exist", path);
            std::exit(1);
        }
    }

    if (std::filesystem::exists(stdPath)) {
        return stdPath.string();
    }

    spdlog::error("libSmallBasicLibrary.a not found");
    std::exit(1);
}
