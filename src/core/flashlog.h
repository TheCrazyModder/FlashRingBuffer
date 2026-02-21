#ifndef FLASHLOG_H
#define FLASHLOG_H

#include "../hal/flash_hal.h"
#include "../include/globals.h"

typedef struct {
    uint32_t last_record_addr;
    uint32_t last_record_seq;
    int struct_already;
} FlashlogState;

typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint32_t content_length;
    uint32_t content_crc;
    uint32_t header_crc;
} record_header; // THIS HEADER HAS TO BE A MULTIPLE OF THE FLASH_ALIGN GLOBAL CONST

static const uint32_t header_size = sizeof(record_header);
static const uint32_t max_content_length = SECTOR_SIZE - header_size - sizeof(uint32_t);

int flashlog_init(FlashlogState *state);
int flashlog_deinit();
flash_error flashlog_write(FlashlogState *state, const void * ptr, uint32_t size);
uint32_t get_latest_size(FlashlogState *state);
flash_error read_latest(FlashlogState *state, void * ptr, uint32_t max_size);

#endif