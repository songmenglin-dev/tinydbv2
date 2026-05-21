#define _POSIX_C_SOURCE 200809L

#include "page_cache.h"
#include "pager.h"
#include "../util/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*============================================================================
 * Constants
 *============================================================================*/

#define HASH_LOAD_FACTOR 0.75

/*============================================================================
 * Hash function for page IDs
 *============================================================================*/

static uint32_t hash_page_id(uint32_t page_id, uint32_t table_size) {
    /* Simple hash - multiplication method */
    return (page_id * 31) & (table_size - 1);
}

/*============================================================================
 * LRU list operations
 *============================================================================*/

static void lru_remove(Page* page) {
    if (page->lru_prev) {
        page->lru_prev->lru_next = page->lru_next;
    }
    if (page->lru_next) {
        page->lru_next->lru_prev = page->lru_prev;
    }
    page->lru_prev = NULL;
    page->lru_next = NULL;
}

static void lru_add_front(PageCache* cache, Page* page) {
    page->lru_next = cache->lru_head;
    page->lru_prev = NULL;
    if (cache->lru_head) {
        cache->lru_head->lru_prev = page;
    }
    cache->lru_head = page;
    if (!cache->lru_tail) {
        cache->lru_tail = page;
    }
}

static void lru_add_back(PageCache* cache, Page* page) {
    page->lru_prev = cache->lru_tail;
    page->lru_next = NULL;
    if (cache->lru_tail) {
        cache->lru_tail->lru_next = page;
    }
    cache->lru_tail = page;
    if (!cache->lru_head) {
        cache->lru_head = page;
    }
}

/* Move page to front (most recently used) */
static void lru_mru(PageCache* cache, Page* page) {
    lru_remove(page);
    lru_add_front(cache, page);
}

/* Move page to back (least recently used) - for future LRU eviction */
__attribute__((unused))
static void lru_lru(PageCache* cache, Page* page) {
    lru_remove(page);
    lru_add_back(cache, page);
}

/*============================================================================
 * Hash table operations
 *============================================================================*/

static void hash_insert(PageCache* cache, Page* page) {
    uint32_t idx = hash_page_id(page->id, cache->table_size);
    page->hash_next = cache->pages[idx];
    page->hash_prev = NULL;
    if (cache->pages[idx]) {
        cache->pages[idx]->hash_prev = page;
    }
    cache->pages[idx] = page;
}

static void hash_remove(PageCache* cache, Page* page) {
    uint32_t idx = hash_page_id(page->id, cache->table_size);
    if (page->hash_prev) {
        page->hash_prev->hash_next = page->hash_next;
    } else {
        cache->pages[idx] = page->hash_next;
    }
    if (page->hash_next) {
        page->hash_next->hash_prev = page->hash_prev;
    }
    page->hash_next = NULL;
    page->hash_prev = NULL;
}

static Page* hash_find(PageCache* cache, uint32_t page_id) {
    uint32_t idx = hash_page_id(page_id, cache->table_size);
    Page* p = cache->pages[idx];
    while (p) {
        if (p->id == page_id) {
            return p;
        }
        p = p->hash_next;
    }
    return NULL;
}

/*============================================================================
 * Page allocation and deallocation
 *============================================================================*/

static Page* page_alloc(uint32_t page_id) {
    Page* page = calloc(1, sizeof(Page));
    if (!page) return NULL;

    page->data = calloc(1, PAGE_SIZE);
    if (!page->data) {
        free(page);
        return NULL;
    }

    page->id = page_id;
    page->refcount = 0;
    page->is_dirty = 0;
    page->is_wal = 0;
    page->lru_next = NULL;
    page->lru_prev = NULL;
    page->hash_next = NULL;
    page->hash_prev = NULL;

    return page;
}

static void page_free(Page* page) {
    if (!page) return;
    free(page->data);
    free(page);
}

/*============================================================================
 * LRU eviction
 *============================================================================*/

static int evict_lru_page(PageCache* cache) {
    /* Find a page to evict - start from LRU tail */
    Page* victim = cache->lru_tail;

    /* Try to find a page that's not pinned */
    while (victim) {
        if (victim->refcount == 0) {
            break;
        }
        victim = victim->lru_prev;
    }

    if (!victim) {
        /* No evictable pages */
        return -1;
    }

    /* Flush if dirty */
    if (victim->is_dirty && cache->pager) {
        if (pager_write_page(cache->pager, victim->id, victim->data) < 0) {
            return -1;
        }
    }

    /* Remove from LRU and hash */
    lru_remove(victim);
    hash_remove(cache, victim);

    cache->count--;
    cache->evict_count++;

    page_free(victim);
    return 0;
}

