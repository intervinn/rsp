#include "hashtable.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HT_INITIAL_CAPACITY 256

typedef struct {
    const char* key;
    void* value;
} HTEntry;

struct _HashTable {
    HTEntry* entries;
    uint64_t capacity;
    uint64_t length;
};

HashTable* ht_create() {
    HashTable* table = malloc(sizeof(HashTable));
    if (table == NULL) return NULL;
    table->length = 0;
    table->capacity = HT_INITIAL_CAPACITY;

    table->entries = calloc(table->capacity, sizeof(HTEntry));
    if (table->entries == NULL) {
        free(table);
        return NULL;
    }

    return table;
}

void ht_destroy(HashTable *t) {
    for (uint64_t i = 0; i < t->capacity; i++) {
        free((void*)t->entries[i].key);
        free((void*)t->entries[i].value);
    }

    free(t->entries);
    free(t);
}

// ======= some hash bullshi ================

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
static uint64_t hash_key(const char* key) {
    uint64_t hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

// ========= querying =======================

void* ht_get(HashTable* t, const char* key) {
    uint64_t hash = hash_key(key);
    uint64_t idx = (uint64_t)(hash & (uint64_t)(t->capacity - 1));

    // loop till empty entry is found
    while (t->entries[idx].key != NULL) {
        if (strcmp(key, t->entries[idx].key) == 0)
            return t->entries[idx].value;
        idx++;
        if (idx >= t->capacity)
            idx = 0;
    }
    return NULL;
}

// set without expanding
static const char* ht_set_entry(HTEntry* entries, uint64_t capacity, const char* key, void* value, uint64_t* sizeptr) {
    uint64_t hash = hash_key(key);
    uint64_t idx = (uint64_t)(hash & (uint64_t)(capacity - 1));

    while (entries[idx].key != NULL) {
        if (strcmp(key, entries[idx].key) == 0) {
            // key found, update value
            entries[idx].value = value;
            return entries[idx].key;
        }
        idx++;
        if (idx >= capacity)
            idx = 0;
    }

    // key not found, allocate then insert
    if (sizeptr != NULL) {
        key = strdup(key);
        if (key == NULL) {
            return NULL;
        }
        (*sizeptr)++;
    }
    entries[idx].key = (char*)key;
    entries[idx].value = value;
    return key;
}

// double hashtable capacity
// returns false if overflow
static bool ht_expand(HashTable* t) {
    uint64_t new_capacity = t->capacity * 2;
    if (new_capacity < t->capacity)
        return false; // integer overflow

    HTEntry* new_entries = calloc(new_capacity, sizeof(HTEntry));
    if (new_entries == NULL)
        return false;

    // iterate entries, move all nonempty ones to new table entries
    for (uint64_t i = 0; i < t->capacity; i++) {
        HTEntry e = t->entries[i];
        if (e.key != NULL) {
            ht_set_entry(new_entries, new_capacity, e.key, e.value, NULL);
        }
    }

    free(t->entries);
    t->entries = new_entries;
    t->capacity = new_capacity;
    return true;
}

const char* ht_set(HashTable* t, const char* key, void* val) {
    assert(val != NULL);
    if (val == NULL) 
        return NULL;
    

    if (t->length >= t->capacity  / 2) 
        if (!ht_expand(t)) 
            return NULL;
    

    return ht_set_entry(t->entries, t->capacity, key, val, &t->length);
}

uint64_t ht_len(HashTable* t) {
    return t->length;
}

HTIter ht_iter(HashTable *t) {
    HTIter hti;
    hti._table = t;
    hti._idx = 0;
    return hti;
}

bool ht_next(HTIter *it) {
    HashTable* t = it->_table;
    while (it->_idx < t->capacity) {
        uint64_t i = it->_idx;
        it->_idx++;
        if (t->entries[i].key != NULL) {
            HTEntry e = t->entries[i];
            it->key = e.key;
            it->value = e.value;
            return true;
        }
    }
    return false;
}
