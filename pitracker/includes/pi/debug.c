#include <pi/uart.h>
#include <unistd.h>
#include <stdarg.h>
#include <config.h>

static void print_literal(const char *s) {
    while (*s) uart_putc(*s++);
}

static void dump_nibble_hex(int8_t v) {
    v &= 0xf;
    if (v>9) v+=0x37; else v+=0x30;
    uart_putc(v);
}


static void dump_int_hex(int32_t v) {
    uart_putc('0');
    uart_putc('x');
    for (unsigned int i=0;i<8;i++) {
        dump_nibble_hex((v & 0xf0000000) >> 28);
        v <<= 4;
    }
}


static void dump_int_dec(int32_t v) {
    if (v < 0) {
        uart_putc('-');
        v = -v;
    }
    unsigned int digits = 1;
    unsigned int maxval = 10;
    while (v >= maxval) {
        maxval *= 10;
        digits++;
    }

    for (unsigned int i=0;i<digits;i++) {
        maxval /= 10;
        unsigned int digit_val = (float)v / maxval; // Bodge
        uart_putc(0x30 + digit_val);
        v -= digit_val * maxval;
    }
}


int debug(const char *s, ...) {
    /*
     * This is basically a crap version of printf + a delay. The default printf
     * doesn't seem to work for some reason so I repurposed my original crap printf code. */

#ifdef DEBUG

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
                    debug("debug: bad placeholder '%c'", c);
                    break;
            }
        }
        if (c == '%') state = 1;
        n++;
    }
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
                    break;
                case 'd':
                    dump_int_dec(va_arg(args, uint32_t));
                    state = 0;
                    break;
                case 'x':
                    dump_int_hex(va_arg(args, uint32_t));
                    state = 0;
                    break;
                case 's':
                    print_literal(va_arg(args, const char*));
                    state = 0;
                    break;
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
    usleep(5);
#endif
}

