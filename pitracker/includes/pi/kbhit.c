// http://linux-sxs.org/programming/kbhit.html

#include "kbhit.h"
#include <pi/uart.h>

void init_keyboard() {
}

void close_keyboard() {
}

int kbhit() {
    return uart_input_ready();
}

int readch() {
    return uart_read();
}
