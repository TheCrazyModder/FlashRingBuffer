#ifndef FLASH_HAL_H
#define FLASH_HAL_H

#include "../include/errors.h"
#include "stdint.h"

typedef struct {
    int (*init)();
    void (*deinit)();
    
    // Write function by default will zero pad to keep the writes to the FLASH_ALIGN
    flash_error (*write)(uint32_t, const void * ptr, uint32_t len);
    flash_error (*read) (uint32_t, void * ptr, uint32_t len);
    flash_error (*erase)(uint32_t);
} flash_hal_t;

extern flash_hal_t g_flash_hal;

#endif