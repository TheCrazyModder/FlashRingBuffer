#ifndef SIM_H
#define SIM_H

#include "../include/errors.h"
#include "stdint.h"

flash_error write(uint32_t addr, void * ptr, uint32_t len);
flash_error read(uint32_t addr, void * ptr, uint32_t len);
flash_error erase(uint32_t sector);

#endif SIM_H