#ifndef UTILS_H
#define UTILS_H

#include "stdint.h"

uint32_t round_up(uint32_t number, uint32_t multiple);
uint32_t round_down(uint32_t number, uint32_t multiple);

uint32_t max(uint32_t a, uint32_t b);
uint32_t min(uint32_t a, uint32_t b);

#endif