/*============================================================================
 * PageCache lifecycle
 *============================================================================*/

PageCache* page_cache_create(int max_pages, struct Pager* pager) {
    /* Validate max_pages */
    if (max_pages <= 0) {
        max_pages = DEFAULT_CACHE_SIZE;
    }
    if (max_pages > PAGER_MAX_PAGES) {
        max_pages = PAGER_MAX_PAGES;
    }

    /* Round up to power of 2 for hash table */
    int table_size = 16;
    while (table_size < max_pages * 2) {
        table_size *= 2;
    }

    PageCache* cache = calloc(1, sizeof(PageCache));
    if (!cache) return NULL;

    cache->pages = calloc(table_size, sizeof(Page*));
    if (!cache->pages) {
        free(cache);
        return NULL;
    }

    cache->table_size = table_size;
    cache->max_pages = max_pages;
    cache->pager = pager;
    cache->count = 0;
    cache->hit_count = 0;
    cache->miss_count = 0;
    cache->pin_count = 0;
    cache->evict_count = 0;
    cache->lru_head = NULL;
    cache->lru_tail = NULL;

    pthread_mutex_init(&cache->mutex, NULL);
    // pthread_rwlock_init(&cache->rwlock, NULL);

    return cache;
}

void page_cache_destroy(PageCache* cache) {
    if (!cache) return;

    /* Flush all dirty pages */
    page_cache_flush(cache);

    /* Free all pages */
    for (int i = 0; i < cache->table_size; i++) {
        Page* p = cache->pages[i];
        while (p) {
            Page* next = p->hash_next;
            page_free(p);
            p = next;
        }
    }

    free(cache->pages);
    pthread_mutex_destroy(&cache->mutex);
    // pthread_rwlock_destroy(&cache->rwlock);
    free(cache);
}

/*============================================================================
 * Page operations
 *============================================================================*/

Page* page_cache_get(PageCache* cache, uint32_t page_id) {
    if (!cache) return NULL;

    // pthread_rwlock_rdlock(&cache->rwlock);

    /* Look up in hash table */
    Page* page = hash_find(cache, page_id);

    if (page) {
        /* Cache hit */
        cache->hit_count++;
        lru_mru(cache, page);
        page->refcount++;
        cache->pin_count++;
        // pthread_rwlock_unlock(&cache->rwlock);
        return page;
    }

    /* Cache miss */
    cache->miss_count++;
    // pthread_rwlock_unlock(&cache->rwlock);

    /* Need to load page from disk */
    if (!cache->pager) {
        return NULL;
    }

    /* Acquire write lock to insert new page */
    // pthread_rwlock_wrlock(&cache->rwlock);

    /* Check again (another thread may have inserted it) */
    page = hash_find(cache, page_id);
    if (page) {
        cache->hit_count++;
        lru_mru(cache, page);
        page->refcount++;
        cache->pin_count++;
        // pthread_rwlock_unlock(&cache->rwlock);
        return page;
    }

    /* Evict if necessary */
    while (cache->count >= cache->max_pages) {
        if (evict_lru_page(cache) < 0) {
            // pthread_rwlock_unlock(&cache->rwlock);
            return NULL;
        }
    }

    /* Allocate new page */
    page = page_alloc(page_id);
    if (!page) {
        // pthread_rwlock_unlock(&cache->rwlock);
        return NULL;
    }

    /* Read page from disk */
    if (pager_read_page(cache->pager, page_id, page->data) < 0) {
        page_free(page);
        // pthread_rwlock_unlock(&cache->rwlock);
        return NULL;
    }

    /* Insert into hash and LRU */
    hash_insert(cache, page);
    lru_add_front(cache, page);
    cache->count++;

    page->refcount = 1;
    cache->pin_count++;

    // pthread_rwlock_unlock(&cache->rwlock);
    return page;
}

Page* page_cache_get_exclusive(PageCache* cache, uint32_t page_id) {
    /* For now, same as page_cache_get */
    /* In a more advanced implementation, we'd track exclusive owners */
    return page_cache_get(cache, page_id);
}

Page* page_pin(PageCache* cache, uint32_t page_id) {
    return page_cache_get(cache, page_id);
}

