#include <pi/uart.h>

#include <stdint.h>
#include <stdarg.h>


void printf(const char *s, ...) {
    // This is a really, really sketchy implementation
    // No type checking / error handling.
    // Only handles %c, %x and %d (badly) so far. Let's fix it up as we go along.
    // Handle with care!
    unsigned int n=0, num_args=0;
    unsigned int state = 0;
    char c;
    va_list args;

    while(s[n]) {
        c = s[n];
        if (state == 1) {
            switch (c) {
                case 'c':
                    num_args++;
                    state = 0;
                    break;
                case 'd':
                    num_args++;
                    state = 0;
                    break;
                case 'x':
                    num_args++;
                    state = 0;
                    break;
                default:
                    printf("printf: bad placeholder '%c'", c);
                    break;
            }
        }
        if (c == '%') state = 1;
        n++;
    }
    unsigned int argi = 0;
    n = 0;
    va_start(args, num_args);
    state = 0;
    while (s[n]) {
        c = s[n];
        if (state == 1) {
            switch(c) {
                case 'c':
                    uart_putc((char)va_arg(args, uint32_t));
                    state = 0;
                    argi++;
                case 'd':
                    dump_int_hex(va_arg(args, uint32_t));
                    state = 0;
                    argi++;
                case 'x':
                    dump_int_hex(va_arg(args, uint32_t));
                    state = 0;
                    argi++;
                default:
                    break;
            }
        } else {
            if (c == '%') state = 1;
            else uart_putc(c);
        }
        n++;
    }
    va_end(args);
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


void dump_int_hex(int32_t v) {
    uart_putc('0');
    uart_putc('x');
    for (unsigned int i=0;i<8;i++) {
        dump_nibble_hex((v & 0xf0000000) >> 28);
        v <<= 4;
    }
}

