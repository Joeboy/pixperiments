/* (non-)implementation of malloc with no working free().
 * Good enough for now */

#ifndef MALLOC_H
#define MALLOC_H

#include <stdint.h>
#include "pi_hardware.h"

unsigned int heap_end=0x0020000;
unsigned int prev_heap_end;

// TODO:
struct mem_control_block {
 int is_available;
 int size;
};

void* malloc(size_t size) {
    prev_heap_end = heap_end;
    heap_end += size;
    pause (5);
    return (void*)prev_heap_end;
}

void free(void* ptr) {
    // Do nothing, cos we suck
}

#endif
