#include "flashlog.h"
#include "../include/globals.h"
#include "../include/utils/utils.h"
#include "../include/crc/crc.h"
#include "../include/debug/debug.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stdbool.h"

typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint32_t content_length;
    uint32_t content_crc;
    uint32_t header_crc;
} record_header; // THIS HEADER HAS TO BE A MULTIPLE OF THE FLASH_ALIGN GLOBAL CONST

const uint32_t header_size = sizeof(record_header);
const uint32_t max_content_length = SECTOR_SIZE - header_size - sizeof(uint32_t);

// Returns true if a is after b in the unsigned sequence, otherwise 0
int is_after(uint32_t a, uint32_t b) {
    return (uint32_t)(a - b) < 0x80000000u;
}

void reset_header(record_header *header) {
    memset(header, 0, header_size);
}

int is_valid_header(const record_header *header) {
    // some quick checks
    if (header->content_length > max_content_length) return 0;
    if (header->content_length == 0) {return 0;}
    if (header->magic == HEADER_MAGIC) {
        if (header->header_crc == crc32_byte((uint8_t*)header, header_size)) {
            return 1;
        }
    }
    return 0;
}

// A helper function that returns the content length + the header size and the commit signature size
uint32_t get_total_record_size(uint32_t content_length) {
    return content_length + header_size + sizeof(uint32_t); // content + header + commit message
}


bool is_valid_record(uint32_t address) {
    if (address >= PARTITION_SIZE) {return false;}
    
    record_header header = {0};
    if (g_flash_hal.read(address, &header, header_size) != ERR_SUCCESS) {return false;}
    
    if (!is_valid_header(&header)) {return false;}
    
    if (header.content_length > max_content_length) {return false;}
    
    // check content crc
    size_t content_bytes_left = header.content_length;
    uint32_t crc = start_crc;
    uint32_t reading_address = address + header_size;
    
    uint8_t bytes[CRC_CHUNK];
    
    while (content_bytes_left > 0) {
        size_t read_bytes = min(content_bytes_left, CRC_CHUNK);
        
        if (g_flash_hal.read(reading_address, bytes, read_bytes) != ERR_SUCCESS) {return false;}
        
        crc = crc32_byte_seq(crc, bytes, read_bytes);
        content_bytes_left -= read_bytes;
        reading_address += read_bytes;
    }
    
    crc = crc32_finalize(crc);
    if (crc != header.content_crc) {return false;}
    
    uint32_t commit = 0;
    if (g_flash_hal.read(address + header_size + header.content_length, &commit, sizeof(uint32_t)) != ERR_SUCCESS) {return false;}
    if (commit != COMMIT_MAGIC) {return false;}
    
    return true;
}

int flashlog_init(FlashlogState *state) {
    if (!state) {return -1;}
    if (!g_flash_hal.init || !g_flash_hal.read || !g_flash_hal.write || !g_flash_hal.erase) {
        return -1;
    }
    if (g_flash_hal.init() != 0) {
        return -1;
    }
    
    // we first do a quick elimination scan to determine which sectors have valid records
    // this method will eliminate sectors that don't have valid records at the first address.
    // Theoretically this could falsly eliminate sectors where only the first record is corrupt
    // but the rest are fine in the case of a corruption from something other than writing a record
    
    uint16_t num_sectors = (int)(PARTITION_SIZE / SECTOR_SIZE);
    
    uint32_t highest_sequence_index = 0;
    uint32_t highest_sequence = 0;
    
    bool found_records = false;
    
    record_header header;
    
    for (uint32_t sector = 0; sector < num_sectors; sector++) {
        
        uint32_t address = sector * SECTOR_SIZE;
        
        if (address >= PARTITION_SIZE - header_size - sizeof(uint32_t)) {break;}
        
        if (!is_valid_record(address)) {
            debug_print("Scanning sector %u at address %u found no record\n", sector, address);
            uint8_t first_byte = 0;
            g_flash_hal.read(address, &first_byte, sizeof(uint8_t));
            debug_print("First debug byte: %hd\n", first_byte);
            continue;
        }
        
        found_records = true; 
        
        reset_header(&header); // reset the header to avoid any ghost values
        
        if (g_flash_hal.read(address, &header, header_size) != ERR_SUCCESS) {continue;}
        
        if (is_after(header.sequence, highest_sequence)) {
            highest_sequence_index = sector;
            highest_sequence = header.sequence;
        }
    }
    
    // if we didn't find any records than set to the default blank state
    
    if (!found_records) {
        state->last_record_addr = 0; 
        state->last_record_seq = 0; 
        state->struct_already = 0;
        return 0;
    }
    
    // if we have a sector to scan, scan for the address and sequence of the latest record
    
    uint32_t address = highest_sequence_index * SECTOR_SIZE;
    
    while (true) {
        reset_header(&header);
        if (!is_valid_record(address)) {break;}
        if (g_flash_hal.read(address, &header, header_size) != ERR_SUCCESS) {break;}
        
        if (is_after(header.sequence, highest_sequence)) {
            highest_sequence = header.sequence;
        }
        
        address += get_total_record_size(header.content_length);
        
        if (address + header_size + sizeof(uint32_t) >= SECTOR_SIZE * highest_sequence_index) {break;}
    }
    
    state->last_record_addr = address;
    state->last_record_seq = highest_sequence;
    state->struct_already = 1;
    
    return 0;
}

