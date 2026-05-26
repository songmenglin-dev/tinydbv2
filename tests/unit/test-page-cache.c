#define _POSIX_C_SOURCE 200809L

#include "mini_test.h"
#include "../../src/storage/page_cache.h"
#include "../../src/storage/pager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/*============================================================================
 * Test declarations
 *============================================================================*/
test(page_cache_create_and_destroy);
test(page_cache_get_page);
test(page_cache_pin_unpin);
test(page_cache_mark_dirty);
test(page_cache_flush);
test(page_cache_stats);
test(page_cache_hit_rate);
test(page_cache_lru_eviction);

/*============================================================================
 * Test file path helper
 *============================================================================*/
static char* get_test_db_path(const char* name) {
    char* path = malloc(256);
    snprintf(path, 256, "/tmp/tinydb_test_cache_%s.db", name);
    return path;
}

static void remove_test_db(const char* path) {
    unlink(path);
}

/*============================================================================
 * Page Cache tests
 *============================================================================*/

test(page_cache_create_and_destroy) {
    char* path = get_test_db_path("cache_create");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(page_cache_get_page) {
    char* path = get_test_db_path("cache_get");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    /* Allocate a page */
    uint32_t page_num;
    pager_allocate_page(pager, &page_num);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    /* Get page */
    Page* page = page_pin(cache, page_num);
    assert_non_null(page);
    assert_eq(page->id, page_num);

    page_unpin(page);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(page_cache_pin_unpin) {
    char* path = get_test_db_path("cache_pin");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    uint32_t page_num;
    pager_allocate_page(pager, &page_num);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    Page* page = page_pin(cache, page_num);
    assert_non_null(page);
    assert_eq(page->refcount, 1);

    /* Pin again */
    Page* page2 = page_pin(cache, page_num);
    assert_non_null(page2);
    assert_eq(page->refcount, 2);

    page_unpin(page);
    page_unpin(page2);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(page_cache_mark_dirty) {
    char* path = get_test_db_path("cache_dirty");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    uint32_t page_num;
    pager_allocate_page(pager, &page_num);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    Page* page = page_pin(cache, page_num);
    assert_non_null(page);

    /* Mark as dirty */
    assert_eq(page->is_dirty, 0);
    page_mark_dirty(page);
    assert_eq(page->is_dirty, 1);

    page_unpin(page);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(page_cache_flush) {
    char* path = get_test_db_path("cache_flush");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    uint32_t page_num;
    pager_allocate_page(pager, &page_num);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    Page* page = page_pin(cache, page_num);
    page_mark_dirty(page);
    page_unpin(page);

    /* Flush */
    int flushed = page_cache_flush(cache);
    assert_eq(flushed, 1);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(page_cache_stats) {
    char* path = get_test_db_path("cache_stats");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    /* Allocate some pages */
    for (int i = 0; i < 5; i++) {
        uint32_t page_num;
        pager_allocate_page(pager, &page_num);
        Page* page = page_pin(cache, page_num);
        page_unpin(page);
    }

    /* Get stats */
    int hits, misses, size, pins;
    page_cache_stats(cache, &hits, &misses, &size, &pins);
    assert_true(size >= 5);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(page_cache_hit_rate) {
    char* path = get_test_db_path("cache_hit_rate");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    /* Allocate a page */
    uint32_t page_num;
    pager_allocate_page(pager, &page_num);

    /* First access - should be miss */
    page_pin(cache, page_num);
    page_unpin(page_pin(cache, page_num));

    /* Second access - should be hit */
    page_pin(cache, page_num);

    /* Check hit rate */
    double rate = page_cache_hit_rate(cache);
    assert_true(rate > 0.0);

    page_unpin(page_pin(cache, page_num));

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(page_cache_lru_eviction) {
    char* path = get_test_db_path("cache_lru");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    /* Create cache with small size */
    PageCache* cache = page_cache_create(4, pager);
    assert_non_null(cache);

    /* Allocate 5 pages */
    uint32_t pages[5];
    for (int i = 0; i < 5; i++) {
        pager_allocate_page(pager, &pages[i]);
    }

    /* Pin all pages */
    for (int i = 0; i < 5; i++) {
        Page* page = page_pin(cache, pages[i]);
        page_unpin(page);
    }

    /* Check that we can still get pages (eviction happened) */
    Page* page = page_pin(cache, pages[0]);
    assert_non_null(page);
    page_unpin(page);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}