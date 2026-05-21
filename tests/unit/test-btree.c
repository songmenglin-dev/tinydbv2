#define _POSIX_C_SOURCE 200809L

#include "mini_test.h"
#include "../../src/storage/btree.h"
#include "../../src/storage/pager.h"
#include "../../src/storage/page_cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*============================================================================
 * Test declarations
 *============================================================================*/
test(btree_create_and_close);
test(btree_insert_and_find);
test(btree_cursor_first);
test(btree_cursor_next);
test(btree_cursor_last);
test(btree_cursor_valid);
test(btree_get_and_peek);
test(btree_range_scan);

/*============================================================================
 * Test file path helper
 *============================================================================*/
static char* get_test_db_path(const char* name) {
    char* path = malloc(256);
    snprintf(path, 256, "/tmp/tinydb_test_btree_%s.db", name);
    return path;
}

static void remove_test_db(const char* path) {
    unlink(path);
}

/*============================================================================
 * BTree tests
 *============================================================================*/

test(btree_create_and_close) {
    char* path = get_test_db_path("btree_create");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);
    assert_eq(tree->is_open, 1);

    assert_eq(btree_close(tree), 0);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_insert_and_find) {
    char* path = get_test_db_path("btree_insert_find");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert some key-value pairs */
    uint64_t key1 = 100;
    const char* val1 = "hello";
    assert_eq(btree_insert(tree, key1, val1, 5), 0);

    uint64_t key2 = 200;
    const char* val2 = "world";
    assert_eq(btree_insert(tree, key2, val2, 5), 0);

    /* Find key1 */
    BTreeCursor* cursor = btree_find(tree, key1);
    assert_non_null(cursor);

    /* Check cursor is valid */
    assert_eq(btree_cursor_valid(cursor), 1);

    btree_cursor_free(cursor);
    btree_close(tree);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_cursor_first) {
    char* path = get_test_db_path("btree_first");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert some keys */
    btree_insert(tree, 50, "a", 1);
    btree_insert(tree, 30, "b", 1);
    btree_insert(tree, 70, "c", 1);

    /* Get first cursor */
    BTreeCursor* cursor = btree_first(tree);
    assert_non_null(cursor);
    assert_eq(btree_cursor_valid(cursor), 1);

    btree_cursor_free(cursor);
    btree_close(tree);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_cursor_next) {
    char* path = get_test_db_path("btree_next");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert some keys */
    btree_insert(tree, 10, "v1", 2);
    btree_insert(tree, 20, "v2", 2);
    btree_insert(tree, 30, "v3", 2);

    /* Get first */
    BTreeCursor* cursor = btree_first(tree);
    assert_non_null(cursor);

    /* Advance */
    btree_cursor_next(cursor);
    assert_eq(btree_cursor_valid(cursor), 1);

    btree_cursor_free(cursor);
    btree_close(tree);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_cursor_last) {
    char* path = get_test_db_path("btree_last");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert some keys */
    btree_insert(tree, 10, "a", 1);
    btree_insert(tree, 20, "b", 1);
    btree_insert(tree, 30, "c", 1);

    /* Get last cursor */
    BTreeCursor* cursor = btree_last(tree);
    assert_non_null(cursor);
    assert_eq(btree_cursor_valid(cursor), 1);

    btree_cursor_free(cursor);
    btree_close(tree);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_cursor_valid) {
    char* path = get_test_db_path("btree_valid");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert a key */
    btree_insert(tree, 100, "value", 5);

    /* Get cursor and check validity */
    BTreeCursor* cursor = btree_find(tree, 100);
    assert_non_null(cursor);
    assert_eq(btree_cursor_valid(cursor), 1);

    btree_cursor_free(cursor);
    btree_close(tree);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_get_and_peek) {
    char* path = get_test_db_path("btree_get");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert a key-value */
    uint64_t key = 12345;
    const char* value = "test_value";
    btree_insert(tree, key, value, 10);

    /* Get cursor */
    BTreeCursor* cursor = btree_find(tree, key);
    assert_non_null(cursor);

    /* Get value */
    uint64_t got_key;
    char buf[256];
    uint32_t len;
    assert_eq(btree_get(cursor, &got_key, buf, &len), 0);
    assert_eq_uint64(got_key, key);
    assert_eq(len, 10);

    btree_cursor_free(cursor);
    btree_close(tree);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_range_scan) {
    char* path = get_test_db_path("btree_range");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert keys in range */
    for (int i = 10; i <= 50; i += 10) {
        char val[16];
        snprintf(val, sizeof(val), "val%d", i);
        btree_insert(tree, i, val, 6);
    }

    /* Create range scan */
    BTreeRange* range = btree_range_new(tree, 20, 40);
    assert_non_null(range);

    btree_range_free(range);
    btree_close(tree);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

/*============================================================================
 * Test runner
 *============================================================================*/
int main(int argc, char** argv) {
    int run_unit = 0;
    int run_integration = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--unit") == 0) run_unit = 1;
        if (strcmp(argv[i], "--integration") == 0) run_integration = 1;
    }

    printf("TinyDB v2 BTree Tests\n");
    printf("======================\n\n");

    if (run_unit) {
        printf("Unit Tests:\n");

        run(btree_create_and_close);
        run(btree_insert_and_find);
        run(btree_cursor_first);
        run(btree_cursor_next);
        run(btree_cursor_last);
        run(btree_cursor_valid);
        run(btree_get_and_peek);
        run(btree_range_scan);

        printf("\nAll btree unit tests passed!\n");
    }

    if (run_integration) {
        printf("\nIntegration Tests:\n");
        printf("  (none configured yet)\n");
    }

    printf("\n======================\n");
    printf("BTree test suite completed.\n");

    return 0;
}