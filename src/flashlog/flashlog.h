#ifndef FLASHLOG_H
#define FLASHLOG_H

#include "../hal/flash_hal.h"

flash_error write(void * ptr, uint32_t size);
uint32_t get_latest_size();
flash_error read_latest(void * ptr);

#endif