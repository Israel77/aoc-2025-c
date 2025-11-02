#ifndef ALLOCATOR_H
#define ALLOCATOR_H

/* These headers are fairly easy to replace if they're not available, they're mostly for typedefs */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* Assert is optional, but recommended to guarantee some invariants. */
#include <assert.h>

/* Importing stdlib is only needed at the top level to define what is size_t */
/* and when using the std allocator. If you provide a custom definition of  */
/* size_t and don't use the std allocator, there is no need to include it. */
/* Example:  */
/* #define CUSTOM_SIZE_T */
/* typedef uint64_t size_t; */
#ifndef CUSTOM_SIZE_T
#include <stdlib.h>
#endif

#ifndef NULL
#define NULL 0
#endif /* #ifndef NULL */

#include "todo.h"

/* The basic data structure that represents a generic allocator */
typedef struct {
    void *(*alloc)(void *ctx, const size_t size);
    void *(*realloc)(void *ctx, void *ptr, const size_t old_size, const size_t new_size);
    void (*free)(void *ctx, void *ptr, const size_t size);
} allocator_t;

#ifdef ALLOC_STD_IMPL

#include <stdlib.h>

static void *std_alloc(void *ctx, const size_t size) {
    UNUSED(ctx);
    return malloc(size);
}

static void *std_realloc(void *ctx, void *ptr, const size_t old_size, const size_t new_size) {
    UNUSED(ctx);
    UNUSED(old_size);
    return realloc(ptr, new_size);
}

static void std_free(void *ctx, void *ptr, const size_t size) {
    UNUSED(ctx);
    UNUSED(size);
    return free(ptr);
}

static const allocator_t global_std_allocator = {
    .alloc = &std_alloc,
    .realloc = &std_realloc,
    .free = &std_free
};

#endif /* #ifdef ALLOC_STD_IMPL */

#ifdef ALLOC_ARENA_IMPL

#ifndef ARENA_CHUNK_DEFAULT_CAP
#define ARENA_CHUNK_DEFAULT_CAP 4096
#endif /* #ifndef ARENA_CHUNK_DEFAULT_CAP */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/*
 * Single region arena allocator. This allocator does not owns the memory of the underlying
 * buffer, the clean-up must be done by the caller.
 */
typedef struct {
    size_t offset;
    size_t capacity;

    uint8_t data[];
} arena_context_t;

/*
 * Initializes an arena allocator that operates on a single contiguous
 * memory region supplied by the caller. The arena does not perform any
 * internal allocations; it simply tracks an offset into the provided
 * buffer.
 *
 * @buf       - Pointer to the start of the memory region that will back the arena.
 * @capacity  - Size of the region in bytes. Must be a multiple of sizeof(uintptr_t).
 *
 * Returns:
 *     Pointer to a newly‑initialized arena_context_t that references @buf.
 *     The returned pointer will be the same as @buf, with the arena metadata
 *     inserted at the start of the buffer. The arena does not own the memory;
 *     the caller must ensure the buffer remains valid for the arena's lifetime.
 */
static inline arena_context_t *arena_init(uintptr_t *buf, size_t capacity);

/*
 * Allocates a block of @size bytes from the arena. The allocation is
 * performed by bumping the arena's offset; no per‑allocation metadata is
 * stored, so the returned pointer is only valid until the arena is reset
 * or freed.
 *
 * @ctx   - Pointer to an arena_context_t returned by arena_init().
 * @size  - Number of bytes to allocate. Must be > 0.
 *
 * Returns:
 *     Pointer to @size bytes within the arena, or NULL if there is not
 *     enough remaining capacity.
 */
static inline void *arena_alloc(void *ctx, const size_t size);

/*
 * Reallocates a previously allocated block to a new size. Because the
 * arena uses a simple bump allocator, the operation can only succeed
 * without moving the block when the original block is the most recent
 * allocation and there is sufficient remaining capacity.
 *
 * If the block was not the most recently allocated, but there is enough
 * remaining capacity to store the desired size, it will be copied as if
 * an arena_alloc followed by memcpy was performed.
 *
 * @ctx       - Pointer to an arena_context_t.
 * @ptr       - Pointer returned by a prior arena_alloc() or arena_realloc().
 * @old_size  - Original size of the allocation.
 * @new_size  - Desired new size.
 *
 * Returns:
 *     Pointer to a block of @new_size bytes (which may be the same as @ptr)
 *     or NULL if there is no remaining capacity. The original block remains
 *     unchanged.
 */
static inline void *arena_realloc(void *ctx, void *ptr,
                                 const size_t old_size,
                                 const size_t new_size);

