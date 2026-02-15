#include "utils.h"

uint32_t round_up(uint32_t number, uint32_t multiple) {
    return number + ((multiple - (number % multiple)) % multiple);
}

uint32_t round_down(uint32_t number, uint32_t multiple) {
    return number - (number % multiple);
}