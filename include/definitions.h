#ifndef FLAGSER_DEFINITIONS_H // F
#define FLAGSER_DEFINITIONS_H

// LIBRARIES
#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <deque>
#include <sstream>
#include <cmath>

// Libraries needed for cnpy
#include <stdexcept>
#include <cstdio>
#include <typeinfo>
#include <cassert>
#include <map>
#include <memory>
#include <stdint.h>
#include <numeric>
#include <complex>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <stdexcept>
#include <regex>


//##############################################################################
// TYPE DEFINTIONS
#ifndef MANY_VERTICES
    typedef unsigned short vertex_index_t;
    typedef int32_t index_t;
#else
    typedef uint64_t vertex_index_t;
    typedef int64_t index_t;
#endif
typedef float value_t;



//##############################################################################
// Class for thread safe printing
class LockIO {
    static std::mutex& mutex();
    std::lock_guard<std::mutex> guard;
    public:
        LockIO() : guard(mutex()) {}
};

inline std::mutex& LockIO::mutex() {
    static std::mutex m;
    return m;
}


#endif // FLAGSER_DEFINITIONS_H
