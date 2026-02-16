#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG 0

#include "stdio.h"

#define debug_print(fmt, ...) \
    do { if (DEBUG) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

#endif