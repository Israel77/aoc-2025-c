#include "allocator.h"
#include "compiler.h"
#include "error.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*
 * This is an open addressing hashmap implementation
 * which tends to perform better if reads are more frequent
 * then writes and deletes. This hashmap is not thread-safe
 */

enum hm_node_state {
    HM_NODE_FREE,
    HM_NODE_USED,
    HM_NODE_DEAD,
};

static const size_t HM_DEFAULT_CAPACITY = 128;

typedef uint64_t (*hash_func_t)(const void *key);
typedef bool (*eq_func_t)(const void *a, const void *b);

typedef struct hm_node {
    void *key;
    void *value;
    enum hm_node_state state;
} hm_node_t;

typedef struct {

    hash_func_t hash_func;
    eq_func_t   eq_func;

    const allocator_t *allocator;
    void        *alloc_ctx;

    size_t count;
    size_t tombstones;

    /* how many items can be present at a given time */
    size_t usable_capacity;
    /* the actual allocated capacity */
    size_t capacity;

    hm_node_t *data;
} hashmap_t;


/*
 * Tries to insert a value into the hashmap and returns the previous
 * value if there was one, NULL otherwise. The key and value must 
 * have a lifetime greater than or equal to the lifetime of the 
 * hashmap to avoid undefined behavior.
 *
 * @allocator        - Allocator to be used for the internal storage
 * @alloc_ctx        - Allocator context
 * @hash_func        - Function to generate the hash for the key type
 * @eq_func          - Function to compare if two keys are equals
 * @desired_capacity - How many items can be stored (if 0 will use a default value)
 * @err              - Container for errors.
 *
 * Returns:
 *     Previous values stored in the same key.
 */
static hashmap_t hm_init(const allocator_t *allocator, void *alloc_ctx,
        const hash_func_t hash_func, const eq_func_t eq_func,
        size_t desired_capacity, error_t *err);
/*
 * Tries to insert a value into the hashmap and returns the previous
 * value if there was one, NULL otherwise. The key and value must 
 * have a lifetime greater than or equal to the lifetime of the 
 * hashmap to avoid undefined behavior.
 *
 * @hm    - The hashmap
 * @key   - Key to the hashmap
 * @value - The value to be stored
 * @err   - Container for errors
 *
 * Returns:
 *     Previous values stored in the same key.
 */
static void *hm_insert(hashmap_t *hm, void *key, void *value, error_t *err);
/*
 * Given a key, returns the corresponding value stored in the hashmap if there
 * is one, otherwise NULL.
 *
 * @hm    - The hashmap
 * @key   - Key to the hashmap
 *
 * Returns:
 *     The value associated with the given key.
 */
static void *hm_get(hashmap_t *hm, const void *key);
/*
 * Given a key, deletes the corresponding value stored in the hashmap if there
 * is one and returns it, otherwise returns NULL.
 *
 * @hm    - The hashmap
 * @key   - Key to the hashmap
 *
 * Returns:
 *     The value previously associated with the given key.
 */
static void *hm_delete(hashmap_t *hm, const void *key);
/*
 * Destroys a hashmap, releasing all internal resources. The function
 * traverses the hashmap's internal storage and frees any auxiliary
 * structure that was allocated using the internal allocator.
 *
 * @hm - Pointer to the hashmap to be destroyed. After the call the
 *       pointer must not be used again.
 *
 * Returns:
 *      Nothing.
 */
static void hm_destroy(hashmap_t *hm);

#ifdef HM_IMPL
static inline uint64_t hm_hash(const hashmap_t *hm, const void *key) {
    return hm->hash_func(key) % hm->usable_capacity;
}

/* This function might allocate temporary storage using the internal allocator */
/* Time  complexity: O(n) */
/* Space complexity: O(n) */
static inline bool hm_rehash(hashmap_t *hm) {

    hm_node_t *needs_rehash;
    size_t rehash_count = 0;
    bool temp_allocated;

    /* Avoid allocation if there is enough space left within the backing array */
    if (hm->capacity > hm->count + hm->usable_capacity) {
        temp_allocated = false;
        needs_rehash = &hm->data[hm->usable_capacity];
    } else {
        temp_allocated = true;
        needs_rehash = hm->allocator->alloc(hm->alloc_ctx, hm->count * sizeof (hm_node_t));
    }

    if (!needs_rehash) {
        return false;
    }

    for (size_t i = 0; i < hm->usable_capacity; ++i) {
        if (hm->data[i].state == HM_NODE_USED) {
            needs_rehash[rehash_count++] = hm->data[i];
        }
        hm->data[i].state = HM_NODE_FREE;
    }

    for (size_t i = 0; i < rehash_count; ++i) {
        uintptr_t hash = hm_hash(hm, needs_rehash[i].key);
        while (hm->data[hash].state == HM_NODE_USED) {
            hash = (hash + 1) % hm->usable_capacity;
        }
        hm->data[hash] = needs_rehash[i];
    }

    if (temp_allocated) {
        hm->allocator->free(hm->alloc_ctx, needs_rehash, hm->count * sizeof (hm_node_t));
    }

    /* there should be no tombstones after rehashing */
    hm->tombstones = 0;

    return true;
}

