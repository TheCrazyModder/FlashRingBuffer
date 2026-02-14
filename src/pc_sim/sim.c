#include "sim.h"
#include "stdlib.h"
#include "../include/globals.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../hal/flash_hal.h"

uint8_t *memory;

flash_hal_t g_flash_hal = (flash_hal_t){
    .init = &init,
    .deinit = &deinit,
    .write = &write,
    .read = &read,
    .erase = &erase
};

int init() {
    memory = (uint8_t *)calloc(PARTITION_SIZE, 1);
    if (memory == NULL) {
        return -1;
    }
    return 0;
}

int initialized() {
    return !(memory == NULL);
}

void deinit() {
    if (memory != NULL) {
        free(memory);
    }
}


flash_error write(uint32_t addr, uint8_t *ptr, uint32_t len) {
    if (!initialized()) {return ERR_UNINITIALIZED;}
    
    // enforce flash rules
    if (addr % FLASH_ALIGN != 0) {return ERR_INVALID_ALIGN;}
    if (len % FLASH_ALIGN != 0) {return ERR_INVALID_ALIGN;}
    if (len == 0) {return ERR_INVALID_ALIGN;}
    
    // make sure we aren't out of bounds
    if (addr + len > PARTITION_SIZE) {return ERR_OUT_OF_BOUNDS;}
    
    if (ptr == NULL) {return ERR_NULL_PTR;}
    
    // right now we are enforcing that a bit cant go from 0 -> 1
    for (uint32_t i = 0; i < len; i++) {
        uint8_t new = (uint8_t)ptr[i];
        uint8_t old = memory[addr + len];
        
        if (((uint8_t)~old) & new) { // check if its a 0 -> 1
            return ERR_BIT_CLEAR;
        }
        
        memory[addr + len] = new;
        
    }
    return ERR_SUCCESS;
}

flash_error read(uint32_t addr, uint8_t *ptr, uint32_t len) {
    if (!initialized()) {return ERR_UNINITIALIZED;}
    
    // enforce flash rules
    if (addr % FLASH_ALIGN != 0) {return ERR_INVALID_ALIGN;}
    if (len % FLASH_ALIGN != 0) {return ERR_INVALID_ALIGN;}
    if (len == 0) {return ERR_INVALID_ALIGN;}
    
    // make sure we aren't out of bounds
    if (addr + len > PARTITION_SIZE) {return ERR_OUT_OF_BOUNDS;}
    
    if (ptr == NULL) {return ERR_NULL_PTR;}
    
    memcpy(ptr, memory, len);
    return ERR_SUCCESS;
}

flash_error erase(uint32_t sector) {
    if (!initialized()) {return ERR_UNINITIALIZED;}
    if (sector > (PARTITION_SIZE / SECTOR_SIZE)) {
        return ERR_OUT_OF_BOUNDS;
    }
    
    uint32_t start = sector * SECTOR_SIZE;
    
    memset(&memory[start], 0, SECTOR_SIZE);
    return ERR_SUCCESS;
}