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
test(btree_delete_basic);
test(btree_delete_nonexistent);
test(btree_update_existing);
test(btree_find_nonexistent);
test(btree_cursor_prev);
test(btree_multiple_inserts);
test(btree_page_split);
test(btree_verify_empty);
test(btree_verify_with_data);

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

test(btree_delete_basic) {
    char* path = get_test_db_path("btree_delete");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert a key-value */
    btree_insert(tree, 42, "answer", 6);

    /* Verify it exists */
    BTreeCursor* cursor = btree_find(tree, 42);
    assert_non_null(cursor);
    assert_eq(btree_cursor_valid(cursor), 1);
    btree_cursor_free(cursor);

    /* Delete it */
    assert_eq(btree_delete(tree, 42), 0);

    /* Verify it's gone */
    cursor = btree_find(tree, 42);
    assert_non_null(cursor);
    assert_eq(btree_cursor_valid(cursor), 0);
    btree_cursor_free(cursor);

    btree_close(tree);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_delete_nonexistent) {
    char* path = get_test_db_path("btree_delete_nonexist");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Delete non-existent key returns -1 (key not found) */
    assert_eq(btree_delete(tree, 999), -1);

    btree_close(tree);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_update_existing) {
    char* path = get_test_db_path("btree_update");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert initial value */
    btree_insert(tree, 1, "original", 8);

    /* Update with new value */
    assert_eq(btree_update(tree, 1, "updated", 7), 0);

    /* Verify updated value */
    BTreeCursor* cursor = btree_find(tree, 1);
    assert_non_null(cursor);
    uint64_t key;
    char buf[256];
    uint32_t len;
    assert_eq(btree_get(cursor, &key, buf, &len), 0);
    assert_eq(len, 7);
    assert_mem_eq(buf, "updated", 7);
    btree_cursor_free(cursor);

    btree_close(tree);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_find_nonexistent) {
    char* path = get_test_db_path("btree_find_nonexist");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert some keys */
    btree_insert(tree, 10, "ten", 3);
    btree_insert(tree, 20, "twenty", 6);

    /* Find non-existent key */
    BTreeCursor* cursor = btree_find(tree, 99);
    assert_non_null(cursor);
    assert_eq(btree_cursor_valid(cursor), 0);
    btree_cursor_free(cursor);

    btree_close(tree);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_cursor_prev) {
    char* path = get_test_db_path("btree_prev");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert keys */
    btree_insert(tree, 10, "a", 1);
    btree_insert(tree, 20, "b", 1);
    btree_insert(tree, 30, "c", 1);

    /* Get last then move backward */
    BTreeCursor* cursor = btree_last(tree);
    assert_non_null(cursor);
    assert_eq(btree_cursor_valid(cursor), 1);

    btree_cursor_prev(cursor);
    assert_eq(btree_cursor_valid(cursor), 1);

    btree_cursor_free(cursor);
    btree_close(tree);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_multiple_inserts) {
    char* path = get_test_db_path("btree_multiple");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert 100 keys */
    for (int i = 0; i < 100; i++) {
        char val[16];
        snprintf(val, sizeof(val), "val%d", i);
        assert_eq(btree_insert(tree, i, val, strlen(val)), 0);
    }

    /* Verify all can be found */
    for (int i = 0; i < 100; i++) {
        BTreeCursor* cursor = btree_find(tree, i);
        assert_non_null(cursor);
        assert_eq(btree_cursor_valid(cursor), 1);
        btree_cursor_free(cursor);
    }

    btree_close(tree);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_page_split) {
    char* path = get_test_db_path("btree_split");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert many keys to trigger page splits */
    for (int i = 0; i < 50; i++) {
        char val[64];
        snprintf(val, sizeof(val), "value_for_key_%d_this_is_a_long_value_to_fill_pages", i);
        assert_eq(btree_insert(tree, i, val, strlen(val)), 0);
    }

    /* Verify tree still works correctly */
    BTreeCursor* cursor = btree_first(tree);
    assert_non_null(cursor);
    assert_eq(btree_cursor_valid(cursor), 1);

    /* Iterate through all keys */
    int count = 0;
    while (btree_cursor_valid(cursor)) {
        count++;
        btree_cursor_next(cursor);
    }
    assert_eq(count, 50);
    btree_cursor_free(cursor);

    btree_close(tree);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

/*============================================================================
 * BTree verify tests
 *============================================================================*/

test(btree_verify_empty) {
    char* path = get_test_db_path("btree_verify_empty");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Verify empty tree returns SUCCESS */
    assert_eq(btree_verify(tree), SUCCESS);

    btree_close(tree);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(btree_verify_with_data) {
    char* path = get_test_db_path("btree_verify_data");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    BTree* tree = btree_create(pager, cache);
    assert_non_null(tree);

    /* Insert multiple key-value pairs */
    for (uint64_t i = 1; i <= 50; i++) {
        char val[16];
        snprintf(val, sizeof(val), "value%lu", (unsigned long)i);
        assert_eq(btree_insert(tree, i * 100, val, (uint32_t)strlen(val)), 0);
    }

    /* Verify tree with data returns SUCCESS */
    assert_eq(btree_verify(tree), SUCCESS);

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
        run(btree_delete_basic);
        run(btree_delete_nonexistent);
        run(btree_update_existing);
        run(btree_find_nonexistent);
        run(btree_cursor_prev);
        run(btree_multiple_inserts);
        run(btree_page_split);
        run(btree_verify_empty);
        run(btree_verify_with_data);

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