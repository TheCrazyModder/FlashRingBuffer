#ifndef FLASH_HAL_H
#define FLASH_HAL_H

#include "../include/errors.h"
#include "stdint.h"

typedef struct {
    // This function is expected to return 0 if successful. 
    // any other value will be returned to the application from the flashlog_init function
    int (*init)();
    
    // this function has no return value. it is mainly here for something like freeing allocated memory
    // in cases of the PC memory simulation or similar
    void (*deinit)();
    
    // this function is expected to pad the writes to align with the FLASH_ALIGN variable
    flash_error (*write)(uint32_t, const void * ptr, uint32_t len);
    
    flash_error (*read) (uint32_t, void * ptr, uint32_t len);
    
    flash_error (*erase)(uint32_t);
    
} flash_hal_t;

extern flash_hal_t g_flash_hal;

#endif