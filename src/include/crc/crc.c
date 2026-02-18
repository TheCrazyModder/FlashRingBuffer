#include "crc.h"

const uint32_t start_crc = 0xFFFFFFFFu;

uint32_t crc32_finalize(uint32_t crc) {
    return crc ^ 0xFFFFFFFFu;
}

uint32_t crc32_byte_seq(uint32_t crc, const uint8_t *p, uint32_t len) {
    while (len--) {crc = poly8_lookup[(uint8_t)(crc ^ *p++)] ^ (crc >> 8);}
    return crc;
}

uint32_t crc32_byte(const uint8_t *p, uint32_t len) {
	return crc32_finalize(crc32_byte_seq(start_crc, p, len));
}