/*
 * Frees a block allocated from the arena. Since the arena is a bump
 * allocator, individual frees are no‑ops; the function exists for API
 * symmetry.
 *
 * @ctx   - Pointer to an arena_context_t.
 * @ptr   - Pointer returned by arena_alloc() or arena_realloc().
 * @size  - Size of the allocation being freed.
 */
static inline void arena_free(void *ctx, void *ptr, const size_t size);

/*
 * Bumps the arena's offset by @size bytes, returning a pointer that is
 * aligned to @alignment. The alignment must be a power of two.
 *
 * @ctx        - Pointer to an arena_context_t.
 * @size       - Number of bytes to allocate.
 * @alignment  - Desired alignment (e.g., 8, 16, 32).
 *
 * Returns:
 *     Aligned pointer within the arena, or NULL if the allocation would
 *     exceed the arena's capacity.
 */
static inline void *arena_region_bump_aligned(arena_context_t *ctx,
                                             size_t size,
                                             size_t alignment);

/*
 * Simple bump allocation without any alignment guarantees beyond the
 * natural alignment of the underlying type.
 *
 * @ctx   - Pointer to an arena_context_t.
 * @size  - Number of bytes to allocate.
 *
 * Returns:
 *     Pointer to @size bytes within the arena, or NULL if insufficient
 *     space remains.
 */
static inline void *arena_region_bump(arena_context_t *ctx, size_t size);

/*
 * Resets a single‑region arena to an empty state. The arena's offset is
 * set back to zero, allowing the same memory buffer to be reused for
 * new allocations. No deallocation is performed; the backing buffer
 * remains owned by the caller.
 *
 * @ctx - Pointer to the arena_context_t to be reset.
 *
 * Returns:
 *     Nothing.
 */
static inline void arena_reset(arena_context_t *ctx);


static inline arena_context_t *arena_init(uintptr_t *buf, size_t capacity) {

    /* Remaining capacity in the buffer after storing metadata about the arena */
    size_t remaining_cap = capacity - 2 * sizeof (size_t);

    ((size_t *)buf)[0] = 0; /* offset */
    ((size_t *)buf)[1] = remaining_cap; /* capacity */

    return (arena_context_t *)buf;
}

static inline void *arena_region_bump_aligned(arena_context_t *region, size_t size, size_t alignment) {
    void *result = NULL;

    assert(((alignment & (alignment - 1)) == 0)
            && "Alignment must be a power of 2");

    /* Align the pointer forward */
    uintptr_t offset = (uintptr_t)region->data + region->offset;
    uintptr_t modulo = offset & (uintptr_t)(alignment - 1);

    if (modulo != 0) {
        offset += alignment - modulo;
    }

    if (region->capacity - offset < size) {
        return NULL;
    }

    result = region->data + offset;
    region->offset = offset + size;
    
    return result;
}

static inline void *arena_region_bump(arena_context_t *region, size_t size) {
    void *result;

    if (region->capacity - region->offset < size) {
        return NULL;
    }

    result = region->data + region->offset;
    region->offset += size;

    return result;
}

static inline void arena_reset(arena_context_t *ctx) {
    ctx->offset = 0;
}

static inline void *arena_alloc(void *ctx, const size_t size) {
    return arena_region_bump(ctx, size);
}
static inline void *arena_alloc_aligned(void *ctx, const size_t size) {
    return arena_region_bump_aligned(ctx, size, 16);
}

static inline void *arena_realloc(void *ctx, void *ptr, const size_t old_size, const size_t new_size) {
    UNUSED(old_size);
    UNUSED(ptr);
    void *result = arena_region_bump(ctx, new_size);
    memcpy(result, ptr, old_size);

    return result;
}

static inline void arena_free(void *ctx, void *ptr, const size_t size) {
    UNUSED(ctx);
    UNUSED(ptr);
    UNUSED(size);
}


/* 
 * Multi-region arena allocator. Unlike the single-region version, this allocator
 * DOES own the memory of the underlying data. The clean-up must be done by calling
 * multiarena_free_all
 */
typedef struct multiarena_region_t multiarena_region_t;
struct multiarena_region_t {
    multiarena_region_t *next;
    size_t offset;
    size_t capacity;

    uint8_t data[];
};

typedef struct {
    multiarena_region_t *begin;
    multiarena_region_t *end;

    /* Arenas are built on top of available allocators */
    const allocator_t *inner_alloc;
    void        *inner_ctx;
} multiarena_context_t;

