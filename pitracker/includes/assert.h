#ifndef ASSERT_H
#define ASSERT_H

#include "stdio.h"

void assert(uint8_t expr) {
    if (!expr) printf("Assert failed\r\n");
}

#endif
