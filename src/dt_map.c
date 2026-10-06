/*
 * dt_map.c: Associative arrays for Unit 5, Section E.
 *
 * An array does not store its indices. This map stores its keys.
 * An array calculates a position with one subtraction.
 * The map calculates a hash and then compares keys in one bucket.
 *
 * Hashing turns the key into a bucket number. Compare all keys in that bucket
 * because two keys can select it. Use a linked list for each bucket. Start the
 * unsigned accumulator at 14695981039346656037ULL. For each unsigned byte,
 * exclusive-or the byte into it and multiply by 1099511628211ULL.
 *
 * A separate list stores insertion order for stable output. dt_map_key_at
 * reads this list. Updating a key preserves its position. Removing and
 * reinserting a key moves it to the end.
 */

#include "dt.h"

#include <stdlib.h>
#include <string.h>

#define DT_MAP_BUCKET_COUNT 16

typedef struct dt_map_entry {
    char *key;
    dt_value value;
    struct dt_map_entry *next;
} dt_map_entry;

struct dt_map {
    dt_map_entry *buckets[DT_MAP_BUCKET_COUNT];
    dt_map_entry **order;
    size_t length;
    size_t order_capacity;
};

static size_t hash_key(const char *key)
{
    unsigned long long hash = 14695981039346656037ULL;

    for (const unsigned char *p = (const unsigned char *)key;
         *p != '\0';
         p++) {
        hash ^= (unsigned long long)*p;
        hash *= 1099511628211ULL;
    }

    return (size_t)(hash % DT_MAP_BUCKET_COUNT);
}

dt_map *dt_map_new(void)
{
    /* TODO: Return an allocated empty map. Return NULL after an allocation failure.
       dt_map_new()  -> a map whose dt_map_len is 0
       cases/normal/map_basics.case */
       
    return calloc(1, sizeof(dt_map));
}

/*
 * dt_map_free releases each entry, copied key, order array, and map.
 * It accepts NULL. The environment owns the values.
 */
void dt_map_free(dt_map *m)
{
    /* TODO: Release each entry, copied key, order array, and map.
       Preserve the values. The environment owns them.
       a map holding a string value  -> the nodes and keys go, the string stays
       dt_map_free(NULL)             -> returns, having done nothing
       cases/cleanup/map_churn.case */
    
    if (m == NULL) {
        return;
    }

    for (size_t i = 0; i < m->length; i++) {
        free(m->order[i]->key);
        free(m->order[i]);
    }

    free(m->order);
    free(m);
}

/*
 * dt_map_len returns the number of keys in constant time.
 */
size_t dt_map_len(const dt_map *m)
{
    /* TODO: Return the current key count.
       Replacing a value does not change this count.
       after put alpha, beta, gamma:  dt_map_len(m) -> 3
       after put beta again:          dt_map_len(m) -> 3, still
       after del alpha:               dt_map_len(m) -> 2
       cases/normal/map_basics.case */

    return m->length;
}

/*
 * dt_map_put binds v to key.
 * An existing key keeps its insertion position. A new key becomes the last key.
 * Copy each new key because the caller owns the source buffer.
 * Return DT_ERR_CAPACITY after an allocation failure.
 */
dt_status dt_map_put(dt_map *m, const char *key, dt_value v)
{
    /* TODO: Replace the value for an existing key.
       Add a new entry for a new key. Copy each new key.
       Hash the key. Select its bucket. Search the bucket chain.
       Add a new entry to the chain and insertion list.
       put "beta" -> 2 on an empty map    -> DT_OK, "beta" is last in order
       put "beta" -> 22 on that map       -> DT_OK, same position, new value
       an allocation failure              -> DT_ERR_CAPACITY, map unchanged
       cases/normal/map_basics.case */
       
    size_t bucket = hash_key(key);

    /* Check whether the key already exists. */
    for (dt_map_entry *entry = m->buckets[bucket];
        entry != NULL;
        entry = entry->next) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = v;
            return DT_OK;
        }
    }

    /* Copy the new key. */
    size_t key_length = strlen(key);

    if (key_length == SIZE_MAX) {
        return DT_ERR_CAPACITY;
    }

    char *key_copy = malloc(key_length + 1);
    if (key_copy == NULL) {
        return DT_ERR_CAPACITY;
    }

    memcpy(key_copy, key, key_length + 1);

    /* Allocate the new map entry. */
    dt_map_entry *entry = malloc(sizeof(dt_map_entry));
    if (entry == NULL) {
        free(key_copy);
        return DT_ERR_CAPACITY;
    }

    /*
    * Grow the insertion-order array when necessary.
    * Nothing has been inserted into the map yet.
    */
    if (m->length == m->order_capacity) {
        size_t new_capacity =
            (m->order_capacity == 0) ? 4 : m->order_capacity * 2;

        if (new_capacity < m->order_capacity ||
            new_capacity > SIZE_MAX / sizeof(dt_map_entry *)) {
            free(entry);
            free(key_copy);
            return DT_ERR_CAPACITY;
        }

        dt_map_entry **new_order =
            realloc(m->order, new_capacity * sizeof(dt_map_entry *));

        if (new_order == NULL) {
            free(entry);
            free(key_copy);
            return DT_ERR_CAPACITY;
        }

        m->order = new_order;
        m->order_capacity = new_capacity;
    }

    /* All required allocations succeeded. */
    entry->key = key_copy;
    entry->value = v;
    entry->next = m->buckets[bucket];

    m->buckets[bucket] = entry;
    m->order[m->length] = entry;
    m->length++;

    return DT_OK;
}

