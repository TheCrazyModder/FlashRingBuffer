#ifndef FLASH_HAL_H
#define FLASH_HAL_H

#include "../include/errors.h"
#include "stdint.h"

typedef struct {
    flash_error (*write)(uint32_t, void * ptr, uint32_t len);
    flash_error (*read) (uint32_t, void * ptr, uint32_t len);
    flash_error (*erase)(uint32_t);
} flash_hal_t;

#endif