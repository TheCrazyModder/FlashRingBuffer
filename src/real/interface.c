#include "../hal/flash_hal.h"
#include "string.h"

flash_error write(uint32_t addr, const void *ptr, uint32_t len) {
    // add code here later
    return ERR_UNINITIALIZED;
}

flash_error read(uint32_t addr, void *ptr, uint32_t len) {
    // add esp code here
    return ERR_UNINITIALIZED;
}

flash_error erase(uint32_t sector) {
    return ERR_UNINITIALIZED;
}

flash_hal_t g_flash_hal = (flash_hal_t){
    .init = NULL,
    .deinit = NULL,
    .write = &write,
    .read = &read,
    .erase = &erase
};