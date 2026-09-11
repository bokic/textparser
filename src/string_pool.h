#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct textparser_string_pool_chunk {
    struct textparser_string_pool_chunk *next;
    size_t used;
    size_t capacity;
    char buffer[];
} textparser_string_pool_chunk;

typedef struct {
    textparser_string_pool_chunk *head;
} textparser_string_pool;

/**
 * Allocate and initialize a new string pool.
 *
 * @return Pointer to allocated textparser_string_pool or NULL on allocation failure.
 */
static inline textparser_string_pool *textparser_string_pool_create(void)
{
    return (textparser_string_pool *)calloc(1, sizeof(textparser_string_pool));
}

/**
 * Duplicate a string into chunked pool memory with bump allocation.
 *
 * @param pool String pool handle.
 * @param str Source string to duplicate.
 * @return Pointer to pooled string copy or NULL on error.
 */
static inline char *textparser_string_pool_strdup(textparser_string_pool *pool, const char *str)
{
    if (!pool || !str) return nullptr;

    size_t len = strlen(str) + 1;
    textparser_string_pool_chunk *chunk = pool->head;

    if (!chunk || (chunk->used + len > chunk->capacity)) {
        size_t cap = len > 4096 ? len : 4096;
        textparser_string_pool_chunk *new_chunk = (textparser_string_pool_chunk *)malloc(sizeof(textparser_string_pool_chunk) + cap);
        if (!new_chunk) return nullptr;
        new_chunk->next = pool->head;
        new_chunk->used = 0;
        new_chunk->capacity = cap;
        pool->head = new_chunk;
        chunk = new_chunk;
    }

    char *dest = chunk->buffer + chunk->used;
    memcpy(dest, str, len);
    chunk->used += len;
    return dest;
}

/**
 * Free all memory chunks and the string pool container itself.
 *
 * @param pool String pool handle to destroy.
 */
static inline void textparser_string_pool_free(textparser_string_pool *pool)
{
    if (!pool) return;
    textparser_string_pool_chunk *curr = pool->head;
    while (curr) {
        textparser_string_pool_chunk *next = curr->next;
        free(curr);
        curr = next;
    }
    free(pool);
}

