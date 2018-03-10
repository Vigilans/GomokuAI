//
// pch.cpp
// Include the standard header and generate the precompiled header.
//

#include "pch.h"
#include <ctime>

// seed random generator
int rndSeed = []() {
    srand(time(nullptr));
    return 0;
}();