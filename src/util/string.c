#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*============================================================================
 * String length with overflow checking
 *============================================================================*/
size_t str_len(const char* s) {
    if (!s) return 0;
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

size_t str_nlen(const char* s, size_t max_len) {
    if (!s) return 0;
    size_t len = 0;
    while (len < max_len && s[len]) len++;
    return len;
}

/*============================================================================
 * String comparison
 *============================================================================*/
int str_eq(const char* a, const char* b) {
    if (!a || !b) return (a == b);
    return strcmp(a, b) == 0;
}

int str_ne(const char* a, const char* b) {
    return !str_eq(a, b);
}

int str_case_eq(const char* a, const char* b) {
    if (!a || !b) return (a == b);
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

/*============================================================================
 * String duplication
 *============================================================================*/
char* str_dup(const char* s) {
    if (!s) return NULL;
    size_t len = str_len(s);
    char* copy = mem_alloc(len + 1);
    if (copy) {
        memcpy(copy, s, len + 1);
    }
    return copy;
}

char* str_ndup(const char* s, size_t len) {
    if (!s) return NULL;
    size_t actual_len = str_nlen(s, len);
    char* copy = mem_alloc(actual_len + 1);
    if (copy) {
        memcpy(copy, s, actual_len);
        copy[actual_len] = '\0';
    }
    return copy;
}

/*============================================================================
 * String copying with bounds checking
 *============================================================================*/
size_t str_cpy(char* dest, size_t dest_size, const char* src) {
    if (!dest || !src || dest_size == 0) return 0;
    size_t src_len = str_len(src);
    size_t copy_len = src_len < dest_size - 1 ? src_len : dest_size - 1;
    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';
    return copy_len;
}

size_t str_ncpy(char* dest, size_t dest_size, const char* src, size_t src_len) {
    if (!dest || !src || dest_size == 0) return 0;
    size_t actual_src_len = src_len;
    size_t copy_len = actual_src_len < dest_size - 1 ? actual_src_len : dest_size - 1;
    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';
    return copy_len;
}

/*============================================================================
 * String concatenation
 *============================================================================*/
size_t str_cat(char* dest, size_t dest_size, const char* src) {
    if (!dest || !src || dest_size == 0) return 0;
    size_t dest_len = str_len(dest);
    size_t remaining = dest_size - dest_len;
    if (remaining <= 1) return dest_len;
    size_t copied = str_cpy(dest + dest_len, remaining, src);
    return dest_len + copied;
}

/*============================================================================
 * Memory allocation with overflow checking
 *============================================================================*/
void* mem_alloc(size_t size) {
    if (size == 0) return NULL;
    void* ptr = malloc(size);
    if (!ptr) {
        panic("Out of memory: cannot allocate %zu bytes", size);
    }
    return ptr;
}

void* mem_calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) return NULL;
    /* Check for overflow */
    if (nmemb > ((size_t)-1) / size) {
        panic("Out of memory: overflow in calloc(%zu, %zu)", nmemb, size);
    }
    void* ptr = calloc(nmemb, size);
    if (!ptr) {
        panic("Out of memory: cannot allocate %zu bytes", nmemb * size);
    }
    return ptr;
}

void* mem_realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        panic("Out of memory: cannot reallocate to %zu bytes", size);
    }
    return new_ptr;
}

void mem_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

/*============================================================================
 * Overflow-checked allocation helpers
 *============================================================================*/
void* mem_alloc_ovf(size_t size, int* overflow) {
    if (overflow) *overflow = 0;
    if (size == 0) return NULL;
    void* ptr = malloc(size);
    if (!ptr) {
        if (overflow) *overflow = 1;
    }
    return ptr;
}

void* mem_calloc_ovf(size_t nmemb, size_t size, int* overflow) {
    if (overflow) *overflow = 0;
    if (nmemb == 0 || size == 0) return NULL;
    if (nmemb > ((size_t)-1) / size) {
        if (overflow) *overflow = 1;
        return NULL;
    }
    void* ptr = calloc(nmemb, size);
    if (!ptr) {
        if (overflow) *overflow = 1;
    }
    return ptr;
}

/*============================================================================
 * List/array implementation
 *============================================================================*/
List* list_create(size_t initial_capacity) {
    List* list = mem_alloc(sizeof(List));
    if (!list) return NULL;

    if (initial_capacity == 0) {
        initial_capacity = 16;
    }

    list->items = mem_calloc(initial_capacity, sizeof(void*));
    if (!list->items) {
        mem_free(list);
        return NULL;
    }

    list->count = 0;
    list->capacity = initial_capacity;
    return list;
}

void list_destroy(List* list) {
    if (list) {
        mem_free(list->items);
        mem_free(list);
    }
}

int list_append(List* list, void* item) {
    if (!list) return -1;

    /* Check if we need to resize */
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity * 2;
        void** new_items = mem_realloc(list->items, new_capacity * sizeof(void*));
        if (!new_items) return -1;
        list->items = new_items;
        list->capacity = new_capacity;
    }

    list->items[list->count++] = item;
    return 0;
}

void* list_get(List* list, size_t index) {
    if (!list || index >= list->count) return NULL;
    return list->items[index];
}

size_t list_count(List* list) {
    return list ? list->count : 0;
}

int list_is_empty(List* list) {
    return !list || list->count == 0;
}
