#ifndef FLASHLOG_H
#define FLASHLOG_H

#include "../hal/flash_hal.h"

typedef struct {
    uint32_t last_record_addr;
    uint32_t last_record_seq;
    int struct_already;
} FlashlogState;

int flashlog_init(FlashlogState *state);
int flashlog_deinit();
flash_error flashlog_write(FlashlogState *state, const void * ptr, uint32_t size);
uint32_t get_latest_size(FlashlogState *state);
flash_error read_latest(FlashlogState *state, void * ptr, uint32_t max_size);

#endif