#pragma once
#include <iostream>
#include <string>
#include <vector>

#include "value.hpp"

extern "C" void runtime_init(int argc, char** argv);
extern "C" void runtime_cleanup();