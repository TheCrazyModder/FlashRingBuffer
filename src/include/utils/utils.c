#include "utils.h"

uint32_t round_up(uint32_t number, uint32_t multiple) {
    return number + ((multiple - (number % multiple)) % multiple);
}

uint32_t round_down(uint32_t number, uint32_t multiple) {
    return number - (number % multiple);
}

uint32_t max(uint32_t a, uint32_t b) {
    if (a > b) {return a;} else {return b;}
}

uint32_t min(uint32_t a, uint32_t b) {
    if (a < b) {return a;} else {return b;}
}