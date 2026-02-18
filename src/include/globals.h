#ifndef GLOBALS_H
#define GLOBALS_H

#include "stdint.h"

#define CRC_CHUNK 512 // used for limiting stack usage when streaming in bytes for crc checking
#define SECTOR_SIZE 4096 // the size of the sectors in flash
#define FLASH_ALIGN 4 // esp often enforces a byte align for writing
#define PARTITION_SIZE 65536 // our custom flash partition size 

static const uint32_t HEADER_MAGIC = 0x4D474943; // ascii MGIC
static const uint32_t COMMIT_MAGIC = 0x434D4954; // ascii CMIT

#endif