/* get rid of tombstones without allocating */
/* Time  complexity: O(n) */
/* Space complexity: O(1) */
static inline void hm_compress(hashmap_t *hm) {
    size_t last_used = 0;
    size_t current   = 1;

    while (current < hm->usable_capacity) {
        hm_node_t *current_item = &hm->data[current];

        /* assuming this function will only be called when there is
         * a lot of tombstones */
        if (likely(current_item->state == HM_NODE_DEAD)) {
            /* get rid of tombstones */
            current_item->state = HM_NODE_FREE;
            hm->tombstones     -= 1;
        } else if (unlikely(current_item->state == HM_NODE_USED)) {

            const uintptr_t hash = hm_hash(hm, current_item->key);
            /* location to continue the iteration */
            size_t copy_from       = current;

            if (hash < last_used) {
                current = hash;
                while (hm->data[current].state == HM_NODE_USED) {
                    current = (current + 1) % hm->usable_capacity;
                }
            } else if (hash == last_used) {
                current   = last_used + 1;
                last_used = current;
            } else {
                current   = hash;
                last_used = current;
            }

            hm->data[current].state = HM_NODE_USED;
            hm->data[current].key   = hm->data[copy_from].key;
            hm->data[current].value = hm->data[copy_from].value;

            current = copy_from;
        }

        ++current;
    }
}

static inline bool hm_grow(hashmap_t *hm) {

    size_t old_capacity = hm->usable_capacity;
    size_t new_capacity = hm->usable_capacity * 2;

    /* the memory is already allocated */
    if (new_capacity <= hm->capacity) {
        goto exit;
    }

    /* needs to allocate more memory */
    hm_node_t *data_ptr = hm->allocator->realloc(hm->alloc_ctx, hm->data, old_capacity * sizeof (hm_node_t), new_capacity * sizeof (hm_node_t));
    if (!data_ptr) {
        return false;
    }
    hm->data = data_ptr;
    hm->capacity = new_capacity;

exit:
    hm->usable_capacity = new_capacity;
    return hm_rehash(hm);
}

static inline bool hm_shrink(hashmap_t *hm) {

    /* don't reduce the actual capacity, only the usable space */
    hm->usable_capacity /= 2;

    return hm_rehash(hm);
}


static hashmap_t hm_init(const allocator_t *allocator, void *alloc_ctx,
        const hash_func_t hash_func, const eq_func_t eq_func,
        size_t desired_capacity, error_t *err) {

    if (desired_capacity == 0) {
        desired_capacity = HM_DEFAULT_CAPACITY;
    }

    hm_node_t *data = allocator->alloc(alloc_ctx, desired_capacity * sizeof (hm_node_t));

    if (!data) {
        err->is_error = true;
        sprintf(err->error_msg, "Error on memory allocation");
        hashmap_t result = {0};
        return result;
    }

    hashmap_t result = {
        .allocator         = allocator,
        .alloc_ctx         = alloc_ctx,
        .hash_func         = hash_func,
        .eq_func           = eq_func,
        .capacity          = desired_capacity,
        .usable_capacity   = desired_capacity,
        .data              = data,
    };

    for (size_t i = 0; i < result.usable_capacity; ++i) {
        result.data[i].state = HM_NODE_FREE;
    }

    return result;
}

static void hm_destroy(hashmap_t *hm) {
    hm->allocator->free(hm->alloc_ctx, hm->data, hm->capacity * sizeof (hm_node_t));
}

static void *hm_insert(hashmap_t *hm, void *key, void *value, error_t *err) {

    err->is_error = false;
    void *result = NULL;

    const float load_factor = (float) hm->count / (float) hm->usable_capacity; 


    if (unlikely(load_factor > 0.9)) {
        /* Fail immediately if cannot grow at 90% capacity*/
        if (!hm_grow(hm)) {
            err->is_error = true;
            sprintf(err->error_msg, "Hashmap was full and failed to increase capacity");
            return result;
        };
    }
    else if (load_factor > 0.7) {
        /* At 70% capacity, try to grow for performance reasons, but don't fail if cannot grow */
        hm_grow(hm);
    }

    uintptr_t hash = hm_hash(hm, key);

    hm_node_t *curr = &hm->data[hash];

    while ((curr->state != HM_NODE_FREE && curr->state != HM_NODE_DEAD)) {
        if (hm->eq_func(curr->key, key)) {
            result = curr->value;

            curr->state = HM_NODE_USED;
            curr->key   = key;
            curr->value = value;

            return result;
        }

        hash = (hash + 1) % hm->usable_capacity;
        curr = &hm->data[hash];
    }

    /* Replace a tombstone with a valid item */
    if (curr->state == HM_NODE_DEAD) {
        hm->tombstones--;
    }

    hm->count++;
    curr->state = HM_NODE_USED;
    curr->key   = key;
    curr->value = value;

    return result;
}
void *hm_get(hashmap_t *hm, const void *key) {

    uintptr_t hash = hm_hash(hm, key);

    hm_node_t *curr = &hm->data[hash];

    while (!(curr->state == HM_NODE_FREE)) {
        if (hm->eq_func(curr->key, key)) {
            return curr->value;
        }

        hash = (hash + 1) % hm->usable_capacity;
        curr = &hm->data[hash];
    }

    return NULL;
}

void *hm_delete(hashmap_t *hm, const void *key) {

    void *result = NULL;

    /* rehash if there are too many tombstones */
    if (hm->tombstones > 3 * (hm->usable_capacity / 4)) {
        hm_compress(hm);
    }

    /* don't shrink by load factor if the array is already small */
    if (hm->usable_capacity > 128 && unlikely(hm->count < hm->usable_capacity)) {
        hm_shrink(hm);
    }

    uintptr_t hash = hm_hash(hm, key);

    hm_node_t *curr = &hm->data[hash];

    while (!(curr->state == HM_NODE_FREE)) {

        if (hm->eq_func(curr->key, key)) {
            result = curr->value;
            curr->state = HM_NODE_DEAD;
            curr->value = NULL;

            hm->count--;
            hm->tombstones++;
            return result;
        }

        hash = (hash + 1) % hm->usable_capacity;
        curr = &hm->data[hash];
    }

    return result;
}

#endif /* ifdef HM_IMPL */
