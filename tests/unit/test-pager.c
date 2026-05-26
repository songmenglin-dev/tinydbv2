#define _POSIX_C_SOURCE 200809L

#include "mini_test.h"
#include "../../src/storage/pager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*============================================================================
 * Test declarations
 *============================================================================*/
test(pager_create_and_close);
test(pager_create_and_open);
test(pager_allocate_pages);
test(pager_read_write_page);
test(pager_free_and_reuse_pages);
test(pager_header_operations);
test(pager_validate_magic);
test(pager_page_offset);

/*============================================================================
 * Test file path helper
 *============================================================================*/
static char* get_test_db_path(const char* name) {
    char* path = malloc(256);
    snprintf(path, 256, "/tmp/tinydb_test_%s.db", name);
    return path;
}

static void remove_test_db(const char* path) {
    unlink(path);
}

/*============================================================================
 * Pager tests
 *============================================================================*/

test(pager_create_and_close) {
    char* path = get_test_db_path("create_close");
    Pager* pager = pager_create(path);
    assert_non_null(pager);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(pager_create_and_open) {
    char* path = get_test_db_path("create_open");
    Pager* pager1 = pager_create(path);
    assert_non_null(pager1);
    pager_close(pager1);

    Pager* pager2 = pager_open(path);
    assert_non_null(pager2);
    pager_close(pager2);

    remove_test_db(path);
    free(path);
}

test(pager_allocate_pages) {
    char* path = get_test_db_path("alloc_pages");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    /* Allocate some pages */
    uint32_t page1, page2, page3;
    assert_eq(pager_allocate_page(pager, &page1), 0);
    assert_eq(pager_allocate_page(pager, &page2), 0);
    assert_eq(pager_allocate_page(pager, &page3), 0);

    /* Pages should be sequential (excluding header) */
    assert_eq_uint64(page1, 1u);  /* First allocated page after header */
    assert_eq_uint64(page2, 2u);
    assert_eq_uint64(page3, 3u);

    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(pager_read_write_page) {
    char* path = get_test_db_path("read_write");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    /* Allocate a page */
    uint32_t page_num;
    assert_eq(pager_allocate_page(pager, &page_num), 0);

    /* Write data to page */
    uint8_t write_buf[PAGE_SIZE];
    for (int i = 0; i < PAGE_SIZE; i++) {
        write_buf[i] = (uint8_t)(i & 0xFF);
    }
    assert_eq(pager_write_page(pager, page_num, write_buf), 0);

    /* Read back and verify */
    uint8_t read_buf[PAGE_SIZE];
    assert_eq(pager_read_page(pager, page_num, read_buf), 0);

    for (int i = 0; i < PAGE_SIZE; i++) {
        if (read_buf[i] != (uint8_t)(i & 0xFF)) {
            printf("FAIL: data mismatch at offset %d\n", i);
            exit(1);
        }
    }

    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(pager_free_and_reuse_pages) {
    char* path = get_test_db_path("free_reuse");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    /* Allocate pages */
    uint32_t page1, page2, page3;
    assert_eq(pager_allocate_page(pager, &page1), 0);
    assert_eq(pager_allocate_page(pager, &page2), 0);
    assert_eq(pager_allocate_page(pager, &page3), 0);

    /* Free page2 */
    assert_eq(pager_free_page(pager, page2), 0);

    /* Allocate again - should get page2 back from freelist */
    uint32_t page4;
    assert_eq(pager_allocate_page(pager, &page4), 0);
    assert_eq_uint64(page4, page2);

    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(pager_header_operations) {
    char* path = get_test_db_path("header_ops");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    /* Validate magic */
    assert_eq(pager_validate_magic(pager), 0);

    /* Get stats */
    PagerStats stats;
    pager_stats(pager, &stats);
    assert_eq_uint64(stats.page_count, 1u);  /* Just header page */
    assert_eq_uint64(stats.first_free_page, 0u);
    assert_eq_uint64(stats.free_page_count, 0u);

    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(pager_validate_magic) {
    char* path = get_test_db_path("magic");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    /* Valid magic should pass */
    assert_eq(pager_validate_magic(pager), 0);

    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(pager_page_offset) {
    /* Test page offset calculation */
    assert_eq(pager_get_page_offset(0), 0);
    assert_eq(pager_get_page_offset(1), PAGE_SIZE);
    assert_eq(pager_get_page_offset(2), PAGE_SIZE * 2);
    assert_eq(pager_get_page_offset(100), PAGE_SIZE * 100);
}

/*============================================================================
 * Test runner
 *============================================================================*/
/* Tests are run via test-suite.c - main() removed to avoid duplicate symbols */