/*
 * Allocates a block of @size bytes from a multi‑region arena. The arena
 * consists of a linked list of regions; when the current region does not
 * have enough free space a new region is obtained from the inner allocator.
 *
 * @ctx   - Pointer to a multiarena_context_t.
 * @size  - Number of bytes to allocate (must be > 0).
 *
 * Returns:
 *     Pointer to @size bytes within the arena, or NULL if the inner
 *     allocator fails to provide a new region.
 */
static inline void *multiarena_alloc(void *ctx, const size_t size);

/*
 * Reallocates a previously allocated block to @new_size bytes. The
 * operation can succeed in‑place only if the block resides at the end of
 * its region and the region has enough remaining capacity. Otherwise a
 * new block is allocated (potentially in a different region), the old
 * contents are copied, and the old block is left untouched.
 *
 * @ctx       - Pointer to a multiarena_context_t.
 * @ptr       - Pointer returned by a prior multiarena_alloc() or
 *              multiarena_realloc().
 * @old_size  - Original size of the allocation.
 * @new_size  - Desired new size.
 *
 * Returns:
 *     Pointer to a block of @new_size bytes (which may be the same as @ptr)
 *     or NULL if a new block cannot be allocated. On failure the original
 *     block remains valid.
 */
static inline void *multiarena_realloc(void *ctx, void *ptr,
                                      const size_t old_size,
                                      const size_t new_size);
/*
 * Frees a block allocated from the multi‑region arena. Individual frees
 * are no‑ops because each region is a bump allocator; the function exists
 * for API symmetry.
 *
 * @ctx   - Pointer to a multiarena_context_t.
 * @ptr   - Pointer returned by multiarena_alloc() or multiarena_realloc().
 * @size  - Size of the allocation being freed.
 *
 * Returns:
 *     Nothing.
 */
static inline void multiarena_free(void *ctx, void *ptr, const size_t size);

/*
 * Converts a single region of a multiarena into a stand‑alone single‑region
 * arena_context_t. The returned arena shares the same underlying memory;
 * it does **not** copy data. The caller must ensure the region outlives the
 * returned arena. This main purpose of this function is to enable code reuse
 * between single-region arena and multiarena allocation algorithms.
 *
 * @multiarena_region - Pointer to the region to be wrapped.
 *
 * Returns:
 *     Pointer to an arena_context_t that operates on the given region.
 */
static inline arena_context_t *region_as_single(multiarena_region_t *multiarena_region);

/*
 * Creates a new region for a multiarena. The region is allocated using the
 * supplied @allocator and @inner_ctx. The region's capacity is at least
 * @ensure_capacity bytes; if the allocator returns a smaller block the
 * function fails and returns NULL.
 *
 * @ensure_capacity - Minimum capacity (in bytes) the region must provide.
 * @allocator       - Allocator used to obtain the memory for the region.
 * @inner_ctx       - Context passed to the allocator.
 *
 * Returns:
 *     Pointer to an initialized multiarena_region_t, or NULL on allocation
 *     failure.
 */
static inline multiarena_region_t *multiarena_region_init(size_t ensure_capacity,
                                                          const allocator_t *allocator,
                                                          void *inner_ctx);
/*
 * Releases all memory owned by a multiarena. Every region in the linked
 * list is returned to the inner allocator, and the context structure is
 * cleared. After this call the multiarena_context_t must not be used
 * unless re‑initialized.
 *
 * @arena - Pointer to the multiarena_context_t to be destroyed.
 *
 * Returns:
 *     Nothing.
 */
static inline void multiarena_free_all(multiarena_context_t *arena);

/*
 * Resets a multi‑region arena to an empty state without releasing the
 * underlying memory back to the inner allocator. All regions remain
 * allocated, but their offsets are set to zero so that subsequent
 * allocations will reuse the existing chunks. This operation is O(1)
 * per region and does **not** invoke inner_alloc->free().
 *
 * @multiarena - Pointer to the multiarena_context_t to be reset.
 *
 * Returns:
 *     Nothing.
 */
static inline void multiarena_reset(multiarena_context_t *multiarena);

static inline arena_context_t *multi_as_region(multiarena_region_t *multiarena_region) {
    arena_context_t *result = NULL;

    if (multiarena_region == NULL) {
        return result;
    }

    /* skip the pointer to next */
    result = (arena_context_t *)((uintptr_t)multiarena_region + sizeof (multiarena_region_t*));

    return result;
}

