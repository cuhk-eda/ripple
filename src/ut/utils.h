#ifndef _UTILS_H_
#define _UTILS_H_

#include "log.h"
#include "timer.h"
#include "geo.h"

#include <algorithm>
#include <climits>
#include <cstdarg>
#include <string>

#define _ZERO 1e-10

#define RoundDn(x) (floor((x) + _ZERO))
#define RoundUp(x) (ceil((x)-_ZERO))

#define range(x, lo, hi) (max((lo), min((hi), (x))))

//#define isNaN(x) ((x)!=(x))

namespace util {
template <typename T>
inline void minmax(const T v1, const T v2, T &minv, T &maxv) {
    if (v1 < v2) {
        minv = v1;
        maxv = v2;
    } else {
        minv = v2;
        maxv = v1;
    }
}
template <typename T>
inline void bounds(const T v, T &minv, T &maxv) {
    if (v < minv) {
        minv = v;
    } else if (v > maxv) {
        maxv = v;
    }
}
}  // namespace util

// function for contain / overlap

inline int binContainedL(int lx, int blx, int bhx, int binw) { return (std::max(blx, lx) - blx + binw - 1) / binw; }
inline int binContainedR(int hx, int blx, int bhx, int binw) { return (std::min(bhx, hx) - blx) / binw - 1; }
inline int binOverlappedL(int lx, int blx, int bhx, int binw) { return (std::max(blx, lx) - blx) / binw; }
inline int binOverlappedR(int hx, int blx, int bhx, int binw) {
    return (std::min(bhx, hx) - blx + binw - 1) / binw - 1;
}

////////////INTEGER PACKING/////////////////
inline long long packInt(const int x, const int y) { return ((long)x) << 32 | ((long)y); }
inline void unpackInt(int &x, int &y, long long i) {
    x = (int)(i >> 32);
    y = (int)(i & 0xffffffff);
}
inline long long packCoor(int x, int y) {
    int ix = x + (INT_MAX >> 2);
    int iy = y + (INT_MAX >> 2);
    return packInt(ix, iy);
}
inline void unpackCoor(int &x, int &y, long long i) {
    unpackInt(x, y, i);
    x -= (INT_MAX >> 2);
    y -= (INT_MAX >> 2);
}

////////////FLOATING-POINT RANDOM NUMBER//////////////
inline int getrand(int lo, int hi) { return (rand() % (hi - lo + 1)) + lo; }
inline double getrand(double lo, double hi) { return (((double)rand() / (double)RAND_MAX) * (hi - lo) + lo); }

//////////////BIT MANIPULATION///////////////////////

template <typename T>
inline void setBit(T &val, T bit) {
    val |= bit;
}
template <typename T>
inline void unsetBit(T &val, T bit) {
    val &= (~bit);
}
template <typename T>
inline void toggleBit(T &val, T bit) {
    val ^= bit;
}
template <typename T>
inline bool isSetBit(T val, T bit) {
    return (val & bit) > 0;
}
template <typename T>
inline T getBit(T val, T bit) {
    return val & bit;
}

double rect_overlap_area(
    double alx, double aly, double ahx, double ahy, double blx, double bly, double bhx, double bhy);

//////////ESCAPE CODE DEFINIATION//////////
#define ESC_RESET "\033[0m"
#define ESC_BOLD "\033[1m"
#define ESC_UNDERLINE "\033[4m"
#define ESC_BLINK "\033[5m"
#define ESC_REVERSE "\033[7m"
#define ESC_BLACK "\033[30m"
#define ESC_RED "\033[31m"
#define ESC_GREEN "\033[32m"
#define ESC_YELLOW "\033[33m"
#define ESC_BLUE "\033[34m"
#define ESC_MAGENTA "\033[35m"
#define ESC_CYAN "\033[36m"
#define ESC_WHITE "\033[37m"
#define ESC_DEFAULT "\033[39m"
#define ESC_BG_BLACK "\033[40m"
#define ESC_BG_RED "\033[41m"
#define ESC_BG_GREEN "\033[42m"
#define ESC_BG_YELLOW "\033[43m"
#define ESC_BG_BLUE "\033[44m"
#define ESC_BG_MAGENTA "\033[45m"
#define ESC_BG_CYAN "\033[46m"
#define ESC_BG_WHITE "\033[47m"
#define ESC_BG_DEFAULT "\033[49m"
#define ESC_OVERLINE "\033[53m"

////////MEMORY CONSUMPTION////////////

class ProcessStat {
public:
    enum Field {
        PID = 0,   // process ID
        COMM = 1,  // command
        STATE = 2,
        PPID = 3,
        PGRP = 4,
        SESSION = 5,
        TTY_NR = 6,
        TPGID = 7,
        FLAGS = 8,
        MINFLT = 9,  // minor page faults
        CMINFLT = 10,
        MAJFLT = 11,  // major page faults
        CMAJFLT = 12,

        UTIME = 13,
        STIME = 14,
        CUTIME = 15,
        CSTIME = 16,

        PRIORITY = 17,
        NICE = 18,
        NUM_THREADS = 19,
        ITREALVALUE = 20,
        STARTTIME = 21,

        VSIZE = 22,  // virtual set size (memory allocated)
        RSS = 23,    // resident set size (physical memory in use)
        RSSLIM = 24,
        STARTCODE = 25,
        ENDCODE = 26,
        STARTSTACK = 27,
        KSTKESP = 28,
        KSTKEIP = 29,

        SIGNAL = 30,
        BLOCKED = 31,
        SIGIGNORE = 32,
        SIGCATCH = 33,
        WCHAN = 34,
        NSWAP = 35,
        CNSWAP = 36,
        EXIT_SIGNAL = 37,
        PROCESSOR = 38,
        RT_PRIORITY = 39,
        POLICY = 40,
        DELAYACCT_BLKIO_TICKS = 41,
        GUEST_TIME = 42,
        CGUEST_TIME = 43,
        START_DATA = 44,
        END_DATA = 45,
        START_BRK = 46,
        ARG_START = 47,
        ARG_END = 48,
        ENV_START = 49,
        ENV_END = 50,
        EXIT_CODE = 51
    };
    int pid;
    std::string comm;
};
#define STAT_ALL 0
#define STAT_MEM 1
#define STAT_TIME 2
std::string show_mem_size(long memsize);
void print_process_stat(int stat);

#endif
