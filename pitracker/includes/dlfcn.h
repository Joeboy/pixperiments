#ifndef DLFCN_H
#define DLFCN_H

void *dlopen(const char *filename, int flag);
void *dlsym(void *handle, const char *symbol);

// TODO:
//char *dlerror(void);
//int dlclose(void *handle);

#endif
