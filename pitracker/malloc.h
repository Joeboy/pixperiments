/* (non-)implementation of malloc with no working free().
 * Good enough for now */

#ifndef MALLOC_H
#define MALLOC_H

#include <stdint.h>
#include "pi_hardware.h"

unsigned int heap_end;

void setup_heap() {
    // called by _start
    heap_end = 0x20000;
}


// TODO:
struct mem_control_block {
 int is_available;
 int size;
};

void* malloc(size_t size) {
    unsigned int prev_heap_end = heap_end;
    heap_end += size;
    return (void*)prev_heap_end;
}

void free(void* ptr) {
    // Do nothing, cos we suck
}

#endif
