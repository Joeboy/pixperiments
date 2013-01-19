#ifndef STDIO_H
#define STDIO_H

#include <pi/uart.c>


void printf(const char *s) {
    // Format specifiers (%d) not implemented
    char c;
    while(1) {
        c = *s;
        if (c == 0) return;
        uart_putc(c);
        s++;
    }
}

void dump_nibble_hex(int8_t v) {
    v &= 0xf;
    if (v>9) v+=0x37; else v+=0x30;
    uart_putc(v);
}

void dump_byte_hex(int8_t v) {
    dump_nibble_hex((v & 0xf0) >> 4);
    dump_nibble_hex((v & 0x0f));
}


void dump_int_bin(uint32_t v) {
    // Dump a binary representation of an unsigned int
    uint32_t j;
    for (j=0;j<32;j++) {
        uart_putc('0'+ !!(v & 1<<31));
        v <<= 1;
    }
    uart_putc(0x0D);
    uart_putc(0x0A);
}


void dump_int_hex(uint32_t v) {
    uart_putc('0');
    uart_putc('x');
    for (unsigned int i=0;i<8;i++) {
        dump_nibble_hex((v & 0xf0000000) >> 28);
        v <<= 4;
    }
}

#endif
