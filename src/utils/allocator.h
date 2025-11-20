#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* Assert is optional, but recommended to guarantee some invariants. */
#include <assert.h>

#include <stdlib.h>

#ifndef NULL
#define NULL 0
#endif /* #ifndef NULL */

#include "macros.h"
#include "typedefs.h"

/* The basic data structure that represents a generic allocator */
typedef struct {
    /* Allocates size bytes */
    void *(*alloc)(void *ctx, const size_t size);
    /* (Optional) Reallocates old_size to new_size (if possible) */
    void *(*realloc)(void *ctx, void *ptr, const size_t old_size, const size_t new_size);
    /* (Optional) Reclaims the memory pointed by ptr to the allocator */
    void (*free)(void *ctx, void *ptr, const size_t size);
    /* (Optional) Reclaims all the memory that was allocated within the given context */
    void (*free_all)(void *ctx);
} allocator_iface;

typedef struct {
    const allocator_iface *interface;
    void                  *alloc_ctx;
} allocator_t;

void *allocator_alloc(const allocator_t *allocator, size_t size) {
    return allocator->interface->alloc(allocator->alloc_ctx, size);
}

void *allocator_realloc(const allocator_t *allocator, void *ptr, const size_t old_size, const size_t new_size) {
    return allocator->interface->realloc(allocator->alloc_ctx, ptr, old_size, new_size);
}

void allocator_free(const allocator_t *allocator, void *ptr, const size_t size) {
    return allocator->interface->free(allocator->alloc_ctx, ptr, size);
}

void allocator_free_all(const allocator_t *allocator) {
    return allocator->interface->free_all(allocator->alloc_ctx);
}

#ifdef ALLOC_STD_IMPL

#include <stdlib.h>

internal void *std_alloc(void *ctx, const size_t size) {
    UNUSED(ctx);
    return malloc(size);
}

internal void *std_realloc(void *ctx, void *ptr, const size_t old_size, const size_t new_size) {
    UNUSED(ctx);
    UNUSED(old_size);
    return realloc(ptr, new_size);
}

internal void std_free(void *ctx, void *ptr, const size_t size) {
    UNUSED(ctx);
    UNUSED(size);
    return free(ptr);
}

global_var const allocator_iface _global_std_allocator_interface = {
    .alloc = std_alloc,
    .realloc = std_realloc,
    .free = std_free
};

global_var const allocator_t global_std_allocator = {
    .interface = &_global_std_allocator_interface,
    .alloc_ctx = NULL
};

#endif /* #ifdef ALLOC_STD_IMPL */

#define ALLOC_ARENA_IMPL
#ifdef ALLOC_ARENA_IMPL

#ifndef ARENA_DEFAULT_ALIGN
#define ARENA_DEFAULT_ALIGN (2 * sizeof (void *))
#endif /* #ifndef ARENA_CHUNK_DEFAULT_CAP */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>

enum arena_flags {
    /* If set, allows the creation of multiple regions when it gets full */
    ARENA_GROWABLE        = (1<<0),
    /* Prioritize speed even if it generates fragmentation when using multiple regions */
    ARENA_FAST_ALLOC      = (1<<1),
    /* Possible backends (mutually exclusive) */
    ARENA_VIRTUAL_BACKEND = (1<<2),
    ARENA_MALLOC_BACKEND  = (1<<3),
    ARENA_BUFFER_BACKEND  = (1<<4),
    ARENA_CUSTOM_BACKEND  = (1<<5),
};

typedef struct arena_context_t arena_context_t;
typedef struct arena_region_t arena_region_t;

struct arena_region_t {
    arena_region_t *next;
    size_t offset;
    size_t capacity;

    uint8_t data[];
};

struct arena_context_t {
    struct arena_region_t *begin;
    struct arena_region_t *end;
    enum arena_flags flags;
    /* If the backend is a custom allocator */
    allocator_iface *optional_allocator;
    allocator_iface *optional_alloc_ctx;
};

/* 
 * Allocates a new arena with a minimum capacity of ensure_capacity using the given flags.
 * Returns null if the allocation failed. Can only be used to allocate self-managed arenas,
 * for arenas with a external buffer backend, use arena_from_buf instead.
 *
 * ensure_capacity - The minimum capacity of the arena.
 * flags           - Flags the determine the behaviour of the arena.
 *
 * Returns:
 *     A pointer to the newly allocated arena context (NULL if the allocation failed).
 */
internal inline arena_context_t arena_init(const size_t ensure_capacity, enum arena_flags flags,
        allocator_iface *optional_allocator, void *optional_alloc_ctx);

