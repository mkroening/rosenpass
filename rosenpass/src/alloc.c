#include <reent.h>
#include <stdalign.h>
#include <stddef.h>
#include <sys/param.h>

#include "syscall.h"

// Align address upwards.
//
// `align` must be a power of two
size_t _hermit_align_up(size_t addr, size_t align) {
    size_t align_mask = align - 1;
    if ((addr & align_mask) == 0) {
        // already aligned
        return addr;
    } else {
        return (addr | align_mask) + 1;
    }
}

void *malloc(size_t size) {
    size_t mem_align = alignof(max_align_t);
    size_t mem_size = mem_align + size;

    void *mem = sys_alloc(mem_size, mem_align);

    if (mem == NULL) {
        return NULL;
    }

    *((size_t *)mem) = mem_size;

    return mem + mem_align;
}

void *calloc(size_t num, size_t size) {
    size_t mem_align = alignof(max_align_t);
    size_t mem_size = mem_align + _hermit_align_up(num * size, mem_align);

    void *mem = sys_alloc_zeroed(mem_size, mem_align);

    if (mem == NULL) {
        return NULL;
    }

    *((size_t *)mem) = mem_size;

    return mem + mem_align;
}

void *realloc(void *ptr, size_t new_size) {
    size_t mem_align = alignof(max_align_t);
    size_t mem_size = mem_align + new_size;

    if (ptr == NULL) {
        return malloc(mem_size);
    }

    void *mem = ptr - mem_align;
    size_t size = *((size_t *)mem);

    void *new_mem = sys_realloc(mem, size, mem_align, mem_size);

    if (new_mem == NULL) {
        return NULL;
    }

    *((size_t *)new_mem) = mem_size;

    return new_mem + mem_align;
}

void free(void *ptr) {
    printf("\n\nptr = %p\n", ptr);

    if (ptr == NULL) {
        return;
    }

    size_t mem_align = alignof(max_align_t);
    void *mem = ptr - mem_align;
    size_t mem_size = *((size_t *)mem);

    sys_dealloc(mem, mem_size, mem_align);
}

void free_sized(void *ptr, size_t size) {
    if (ptr == NULL) {
        return;
    }

    size_t mem_align = alignof(max_align_t);
    size_t mem_size = mem_align + size;

    sys_dealloc(ptr, mem_size, mem_align);
}

void free_aligned_size(void *ptr, size_t alignment, size_t size) {
    if (ptr == NULL) {
        return;
    }

    size_t mem_align = MAX(alignment, alignof(size_t));
    void *mem = ptr - mem_align;
    size_t mem_size = mem_align + size;

    sys_dealloc(mem, mem_size, mem_align);
}

void *aligned_alloc(size_t alignment, size_t size) {
    printf("\n\nalignment = %zu, size = %zu\n", alignment, size);
    size_t mem_align = MAX(alignment, alignof(size_t));
    size_t mem_size = mem_align + size;

    void *mem = sys_alloc(mem_size, mem_align);

    if (mem == NULL) {
        return NULL;
    }

    *((size_t *)mem) = mem_size;

    return mem + mem_align;
}

void* _malloc_r(struct _reent* reent, size_t size) {
   return malloc(size);
}

void* _calloc_r(struct _reent* reent, size_t num, size_t size) {
   return calloc(num, size);
}

void* _realloc_r(struct _reent* reent, void* ptr, size_t new_size) {
   return realloc(ptr, new_size);
}

void _free_r(struct _reent* reent, void* ptr) {
   free(ptr);
}
