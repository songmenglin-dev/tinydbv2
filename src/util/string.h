#ifndef TINYDB_STRING_H
#define TINYDB_STRING_H

#include <stddef.h>
#include <stdint.h>
#include "error.h"

/*============================================================================
 * String utilities
 *============================================================================*/

/* String length with overflow checking */
size_t str_len(const char* s);
size_t str_nlen(const char* s, size_t max_len);

/* String comparison */
int str_eq(const char* a, const char* b);
int str_ne(const char* a, const char* b);
int str_case_eq(const char* a, const char* b);

/* String duplication */
char* str_dup(const char* s);
char* str_ndup(const char* s, size_t len);

/* String copying with bounds checking */
size_t str_cpy(char* dest, size_t dest_size, const char* src);
size_t str_ncpy(char* dest, size_t dest_size, const char* src, size_t src_len);

/* String concatenation */
size_t str_cat(char* dest, size_t dest_size, const char* src);

/* Memory buffer operations */
void* mem_alloc(size_t size);
void* mem_calloc(size_t nmemb, size_t size);
void* mem_realloc(void* ptr, size_t size);
void  mem_free(void* ptr);

/* Overflow-checked allocation helpers */
void* mem_alloc_ovf(size_t size, int* overflow);
void* mem_calloc_ovf(size_t nmemb, size_t size, int* overflow);

/*============================================================================
 * List/array data structures
 *============================================================================*/
typedef struct {
    void** items;
    size_t count;
    size_t capacity;
} List;

List* list_create(size_t initial_capacity);
void list_destroy(List* list);
int list_append(List* list, void* item);
void* list_get(List* list, size_t index);
size_t list_count(List* list);
int list_is_empty(List* list);

#endif /* TINYDB_STRING_H */
