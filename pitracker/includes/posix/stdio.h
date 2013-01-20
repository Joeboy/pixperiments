#ifndef STDIO_H
#define STDIO_H

#include <stdint.h>

void printf(const char *s);

void dump_nibble_hex(int8_t v);

void dump_byte_hex(int8_t v);

void dump_int_bin(uint32_t v);

void dump_int_hex(uint32_t v);

#endif
