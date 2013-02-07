#include <pi/uart.h>

#include <stdint.h>
#include <stdarg.h>


void print_literal(const char *s) {
    while (*s) putc(*s++);
}

void dump_nibble_hex(int8_t v) {
    v &= 0xf;
    if (v>9) v+=0x37; else v+=0x30;
    putc(v);
}

void dump_byte_hex(int8_t v) {
    dump_nibble_hex((v & 0xf0) >> 4);
    dump_nibble_hex((v & 0x0f));
}


void dump_int_bin(uint32_t v) {
    // Dump a binary representation of an unsigned int
    uint32_t j;
    for (j=0;j<32;j++) {
        putc('0'+ !!(v & 1<<31));
        v <<= 1;
    }
    putc(0x0D);
    putc(0x0A);
}


void dump_int_hex(int32_t v) {
    putc('0');
    putc('x');
    for (unsigned int i=0;i<8;i++) {
        dump_nibble_hex((v & 0xf0000000) >> 28);
        v <<= 4;
    }
}


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
                case 's':
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
                    putc((char)va_arg(args, uint32_t));
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
                case 's':
                    print_literal(va_arg(args, const char*));
                    state = 0;
                    argi++;
                default:
                    break;
            }
        } else {
            if (c == '%') state = 1;
            else putc(c);
        }
        n++;
    }
    va_end(args);
}

