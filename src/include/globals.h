#ifndef GLOBALS_H
#define GLOBALS_H

#define SECTOR_SIZE 4096 // the size of the sectors in flash
#define FLASH_ALIGN 4 // esp often enforces a byte align for writing and reading
#define PARTITION_SIZE 65536 // our custom flash partition size 
#define MAX_WRITE_SIZE 256 // max amount of bytes we can write in a singe call

#define HEADER_MAGIC = 0x4D474943; // ascii MGIC
#define COMMIT_MAGIC = 0x434D4954; // ascii CMIT

#endif