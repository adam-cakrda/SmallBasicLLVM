#include "main.h"

extern "C" void runtime_init(const int argc, char** argv) {
    g_program_arguments.clear();
    for (int i = 1; i < argc; ++i) {
        g_program_arguments.emplace_back(argv[i]);
    }
}

extern "C" void runtime_cleanup() {
    std::cout << "Press any key to continue...";
    std::cin.get();
}
