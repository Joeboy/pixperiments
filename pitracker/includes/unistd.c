#include <stdint.h>

extern void dummy(uint32_t);

void usleep(unsigned int t) {
    // Pause for about t ms
    int i;
    for (;t>0;t--) {
        for (i=5000;i>0;i--) dummy(i);
    }
}

