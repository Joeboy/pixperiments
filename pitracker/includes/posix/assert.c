#include "stdio.h"
#include <stdint.h>

void assert(uint8_t expr) {
    if (!expr) printf("Assert failed\r\n");
}
