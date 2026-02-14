#ifndef SIM_H
#define SIM_H

#include "../include/errors.h"
#include "stdint.h"

int init();
void deinit();

flash_error write(uint32_t addr, uint8_t * ptr, uint32_t len);
flash_error read(uint32_t addr, uint8_t * ptr, uint32_t len);
flash_error erase(uint32_t sector);

#endif SIM_H