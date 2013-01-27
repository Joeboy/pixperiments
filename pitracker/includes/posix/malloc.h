/* (non-)implementation of malloc with no working free().
 * Good enough for now */

#ifndef MALLOC_H
#define MALLOC_H

#include <stdint.h>
#include <stddef.h>

// TODO:
struct mem_control_block {
 int is_available;
 int size;
};

void setup_heap();

void* malloc(size_t size);

void free(void* ptr); // Note: this is currently a no-op

#endif
