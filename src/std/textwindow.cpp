#include <iostream>

#ifdef __linux__

#elif _WIN32
    #include <windows.h>
#endif

#include "value.hpp"

extern "C" void textwindow_writeline(Primitive* val) {
    if (!val) {
        std::cout << std::endl;
        return;
    }
    
    std::cout << value_to_string(val) << std::endl;
}

extern "C" void textwindow_write(Primitive* val) {
    if (!val) { return; }

    std::cout << value_to_string(val);
}

extern "C" Primitive* textwindow_read() {
    std::string input;
    std::getline(std::cin, input);
    return new Primitive(input);
}

extern "C" void textwindow_pause() {
    std::cout << "Press any key to continue...";
    std::cin.get();
}
extern "C" void textwindow_clear() {
#ifdef _WIN32
    system("cls");
#elif __LINUX__
    system("clear");
#endif
}

extern "C" Primitive* textwindow_title_get() {
    std::string result = "Not implemented";
#ifdef __linux__
    // on linux it is very tricky
#elif _WIN32
    char wnd_title[256];
    HWND hwnd=GetForegroundWindow();
    GetWindowText(hwnd,wnd_title,sizeof(wnd_title));
    result = wnd_title;
#endif
    return value_from_string(result.c_str());
}

extern "C" void textwindow_title_set(Primitive* value) {
#ifdef __linux__
    std::cout << "\033]0;" << value_to_string(value) << "\007";
#elif _WIN32
    SetConsoleTitle(value_to_string(value));
#endif
}