/* 
 * Creates an arena that uses an existing memory buffer supplied by the caller. No allocation is
 * performed; the arena simply wraps the buffer and its management is up to the caller. The buffer
 * lifetime must be greater than or equal to the arena's lifetime, otherwise there will be undefined
 * behaviour.
 *
 * Returns:
 *    Pointer to an arena_context_t that references the supplied buffer. NULL if buffer is zero or
 *    it does not have enough capacity to store the arena context metadata.
 */
internal inline arena_context_t *arena_from_buf(uint8_t *buf, const size_t buf_capacity);

internal inline void *arena_alloc(void *ctx, const size_t size);


internal inline void *arena_realloc(void *ctx, void *ptr,
                                 const size_t old_size,
                                 const size_t new_size);

internal inline void arena_free(void *ctx, void *ptr, const size_t size);

internal inline void arena_destroy(void *ctx);

internal inline void arena_reset(arena_context_t *ctx);

internal inline void *arena_bump_aligned(arena_region_t *ctx,
                                             size_t size,
                                             size_t alignment);

internal inline void *arena_alloc_aligned(void *ctx, const size_t size, const size_t alignment);


global_var allocator_iface arena_interface = {
    .alloc    = arena_alloc,
    .realloc  = arena_realloc,
    .free     = arena_free,
    .free_all = arena_destroy
};

internal inline arena_context_t arena_init(size_t capacity, enum arena_flags flags,
        allocator_iface *optional_allocator, void *optional_alloc_ctx) {

    arena_context_t result; 

    /* Validate allocation flags flags */
    assert(!(flags & ARENA_BUFFER_BACKEND)
            && "Buffer backed arena is managed externally, use arena_from_buf instead");
    assert((flags & ARENA_MALLOC_BACKEND) | (flags & ARENA_VIRTUAL_BACKEND) | (flags & ARENA_MALLOC_BACKEND) 
            && "One allocation strategy must be selected for self-managed arenas");
    assert(!(flags & ARENA_MALLOC_BACKEND & ARENA_VIRTUAL_BACKEND & ARENA_CUSTOM_BACKEND)
            && "Only one allocation strategy can be chosen" );


    /* We need to allocate memory for the metadata about the region */
    uint64_t actual_capacity = sizeof(arena_region_t) + capacity;

    if (flags & ARENA_MALLOC_BACKEND) {

        result.begin = malloc(actual_capacity);
    }

    if (flags & ARENA_CUSTOM_BACKEND) {

        result.begin = optional_allocator->alloc(optional_alloc_ctx, actual_capacity);
    }

    if (flags & ARENA_VIRTUAL_BACKEND) {

        const uint64_t page_size = sysconf(_SC_PAGESIZE);

        actual_capacity = ALIGN_POW_2(actual_capacity, page_size);

        result.begin = mmap(NULL, actual_capacity,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANON | MAP_NORESERVE,
                -1, 0);

    }

    if (result.begin) result.begin->next = NULL;

    result.begin->offset    = 0;
    result.begin->capacity  = capacity;
    result.end              = result.begin;
    result.flags            = flags;

    result.optional_allocator = optional_allocator;
    result.optional_alloc_ctx = optional_alloc_ctx;

    return result;
}

internal inline arena_context_t *arena_from_buf(uint8_t *buf, const size_t buf_capacity) {

    const uint64_t metadata_size = sizeof (arena_context_t) + sizeof (arena_region_t);
    const uint64_t remaining_capacity = buf_capacity - metadata_size;

    if (buf_capacity < metadata_size) return NULL;
    

    arena_context_t *result = (void*)buf; 

    result->begin        = (arena_region_t*)&result[1];
    result->end          = result->begin;
    result->flags        = ARENA_BUFFER_BACKEND;

    result->begin->offset   = 0;
    result->begin->capacity = remaining_capacity;
    result->begin->next     = NULL;

    return result;
}

internal inline void *arena_alloc(void *ctx, const size_t size) {

    return arena_alloc_aligned(ctx, size, ARENA_DEFAULT_ALIGN);

}

internal inline void *arena_realloc(void *ctx, void *ptr, const size_t old_size, const size_t new_size){

    void *result = arena_alloc(ctx, new_size);

    if (result == NULL) return result;

    memcpy(result, ptr, old_size);

    return result;
}