void page_unpin(Page* page) {
    if (!page) return;

    /* Decrement refcount */
    if (page->refcount > 0) {
        page->refcount--;
    }
}

void page_mark_dirty(Page* page) {
    if (!page) return;
    page->is_dirty = 1;
}

void page_cache_release(PageCache* cache, Page* page) {
    (void)cache;
    page_unpin(page);
}

/*============================================================================
 * Cache management
 *============================================================================*/

int page_cache_flush(PageCache* cache) {
    if (!cache) return -1;

    pthread_mutex_lock(&cache->mutex);

    int flushed = 0;
    for (int i = 0; i < cache->table_size; i++) {
        Page* p = cache->pages[i];
        while (p) {
            if (p->is_dirty && cache->pager) {
                if (pager_write_page(cache->pager, p->id, p->data) == 0) {
                    p->is_dirty = 0;
                    flushed++;
                }
            }
            p = p->hash_next;
        }
    }

    pthread_mutex_unlock(&cache->mutex);
    return flushed;
}

int page_cache_flush_page(PageCache* cache, uint32_t page_id) {
    if (!cache) return -1;

    pthread_mutex_lock(&cache->mutex);

    Page* page = hash_find(cache, page_id);
    if (!page) {
        pthread_mutex_unlock(&cache->mutex);
        return -1;
    }

    if (page->is_dirty && cache->pager) {
        int ret = pager_write_page(cache->pager, page->id, page->data);
        if (ret == 0) {
            page->is_dirty = 0;
            pthread_mutex_unlock(&cache->mutex);
            return 0;
        }
        pthread_mutex_unlock(&cache->mutex);
        return -1;
    }

    pthread_mutex_unlock(&cache->mutex);
    return 0;
}

int page_cache_evict(PageCache* cache, uint32_t page_id) {
    if (!cache) return -1;

    // pthread_rwlock_wrlock(&cache->rwlock);

    Page* page = hash_find(cache, page_id);
    if (!page) {
        // pthread_rwlock_unlock(&cache->rwlock);
        return -1;
    }

    if (page->refcount > 0) {
        // pthread_rwlock_unlock(&cache->rwlock);
        return -1;  /* Cannot evict pinned page */
    }

    /* Flush if dirty */
    if (page->is_dirty && cache->pager) {
        if (pager_write_page(cache->pager, page->id, page->data) < 0) {
            // pthread_rwlock_unlock(&cache->rwlock);
            return -1;
        }
    }

    lru_remove(page);
    hash_remove(cache, page);
    cache->count--;
    cache->evict_count++;

    page_free(page);

    // pthread_rwlock_unlock(&cache->rwlock);
    return 0;
}

void page_cache_clear(PageCache* cache) {
    if (!cache) return;

    // pthread_rwlock_wrlock(&cache->rwlock);

    for (int i = 0; i < cache->table_size; i++) {
        Page* p = cache->pages[i];
        while (p) {
            Page* next = p->hash_next;

            /* Flush if dirty */
            if (p->is_dirty && cache->pager) {
                pager_write_page(cache->pager, p->id, p->data);
            }

            lru_remove(p);
            page_free(p);
            p = next;
        }
        cache->pages[i] = NULL;
    }

    cache->count = 0;
    cache->lru_head = NULL;
    cache->lru_tail = NULL;

    // pthread_rwlock_unlock(&cache->rwlock);
}

/*============================================================================
 * Statistics
 *============================================================================*/

void page_cache_stats(PageCache* cache, int* hits, int* misses, int* size, int* pins) {
    if (!cache) return;

    pthread_mutex_lock(&cache->mutex);
    if (hits) *hits = cache->hit_count;
    if (misses) *misses = cache->miss_count;
    if (size) *size = cache->count;
    if (pins) *pins = cache->pin_count;
    pthread_mutex_unlock(&cache->mutex);
}

void page_cache_reset_stats(PageCache* cache) {
    if (!cache) return;

    pthread_mutex_lock(&cache->mutex);
    cache->hit_count = 0;
    cache->miss_count = 0;
    cache->pin_count = 0;
    cache->evict_count = 0;
    pthread_mutex_unlock(&cache->mutex);
}

double page_cache_hit_rate(PageCache* cache) {
    if (!cache) return 0.0;

    int hits, misses;
    page_cache_stats(cache, &hits, &misses, NULL, NULL);

    int total = hits + misses;
    if (total == 0) return 0.0;

    return (double)hits / (double)total;
}