static inline multiarena_region_t *multiarena_region_init(size_t ensure_capacity, const allocator_t *allocator, void *inner_ctx) {
    multiarena_region_t *result = {0};

    /* Use the maximum between ARENA_CHUNK_DEFAULT_CAP and the argument passed */
    size_t init_cap = ARENA_CHUNK_DEFAULT_CAP < ensure_capacity
        ? ensure_capacity
        : ARENA_CHUNK_DEFAULT_CAP;

    size_t alloc_size = sizeof(multiarena_region_t) /* Metadata about the chunk itself */
        + sizeof(uintptr_t) * init_cap;       /* How much data needs to be stored */

    result = (multiarena_region_t*) allocator->alloc(inner_ctx, alloc_size);

    if (result == NULL) {
        return NULL;
    }
    
    result->capacity = init_cap;
    result->offset = 0;

    return result;
}

static void *multiarena_alloc(void *ctx, const size_t size) {
    multiarena_context_t *arena = (multiarena_context_t *)ctx;
    
    /* Use the std allocator if it is available and no other allocator was provided */
    if (arena->inner_alloc == NULL) {
#ifdef ALLOC_STD_IMPL
        arena->inner_alloc = &global_std_allocator;
#else /* ifdef ALLOC_STD_IMPL */
        return NULL;
#endif
    }

    if (arena->begin == NULL) {
        arena->begin = multiarena_region_init(size, arena->inner_alloc, arena->inner_ctx);
        arena->end = arena->begin;
        arena->end->next = NULL;
    }

    /*
     * NOTE: The following allocation strategy prioritizes speed over memory usage.
     * To make the implementation more memory efficient, allocate within the first
     * available chunk, rather than always choosing the last chunk.
     */

    /* Try to allocate within an existing chunk */
    void *result = arena_region_bump(multi_as_region(arena->end), size);
    if (result != NULL) {
        /* Succesful allocation */
        return result;
    }

    /* Create a new chunk otherwise */
    /* NOTE: Use a while loop instead of if, to avoid memory leak after using arena_reset */
    while (arena->end->next != NULL) {
        arena->end = arena->end->next;
    }

    arena->end->next = multiarena_region_init(size, arena->inner_alloc, arena->inner_ctx);
    arena->end = arena->end->next;

    result = arena_region_bump(multi_as_region(arena->end), size);
    if (result != NULL) {
        arena->end->next = NULL;
    }

    return result;
}

static inline void *multiarena_realloc(void *ctx, void *ptr, const size_t old_size, const size_t new_size) {
    multiarena_context_t *arena = (multiarena_context_t *)ctx;

    if (new_size == 0) {
        return NULL;
    }

    if (ptr == NULL) {
        return multiarena_alloc(ctx, new_size);
    }

    /* Check if the old pointer an existing chunk */
    for (multiarena_region_t *chunk = arena->begin; chunk != NULL; chunk = chunk->next) {

        if ((uintptr_t)ptr >= (uintptr_t)chunk->data && 
            (uintptr_t)ptr < (uintptr_t)(chunk->data + chunk->capacity)) {
            /* Pointer is within this chunk */
            size_t remaining_capacity = chunk->capacity - chunk->offset;

            /* Check if the old data is at the end of the chunk */
            if ((uintptr_t)ptr + old_size == (uintptr_t)(chunk->data + chunk->offset)) {
                /* If new_size fits in the remaining capacity, just adjust the count */
                if (remaining_capacity >= new_size) {
                    chunk->offset += (new_size - old_size); /* Adjust count */
                    return ptr;
                }
            }

            /* Allocate new memory if necessary */
            void *new_ptr = multiarena_alloc(ctx, new_size);
            if (new_ptr != NULL) {
                memcpy(new_ptr, ptr, old_size);
                return new_ptr;
            }
            return NULL; /* Allocation failed */
        }
    }

    /* Pointer not found in any chunk */
    return multiarena_alloc(ctx, new_size);
}

static inline void multiarena_free(void *ctx, void *ptr, const size_t size) {
    arena_free(ctx, ptr,  size);
}

/* Frees all the chunks within the arena. Prefer to use arena_reset */
static inline void multiarena_free_all(multiarena_context_t *arena) {

    multiarena_region_t *chunk = arena->begin; 
    while (chunk != NULL) {
        multiarena_region_t *next_chunk = chunk->next;
        arena->inner_alloc->free(arena->inner_ctx, chunk, sizeof(multiarena_region_t) + sizeof(uintptr_t) * chunk->capacity);
        chunk = next_chunk;
    }

    arena->begin = NULL;
    arena->end = NULL;
}

/* Reuse the chunks by overwriting their data. */
/* This function does not call inner_alloc->free */
static inline void multiarena_reset(multiarena_context_t *multiarena) {

    for (multiarena_region_t *region = multiarena->begin; region != NULL; region = region->next) {
        region->offset = 0;
    }

    multiarena->end = multiarena->begin;
}


