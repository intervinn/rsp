#ifndef _HT_H
#define _HT_H
#include <stdbool.h>
#include <stdint.h>

// todo: add thread-safe layer, as this structure is thread-unsafe by design

typedef struct _HashTable HashTable;

HashTable* ht_create();
void ht_destroy(HashTable* t);
void* ht_get(HashTable* t, const char* key);
const char* ht_set(HashTable* t, const char* key, void* val);
uint64_t ht_len(HashTable* t);

typedef struct {
    const char* key; // current key
    void* value;

    HashTable* _table;
    uint64_t _idx;
} HTIter; 


HTIter ht_iter(HashTable* t);

// dont call ht_set during iteration
bool ht_next(HTIter* it);


#endif // _HT_H