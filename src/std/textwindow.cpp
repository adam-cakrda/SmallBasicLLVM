#include <iostream>

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

extern "C" Primitive* textwindow_title_get() {
    return new Primitive("test");
}

extern "C" void textwindow_title_set(Primitive* value) {
    std::cout << "test:" << value_to_string(value) << std::endl;
}