/*
 * dt_map_get writes the value for key to *out.
 * It returns DT_ERR_KEY and does not change *out when the key is absent.
 * An absent key differs from a nil value.
 */
dt_status dt_map_get(const dt_map *m, const char *key, dt_value *out)
{
    /* TODO: Return DT_ERR_KEY when the key is absent.
       Preserve *out after this error. A nil value can be present.
       after put "beta" -> 22:
         dt_map_get(m, "beta", &out)   -> DT_OK, *out is the integer 22
         dt_map_get(m, "ghost", &out)  -> DT_ERR_KEY, *out untouched
       cases/normal/map_basics.case, cases/boundary/map_missing_key.case */

    size_t bucket = hash_key(key);

    for (dt_map_entry *entry = m->buckets[bucket];
        entry != NULL;
        entry = entry->next) {
        if (strcmp(entry->key, key) == 0) {
            *out = entry->value;
            return DT_OK;
        }
    }

    return DT_ERR_KEY;
}

/*
 * dt_map_remove removes key from its bucket and insertion position.
 * It releases the copied key. It returns DT_ERR_KEY when the key is absent.
 */
dt_status dt_map_remove(dt_map *m, const char *key)
{
    /* TODO: Remove the entry from its bucket and insertion position.
       Release the copied key. Return DT_ERR_KEY when the key is absent.
       a map holding alpha, beta, gamma:
         dt_map_remove(m, "alpha")  -> DT_OK, order is now beta, gamma
         dt_map_remove(m, "ghost")  -> DT_ERR_KEY, nothing changes
       reinserting "alpha" appends it after "gamma"
       cases/normal/map_basics.case, cases/boundary/map_remove_missing_key.case */

    size_t bucket = hash_key(key);

    dt_map_entry *previous = NULL;
    dt_map_entry *entry = m->buckets[bucket];

    while (entry != NULL && strcmp(entry->key, key) != 0) {
        previous = entry;
        entry = entry->next;
    }

    if (entry == NULL) {
        return DT_ERR_KEY;
    }

    /* Remove from the bucket chain. */
    if (previous == NULL) {
        m->buckets[bucket] = entry->next;
    } else {
        previous->next = entry->next;
    }

    /* Remove from insertion order. */
    size_t index = 0;

    while (index < m->length && m->order[index] != entry) {
        index++;
    }

    for (size_t i = index; i + 1 < m->length; i++) {
        m->order[i] = m->order[i + 1];
    }

    m->length--;

    free(entry->key);
    free(entry);

    return DT_OK;
}

/*
 * dt_map_key_at writes the key at insertion position index to *out.
 * It returns DT_ERR_RANGE and does not change *out for an invalid index.
 */
dt_status dt_map_key_at(const dt_map *m, size_t index, const char **out)
{
    /* TODO: Write the key at the specified insertion position to *out.
       Return DT_ERR_RANGE for an invalid position. Preserve *out after this error.
       The printer uses this order.
       a map holding alpha, beta, gamma:
         dt_map_key_at(m, 0, &out)  -> DT_OK, *out = "alpha"
         dt_map_key_at(m, 3, &out)  -> DT_ERR_RANGE, *out untouched
       cases/normal/map_basics.case */

    if (index >= m->length) {
        return DT_ERR_RANGE;
    }
    
    *out = m->order[index]->key;
    return DT_OK;
}
