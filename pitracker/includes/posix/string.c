#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

// These implementations are all lousy, but simple and good enough for now.
//
int memcmp(const void *s1, const void *s2, size_t n) {
    // Stub
    printf("Skipping memcmp\r\n");
    usleep(10);
    return 1;
}

void *memcpy(void *dest, const void *src, size_t n) {
    for (uint32_t i=0;i<n;i++) ((uint8_t*)dest)[i] = ((uint8_t*)src)[i];
    return dest;
}

size_t strlen(const char *s) {
    int len = 0;
    while (s[len] != 0) len++;
    return len;
}

int strcmp(const char *s1, const char *s2) {
    // Don't care if strings are > or < each other, just if they match
    if (strlen(s1) != strlen(s2)) return 1;
    unsigned int c = 0;
    while (1) {
        if (s1[c] != s2[c]) return 1;
        if (s1[c] == 0 || s2[c] == 0) return 0;
        c++;
    }
}

void *memset(void *s, int c, size_t n) {
    for (uint32_t i=0;i<n;i++) ((uint8_t*)s)[i] = c;
    return s;
}

