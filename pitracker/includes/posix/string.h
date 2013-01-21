#ifndef STRING_H
#define STRING_H

#include <stddef.h>

// These implementations are all lousy, but simple and good enough for now.
//
int memcmp(const void *s1, const void *s2, size_t n);

void *memcpy(void *dest, const void *src, size_t n);

size_t strlen(const char *s);

int strcmp(const char *s1, const char *s2);

void *memset(void *s, int c, size_t n);

#endif