int flashlog_deinit() {
    if (g_flash_hal.deinit) {g_flash_hal.deinit();}
    return 0;
}

flash_error flashlog_write(FlashlogState *state, const void *ptr, uint32_t size) {
    if (ptr == NULL) {return ERR_NULL_PTR;}
    if (size + header_size > SECTOR_SIZE) {return ERR_OUT_OF_BOUNDS;}
    
    // now we have a record that will fit in a sector
    uint32_t sector_bytes_left = round_up(state->last_record_addr, SECTOR_SIZE) - (SECTOR_SIZE - state->last_record_addr);
    
    // we could techinically split writes between sectors but for now to keep logic simple
    // we will just skip to the next one
    // 
    // TODO
    // add split writes for sectors
    
    uint32_t write_addr = 0;
    
    if (state->struct_already) {
        if (sector_bytes_left < get_total_record_size(size)) {
            debug_print("writing %u bytes, sector has %u bytes left\n", get_total_record_size(size), sector_bytes_left);
            write_addr = round_up(state->last_record_addr, SECTOR_SIZE);
            g_flash_hal.erase(write_addr / SECTOR_SIZE);
            
            debug_print("state has records, skipping to next sector address: %u\n", write_addr);
        } else {
            record_header last_header = {0};
            g_flash_hal.read(state->last_record_addr, &last_header, header_size); // this header shouldn't be random data as the initial memory scan would catch it
            write_addr = state->last_record_addr + get_total_record_size(last_header.content_length);
            
            debug_print("state has records, address: %u\n", write_addr);
        }
    } else {
        debug_print("state has no records: starting at %u\n", write_addr);
    }
    
    
    record_header header = {0};
    header.magic = HEADER_MAGIC;
    header.sequence = state->last_record_seq + 1;
    header.content_crc = crc32_byte((uint8_t*)ptr, size);
    header.content_length = size;
    
    uint8_t error = 0;
    
    debug_print("Writing header to %u\n", write_addr);
    
    error = g_flash_hal.write(write_addr, &header, header_size);
    if (error != 0) {
        debug_print("Error writing header: %i\n", error);
        return error;
    }
    
    debug_print("Writing content to %u\n", write_addr + header_size);
    
    error = g_flash_hal.write(write_addr + header_size, ptr, size);
    if (error != 0) {
        debug_print("Error content header: %i\n", error);
        return error;
    }
    
    error = g_flash_hal.write(write_addr + header_size + size, &COMMIT_MAGIC, sizeof(uint32_t));
    if (error != 0) {
        debug_print("Error commit magic: %i\n", error);
        return error;
    }
    
    debug_print("Completed the write\n");
    
    state->last_record_addr = write_addr;
    state->last_record_seq++;
    state->struct_already = 1;
    
    return error;
}

uint32_t get_latest_size(FlashlogState *state) {
    record_header header = {0};
    g_flash_hal.read(state->last_record_addr, &header, header_size);
    return header.content_length;
}

flash_error read_latest(FlashlogState *state, void *ptr, uint32_t max_size) {
    if (max_size == 0) {return ERR_INVALID_ARGUMENT;}
    if (ptr == NULL) {return ERR_NULL_PTR;}
    
    if (!state->struct_already) {return ERR_NO_RECORD;}
    
    record_header header = {0};
    
    debug_print("Reading header from %u\n", state->last_record_addr);
    g_flash_hal.read(state->last_record_addr, &header, header_size);
    
    if (crc32_byte((uint8_t*)&header, header_size) != header.header_crc) {
        return ERR_NO_RECORD; // temp for now
    }
    
    uint32_t read_size = max_size;
    
    if (header.content_length < max_size) {
        read_size = header.content_length;
    }
    
    debug_print("Reading content from %u\n", state->last_record_addr + header_size);
    return g_flash_hal.read(state->last_record_addr + header_size, ptr, read_size);
}