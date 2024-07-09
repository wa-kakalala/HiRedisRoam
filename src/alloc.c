#include "alloc.h"

hiredisAllocFuncs hiredisAllocFns = {
    .mallocFn = malloc,
    .callocFn = calloc,
    .reallocFn = realloc,
    .strdupFn = strdup,
    .freeFn = free,
};

/* Override hiredis' allocators with ones supplied by the user */
hiredisAllocFuncs hiredisSetAllocators(hiredisAllocFuncs *override) {
    hiredisAllocFuncs orig = hiredisAllocFns;

    hiredisAllocFns = *override;

    return orig;
}

/* Reset allocators to use libc defaults */
void hiredisResetAllocators(void) {
    hiredisAllocFns = (hiredisAllocFuncs) {
        .mallocFn = malloc,
        .callocFn = calloc,
        .reallocFn = realloc,
        .strdupFn = strdup,
        .freeFn = free,
    };
}

#ifdef _WIN32

void *hi_malloc(size_t size) {
    return hiredisAllocFns.mallocFn(size);
}

void *hi_calloc(size_t nmemb, size_t size) {
    /* Overflow check as the user can specify any arbitrary allocator */
    if (SIZE_MAX / size < nmemb)
        return NULL;

    return hiredisAllocFns.callocFn(nmemb, size);
}

void *hi_realloc(void *ptr, size_t size) {
    return hiredisAllocFns.reallocFn(ptr, size);
}

char *hi_strdup(const char *str) {
    return hiredisAllocFns.strdupFn(str);
}

void hi_free(void *ptr) {
    hiredisAllocFns.freeFn(ptr);
}

#endif


void *hi_calloc(size_t nmemb, size_t size) {
    /* Overflow check as the user can specify any arbitrary allocator */
    if (SIZE_MAX / size < nmemb)
        return NULL;

    return hiredisAllocFns.callocFn(nmemb, size);
}