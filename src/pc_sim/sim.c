#include "sim.h"
#include "stdlib.h"
#include "../include/globals.h"
#include "../include/utils/utils.h"
#include "../hal/flash_hal.h"
#include "../include/debug/debug.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint8_t *memory = NULL;

flash_hal_t g_flash_hal = (flash_hal_t){
    .init = &init,
    .deinit = &deinit,
    .write = &write,
    .read = &read,
    .erase = &erase
};

int init() {
    memory = (uint8_t *)malloc(PARTITION_SIZE);
    if (memory == NULL) {
        return -1;
    }
    
    memset(memory, 0xFF, PARTITION_SIZE);
    
    //debug_print("Byte 0 after init: %u\n", memory[0]);
    
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


flash_error write(uint32_t addr, const void *ptr, uint32_t len) {
    if (!initialized()) {return ERR_UNINITIALIZED;}
    
    //debug_print("Byte 0: %u\n", memory[0]);
    
    // enforce flash rules
    if (addr % FLASH_ALIGN != 0) {return ERR_INVALID_ALIGN;}
    
    // for now we will silently pad 0 - 3 extra bytes to keep it aligned
    // TODO: Document this
    // 
    //if (len % FLASH_ALIGN != 0) {return ERR_INVALID_ALIGN;}
    if (len == 0) {return ERR_INVALID_ALIGN;}
    
    // make sure we aren't out of bounds, while also accounting for padding
    if (addr > PARTITION_SIZE || round_up(len, FLASH_ALIGN) > PARTITION_SIZE - addr) {return ERR_OUT_OF_BOUNDS;}
    
    if (ptr == NULL) {return ERR_NULL_PTR;}
    
    // right now we are enforcing that a bit cant go from 0 -> 1
    for (uint32_t i = 0; i < len; i++) {
        uint8_t new = ((const uint8_t*)ptr)[i];
        uint8_t old = memory[addr + i];
        if (old != 0xFF) { // just debugging the first write for now
            debug_print("Byte found at %i\n", addr + i);
        }
        
        if (((uint8_t)~old) & new) { // check if it's a 0 -> 1
            //return ERR_BIT_CLEAR;
        }
        
        memory[addr + i] = new;
    }
    
    // silently pad the write as real hardware often requires alignment
    uint8_t bytes_left = round_up(len, FLASH_ALIGN) - len;
    while (bytes_left > 0) {
        bytes_left--;
        memory[addr + len + bytes_left] = 0xFF;
    }
    
    return ERR_SUCCESS;
}

flash_error read(uint32_t addr, void *ptr, uint32_t len) {
    if (!initialized()) {return ERR_UNINITIALIZED;}
    
    if (len == 0) {return ERR_INVALID_ALIGN;}
    
    // make sure we aren't out of bounds
    if (addr + len > PARTITION_SIZE) {return ERR_OUT_OF_BOUNDS;}
    
    if (ptr == NULL) {return ERR_NULL_PTR;}
    
    memcpy(ptr, memory + addr, len);
    
    // VERY TEMP
    for (int i = addr; i < addr + len; i++) {
        debug_print("%i: %u\n", i, memory[i]);
    }
    
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