#ifndef FLASH_HAL_H
#define FLASH_HAL_H

#include "../include/errors.h"
#include "stdint.h"

typedef struct {
    int (*init)();
    void (*deinit)();
    flash_error (*write)(uint32_t, uint8_t * ptr, uint32_t len);
    flash_error (*read) (uint32_t, uint8_t * ptr, uint32_t len);
    flash_error (*erase)(uint32_t);
} flash_hal_t;

#endif