static allocator_t multiarena_allocator = {
    .alloc = &multiarena_alloc,
    .realloc = &multiarena_realloc,
    .free = &multiarena_free
};

#endif /* #ifdef ALLOC_ARENA_IMPL */

#ifdef ALLOC_FIXED_POOL_IMPL

typedef struct {
    size_t object_size;
    size_t pool_size;

    /* Base allocator (only used when initializing and destroying) */
    const allocator_t *inner_alloc;
    void              *inner_ctx;

    /* Arrays */
    bool    *is_free; /* track free objects */
    uint8_t *pool;   /* store the data */
} fixed_pool_context_t;

/* Allocates an object and returns its index within the internal pool array. */
static size_t fixed_pool_alloc_by_index(fixed_pool_context_t *pool_ctx) {
    /* Find the next free object */
    for (size_t i = 0; i < pool_ctx->pool_size; ++i) {
        if (pool_ctx->is_free[i]) {
            pool_ctx->is_free[i] = false;
            return i;
        }
    }

    return (size_t)-1;
}

/* Free an object by passing its index. Unlike other allocators, free is an idempotent function for pools. */
static void fixed_pool_free_by_index(fixed_pool_context_t *pool_ctx, size_t index) {

    assert(index < pool_ctx->pool_size && "Invalid index");

    pool_ctx->is_free[index] = true;
}

/*#region: Allocator interface */
static void *fixed_pool_alloc(void *ctx, const size_t size) {
    fixed_pool_context_t *pool_ctx = (fixed_pool_context_t *)ctx;

    if (size != pool_ctx->object_size) {
        return NULL; /* Size must match the fixed object size */
    }

    /* Use the index-based allocation */
    size_t index = fixed_pool_alloc_by_index(pool_ctx);
    if (index == (size_t)-1) {
        return NULL; /* Allocation failed */
    }

    return pool_ctx->pool + (index * pool_ctx->object_size); /* Return pointer to the object */
}

static void fixed_pool_free(void *ctx, void *ptr, const size_t size) {
    /* Size is defined by the pool context */
    UNUSED(size);

    fixed_pool_context_t *pool_ctx = (fixed_pool_context_t *)ctx;

    /* Calculate the index of the object being freed */
    size_t index = ((uint8_t *)ptr - pool_ctx->pool) / pool_ctx->object_size;

    /* Use assert to check if the index is valid before freeing */
    assert(index < pool_ctx->pool_size && "Invalid index for free operation");
    fixed_pool_free_by_index(pool_ctx, index);
}

static void *fixed_pool_realloc(void *ctx, void *ptr, const size_t old_size, const size_t new_size) {

    /* If the pointer is null, behave like alloc */
    if (ptr == NULL) {
        return fixed_pool_alloc(ctx, new_size);
    }

    /* Reallocation is not supported in a fixed pool allocator */
#ifdef DEBUG_MODE
    assert(NULL && "You tried to reallocate using a fixed pool allocator");
#endif
    UNUSED(old_size);
    return NULL;
}
/*#endregion: Allocator interface */

static fixed_pool_context_t fixed_pool_init(size_t object_size, size_t pool_size, const allocator_t *allocator, void *ctx) {

    fixed_pool_context_t pool_ctx = {
        .object_size = object_size,
        .pool_size = pool_size,
        .pool = (uint8_t *) allocator->alloc(ctx, object_size * pool_size),
        .is_free = (bool *) allocator->alloc(ctx, sizeof(bool) * pool_size),
        .inner_alloc = allocator,
        .inner_ctx = ctx,
    };

    /* Initialize the free list */
    for (size_t i = 0; i < pool_size; ++i) {
        pool_ctx.is_free[i] = true;
    }

    return pool_ctx;
}

static void fixed_pool_free_all(fixed_pool_context_t *pool_ctx) {
    /* Free the data */
    pool_ctx->inner_alloc->free(pool_ctx->inner_ctx, pool_ctx->pool, pool_ctx->object_size * pool_ctx->pool_size );
    /* Free the array storing free indices */
    pool_ctx->inner_alloc->free(pool_ctx->inner_ctx, pool_ctx->is_free, sizeof(bool) * pool_ctx->pool_size);
}

static allocator_t fixed_pool_allocator = {
    .alloc = &fixed_pool_alloc,
    .realloc = &fixed_pool_realloc,
    .free = &fixed_pool_free
};

#endif /* #ifdef ALLOC_FIXED_POOL_IMPL */

#endif /* #ifndef ALLOCATOR_H */

