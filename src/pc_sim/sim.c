#include "sim.h"
#include "stdlib.h"
#include "../include/globals.h"
#include "../include/utils/utils.h"
#include "../hal/flash_hal.h"
#include "../include/debug/debug.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* file_path = "flash.bin";

uint8_t *memory = NULL;

FILE *file;

flash_hal_t g_flash_hal = (flash_hal_t){
    .init = &init,
    .deinit = &deinit,
    .write = &write,
    .read = &read,
    .erase = &erase
};

long get_file_size(FILE *file) {
    if (file == NULL) {return 0;}
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    return size;
}

int init() {
    memory = (uint8_t *)malloc(PARTITION_SIZE);
    if (!memory) {return -1;}
    
    memset(memory, 0xFF, PARTITION_SIZE);
    
    // load the simulated flash file
    file = fopen(file_path, "rb");
    if (!file) {  // either return or handle with a debug message
        // return -1;
        debug_print("Error opening file, creating file now\n");
        file = fopen(file_path, "w+b");
    } else {
        long size = get_file_size(file);
        size_t bytes = fread(memory, 1, min(size, PARTITION_SIZE), file); // read the file into our memory struct
        if (bytes != size) {return -1;} // EOF or error
    }
    fclose(file);
    file = NULL;
    

    return 0;
}

int initialized() {
    return (!(memory == NULL) && !(file == NULL));
}

void deinit() {
    if (memory) {
        file = fopen(file_path, "wb");
        if (file) {
            size_t written = fwrite(memory, 1, PARTITION_SIZE, file);
            if (written != PARTITION_SIZE) {debug_print("Error writing memory to file\n");}
            fclose(file);
            file = NULL;
        } else {
            debug_print("Error saving file\n");
        }
        free(memory);
        memory = NULL;
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
        //debug_print("%i: %u\n", i, memory[i]);
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