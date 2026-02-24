#include "flashlog.h"
#include "../include/utils/utils.h"
#include "../include/crc/crc.h"
#include "../include/debug/debug.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stdbool.h"

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
        //if (header->header_crc == crc32_byte((uint8_t*)header, header_size)) {
            return 1;
        //}
    }
    return 0;
}

// A helper function that returns the content length + the header size and the commit signature size
uint32_t get_total_record_size(uint32_t content_length) {
    return content_length + header_size + sizeof(uint32_t); // content + header + commit message
}

record_state is_valid_record(uint32_t address) {
    if (address >= PARTITION_SIZE) {return RECORD_NO_EXIST;}
    
    record_header header = {0};
    if (g_flash_hal.read(address, &header, header_size) != ERR_SUCCESS) {return RECORD_READ_ERROR;}
    
    if (!is_valid_header(&header)) {return RECORD_NO_EXIST;}
    
    if (header.content_length > max_content_length) {return RECORD_HEADER_BOUNDS;}
    
    // check content crc
    size_t content_bytes_left = header.content_length;
    uint32_t crc = start_crc;
    uint32_t reading_address = address + header_size;
    
    uint8_t bytes[CRC_CHUNK];
    
    while (content_bytes_left > 0) {
        size_t read_bytes = min(content_bytes_left, CRC_CHUNK);
        
        if (g_flash_hal.read(reading_address, bytes, read_bytes) != ERR_SUCCESS) {return RECORD_READ_ERROR;}
        
        crc = crc32_byte_seq(crc, bytes, read_bytes);
        content_bytes_left -= read_bytes;
        reading_address += read_bytes;
    }
    
    crc = crc32_finalize(crc);
    if (crc != header.content_crc) {return RECORD_CRC_INVALID;}
    
    uint32_t commit = 0;
    if (g_flash_hal.read(round_up(address + header_size + header.content_length, FLASH_ALIGN), &commit, sizeof(uint32_t)) != ERR_SUCCESS) {return RECORD_READ_ERROR;}
    if (commit != COMMIT_MAGIC) {return RECORD_INVALID_COMMIT;}
    
    return RECORD_VALID;
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
        
        debug_print("Scanning sector %u at address %u\n", sector, address);
        
        record_state error = is_valid_record(address);
        
        if (error != RECORD_VALID) {
            debug_print("found no record, error %u\n", error);
            
            // this is all just temporary code to try and iron out some bugs
            // TODO remove
            record_header header = {0};
            uint8_t first_byte = 0;
            uint32_t error = g_flash_hal.read(address, &first_byte, sizeof(uint8_t));
            if (error != ERR_SUCCESS) {
                debug_print("Error reading first byte of sector. Error %u\n", error); // continue for now
            };
            debug_print("First debug byte: %hd\n", first_byte);
            
            error = g_flash_hal.read(address, &header, header_size);
            if (error != ERR_SUCCESS) {
                debug_print("Error reading header from start of sector. Error %u\n", error); // continue for now
            };
            
            record_state record_valid = is_valid_record(address);
            
            if (record_valid == RECORD_VALID) {
                debug_print("\nRaw header from sector start\n");
                debug_print("Magic %u\n", header.magic);
                debug_print("Sequence %u\n", header.sequence);
                debug_print("Content length %u\n", header.content_length);
                debug_print("Content crc %u\n", header.content_crc);
                debug_print("Header crc %u\n\n", header.header_crc);
            }
            
            continue;
        }
        
        found_records = true; 
        
        reset_header(&header); // reset the header to avoid any ghost values
        
        if (g_flash_hal.read(address, &header, header_size) != ERR_SUCCESS) {continue;}
        
        if (is_after(header.sequence, highest_sequence)) {
            debug_print("Found header with seq %u, last one %u\n", header.sequence, highest_sequence);
            highest_sequence_index = sector;
            highest_sequence = header.sequence;
        }
    }
    
    // if we didn't find any records than set to the default blank state
    
    if (!found_records) {
        debug_print("Didn't find any records, setting to default\n");
        state->last_record_addr = 0; 
        state->last_record_seq = 0; 
        state->struct_already = 0;
        return 0;
    }
    
    // if we have a sector to scan, scan for the address and sequence of the latest record
    
    uint32_t address = highest_sequence_index * SECTOR_SIZE;
    uint32_t max_sector_address = address + SECTOR_SIZE;
    
    debug_print("Starting narrow scan at %u\n", address);
    while (true) {
        reset_header(&header);
        
        debug_print("Narrow scanning address: %u\n", address);
        
        record_state record = is_valid_record(address);
        
        if (record != RECORD_VALID) {
            debug_print("Error %u reading record from address: %u, stopping scan\n", record, address);
            break;
        }
        
        if (g_flash_hal.read(address, &header, header_size) != ERR_SUCCESS) {
            debug_print("Error reading header from address %u\n", address);
            break;
        }
        
        debug_print("Found header at addr: %u, next scan at %u\n", address, address + get_total_record_size(header.content_length));
        
        if (is_after(header.sequence, highest_sequence)) {
            highest_sequence = header.sequence;
        }
        
        address += get_total_record_size(header.content_length);
        
        uint32_t min_next = address + header_size + sizeof(uint32_t);
        
        if (min_next >= max_sector_address) {
            debug_print("Address %u is outside of sector max %u, breaking\n", address, max_sector_address);
            break;
        }
    }
    
    state->last_record_addr = round_up(address, FLASH_ALIGN);
    state->last_record_seq = highest_sequence;
    state->struct_already = 1;
    
    debug_print("Found records, setting to addr: %u, seq: %u\n", address, highest_sequence);
    
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
            write_addr = round_up(state->last_record_addr + get_total_record_size(last_header.content_length), FLASH_ALIGN);
            
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
    header.header_crc = 0xFF; // temp for now. TODO add calculation for other fields later
    
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
    
    error = g_flash_hal.write(round_up(write_addr + header_size + size, FLASH_ALIGN), &COMMIT_MAGIC, sizeof(uint32_t));
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
        //return ERR_NO_RECORD; // temp for now
    }
    
    uint32_t read_size = min(max_size, header.content_length);
    
    // check for commit signature
    uint32_t commit_address = round_up(state->last_record_addr + header_size + header.content_length, FLASH_ALIGN);
    uint32_t commit = 0;
    
    if (g_flash_hal.read(commit_address, &commit, sizeof(uint32_t)) != ERR_SUCCESS) {return ERR_CORRUPT;}
    
    if (commit != COMMIT_MAGIC) {return ERR_NO_COMMIT;}
    
    debug_print("Reading content from %u\n", state->last_record_addr + header_size);
    
    return g_flash_hal.read(state->last_record_addr + header_size, ptr, read_size);
}