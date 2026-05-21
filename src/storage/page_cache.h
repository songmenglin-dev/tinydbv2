#ifndef TINYDB_PAGE_CACHE_H
#define TINYDB_PAGE_CACHE_H

#define _POSIX_C_SOURCE 200809L

#include "../../include/tinydb.h"
#include "../../include/types.h"
#include <pthread.h>

/*============================================================================
 * Page Cache - LRU Cache for Database Pages
 *============================================================================*/

/* Page states */
typedef enum {
    PAGE_CLEAN = 0,
    PAGE_DIRTY = 1
} PageState;

/* Page structure */
typedef struct Page {
    void* data;              /* Page data (PAGE_SIZE bytes) */
    uint32_t id;             /* Page number */
    int refcount;            /* Reference count (pin/unpin) */
    int is_dirty;            /* Modified since last flush */
    int is_wal;              /* Page is from WAL (not yet applied) */

    /* LRU list linkage */
    struct Page* lru_next;
    struct Page* lru_prev;

    /* Hash linkage */
    struct Page* hash_next;
    struct Page* hash_prev;
} Page;

/* Page cache with LRU eviction */
typedef struct PageCache {
    /* Page array and hash table */
    Page** pages;            /* Hash table: page_id -> Page* */
    int table_size;          /* Hash table size (power of 2) */

    /* LRU list for eviction */
    Page* lru_head;          /* Most recently used */
    Page* lru_tail;          /* Least recently used */

    /* Cache statistics */
    int count;               /* Current page count */
    int max_pages;           /* Maximum pages (from config) */
    int hit_count;           /* Cache hits */
    int miss_count;          /* Cache misses */

    /* Pager reference */
    struct Pager* pager;      /* Reference to underlying pager */

    /* Thread safety */
    pthread_mutex_t mutex;
    pthread_rwlock_t rwlock;  /* Read-write lock for page access */

    /* Statistics */
    int pin_count;           /* Current pins */
    int evict_count;         /* Total evictions */
} PageCache;

/*============================================================================
 * Page Cache lifecycle
 *============================================================================*/

/* Create a new page cache */
PageCache* page_cache_create(int max_pages, struct Pager* pager);

/* Destroy page cache and release all pages */
void page_cache_destroy(PageCache* cache);

/*============================================================================
 * Page operations
 *============================================================================*/

/* Get a page (shared read lock) */
Page* page_cache_get(PageCache* cache, uint32_t page_id);

/* Get a page for writing (exclusive lock) */
Page* page_cache_get_exclusive(PageCache* cache, uint32_t page_id);

/* Pin a page (prevent eviction) */
Page* page_pin(PageCache* cache, uint32_t page_id);

/* Unpin a page (allow eviction) */
void page_unpin(Page* page);

/* Mark page as dirty (modified) */
void page_mark_dirty(Page* page);

/* Release page back to cache (after pin) */
void page_cache_release(PageCache* cache, Page* page);

/*============================================================================
 * Cache management
 *============================================================================*/

/* Flush all dirty pages to disk */
int page_cache_flush(PageCache* cache);

/* Flush a specific page to disk */
int page_cache_flush_page(PageCache* cache, uint32_t page_id);

/* Evict a page from cache (force) */
int page_cache_evict(PageCache* cache, uint32_t page_id);

/* Clear all pages from cache */
void page_cache_clear(PageCache* cache);

/*============================================================================
 * Statistics
 *============================================================================*/

/* Get cache statistics */
void page_cache_stats(PageCache* cache, int* hits, int* misses, int* size, int* pins);

/* Reset statistics */
void page_cache_reset_stats(PageCache* cache);

/* Get hit rate */
double page_cache_hit_rate(PageCache* cache);

#endif /* TINYDB_PAGE_CACHE_H */