internal inline void arena_free(void *ctx, void *ptr, const size_t size) {

    /*
    arena_context_t *arena = ctx;

    if (unlikely(arena && arena->end && &arena->end->data[arena->end->offset] - size == ptr)) {
        arena->end->offset -= size;
    }
    */

    /* Stop warnings about the arena_allocator as well */
    UNUSED(arena_interface);
    UNUSED(ctx);
    UNUSED(ptr);
    UNUSED(size);
}

internal inline void arena_destroy(void *ctx) {

    arena_context_t *arena = ctx;

    if (arena->flags & ARENA_BUFFER_BACKEND) {
        return arena_reset(arena);
    }

    arena_region_t *current = arena->begin;

    const uint64_t page_size = sysconf(_SC_PAGESIZE);

    while (current != NULL) {
        arena_region_t *next = current->next;

        uint64_t dealloc_size  = sizeof (*current) + current->capacity;
        if (arena->flags & ARENA_CUSTOM_BACKEND) {
            arena->optional_allocator->free(arena->optional_alloc_ctx, current, dealloc_size);
        } else if (arena->flags & ARENA_MALLOC_BACKEND) {
            free(current);
        } else if (arena->flags & ARENA_VIRTUAL_BACKEND) {
            dealloc_size = ALIGN_POW_2(dealloc_size, page_size);

            munmap(current, dealloc_size);
        }

        current = next;
    }
}

internal inline void arena_reset(arena_context_t *ctx) {

    arena_context_t *arena = ctx;

    arena_region_t *current = arena->begin;
    while (current != NULL) {
        current->offset = 0;
        current = current->next;
    }
}

internal inline void *arena_bump_aligned(arena_region_t *ctx,
                                             size_t size,
                                             size_t alignment) {


    /* Ensure alignment is a power of two; otherwise the rounding logic is undefined. */
    if ((alignment & (alignment - 1)) != 0) {
        return NULL; 
    }

    uintptr_t raw_ptr     = (uintptr_t)ctx->data + (uintptr_t)ctx->offset;

    uintptr_t aligned_ptr = ALIGN_POW_2(raw_ptr, alignment);
    size_t padding = (size_t)(aligned_ptr - raw_ptr);

    size_t total_needed = padding + size;

    if (ctx->capacity - ctx->offset < total_needed) {
        return NULL;
    }

    ctx->offset += total_needed;

    return (void *)aligned_ptr;

}

internal inline void *arena_alloc_aligned(void *ctx, const size_t size, const size_t alignment) {

    void *result = NULL;
    arena_context_t *arena = ctx;

    if ((alignment & (alignment - 1)) != 0) {
        return NULL; 
    }

    /* Try to allocate within the first region */
    result = arena_bump_aligned(arena->begin, size, alignment);
    if (result != NULL
            || (arena->flags & ARENA_BUFFER_BACKEND)
            || !(arena->flags & ARENA_GROWABLE)) {
        /* There is nothing extra to do in a arena that can't grow */
        return result;
    }

    arena_region_t *try_region;
    if (arena->flags & ARENA_FAST_ALLOC) {
        try_region = arena->end;
    } else {
        try_region = arena->begin->next;
    }

    result = arena_bump_aligned(try_region, size ,alignment);

    while (result == NULL && try_region->next != NULL) {
        result = arena_bump_aligned(try_region->next, size ,alignment);
        try_region = try_region->next;
    }

    if (result == NULL) {
        /* We need to allocate memory for the metadata about the region */
        uint64_t new_capacity = max(try_region->capacity * 2, size + alignment + sizeof (arena_region_t));

        if (arena->flags & ARENA_MALLOC_BACKEND) {

            try_region->next = malloc(new_capacity);
        }

        if (arena->flags & ARENA_CUSTOM_BACKEND) {

            try_region->next = arena->optional_allocator->alloc(arena->optional_alloc_ctx, new_capacity);
        }

        if (arena->flags & ARENA_VIRTUAL_BACKEND) {

            const uint64_t page_size = sysconf(_SC_PAGESIZE);

            new_capacity = ALIGN_POW_2(new_capacity, page_size);

            try_region->next = mmap(NULL, new_capacity,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANON | MAP_NORESERVE,
                    -1, 0);
        }

        try_region = try_region->next;
        if (try_region) {

            try_region->offset   = 0;
            try_region->capacity = new_capacity;
            try_region->next     = 0;

            result = arena_bump_aligned(try_region, size, alignment);
            arena->end = try_region;
        }
    }

    return result;
}

#endif /* #ifdef ALLOC_ARENA_IMPL */

#endif /* #ifndef ALLOCATOR_H */

