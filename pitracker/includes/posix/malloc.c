/* (non-)implementation of malloc with no working free().
 * Good enough for now */

#include <stdint.h>
#include <pi/hardware.h>
#include <stddef.h>

#include <malloc.h>

static unsigned int heap_end;

void setup_heap() {
    // called by _start
    heap_end = 0x20000;
}

void* malloc(size_t size) {
    unsigned int prev_heap_end = heap_end;
    heap_end += size;
    return (void*)prev_heap_end;
}

void free(void* ptr) {
    // Do nothing, cos we suck
}

