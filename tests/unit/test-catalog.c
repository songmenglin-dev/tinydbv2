#define _POSIX_C_SOURCE 200809L

#include "mini_test.h"
#include "../../src/sql/catalog.h"
#include "../../src/storage/btree.h"
#include "../../src/storage/pager.h"
#include "../../src/storage/page_cache.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*============================================================================
 * Test declarations
 *============================================================================*/
test(catalog_entry_creation);
test(catalog_entry_types);
test(catalog_cursor_struct_size);
test(catalog_constants);
test(catalog_column_info_struct);
test(column_serialize_deserialize);
test(column_serialize_round_trip);
test(entry_serialize_deserialize);
test(entry_serialize_round_trip);
test(catalog_open_close);
test(catalog_init_and_reopen);
test(catalog_insert_entry);
test(catalog_lookup_by_type_name);
test(catalog_get_tables);
test(catalog_get_tables_empty);
test(catalog_cursor_iterate);
test(catalog_cursor_next_and_valid);
test(catalog_free_entries);

/*============================================================================
 * Test file path helpers
 *============================================================================*/
static char* get_test_db_path(const char* name) {
    char* path = malloc(256);
    snprintf(path, 256, "/tmp/tinydb_test_catalog_%s.db", name);
    return path;
}

static void remove_test_db(const char* path) {
    unlink(path);
}

/*============================================================================
 * Basic structure tests (no I/O)
 *============================================================================*/

test(catalog_entry_creation) {
    CatalogEntry entry;
    memset(&entry, 0, sizeof(entry));

    entry.type = CATALOG_TYPE_TABLE;
    strcpy(entry.name, "users");
    strcpy(entry.tbl_name, "users");
    strcpy(entry.sql, "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");

    assert_eq_int(entry.type, CATALOG_TYPE_TABLE);
    assert_str_eq(entry.name, "users");
    assert_str_eq(entry.tbl_name, "users");
}

test(catalog_entry_types) {
    assert_eq_int(CATALOG_TYPE_TABLE, 1);
    assert_eq_int(CATALOG_TYPE_INDEX, 2);
}

test(catalog_cursor_struct_size) {
    CatalogCursor cursor;
    memset(&cursor, 0, sizeof(cursor));
    assert_non_null(&cursor);
}

test(catalog_constants) {
    /* CATALOG_TABLE_NAME must be defined */
    assert_str_eq(CATALOG_TABLE_NAME, "tinydb_master");

    /* Entry type enum values */
    assert_eq_int(CATALOG_TYPE_TABLE, 1);
    assert_eq_int(CATALOG_TYPE_INDEX, 2);
}

test(catalog_column_info_struct) {
    ColumnInfo col;
    memset(&col, 0, sizeof(col));

    strcpy(col.name, "id");
    col.type = 0; /* COL_TYPE_INTEGER */
    col.not_null = 1;
    col.primary_key = 1;
    col.autoincrement = 1;
    strcpy(col.default_val, "0");

    assert_str_eq(col.name, "id");
    assert_eq_int(col.type, 0);
    assert_eq_int(col.not_null, 1);
    assert_eq_int(col.primary_key, 1);
    assert_eq_int(col.autoincrement, 1);
    assert_str_eq(col.default_val, "0");
}

/*============================================================================
 * Serialization tests
 *============================================================================*/

test(column_serialize_deserialize) {
    ColumnInfo original;
    memset(&original, 0, sizeof(original));
    strcpy(original.name, "username");
    original.type = 2; /* COL_TYPE_TEXT */
    original.not_null = 1;
    original.primary_key = 0;
    original.autoincrement = 0;
    strcpy(original.default_val, "guest");

    uint8_t buf[208];
    memset(buf, 0, sizeof(buf));

    /* Serialize */
    int ret = serialize_column_info(&original, buf);
    assert_eq_int(ret, SUCCESS);

    /* Verify some serialized bytes */
    /* name field starts at offset 0, null-terminated string "username" */
    assert_eq_int(buf[0], 'u');  /* first char of "username" */
    assert_eq_int(buf[7], 'e');   /* last char of "username" */
}

test(column_serialize_round_trip) {
    ColumnInfo original;
    memset(&original, 0, sizeof(original));
    strcpy(original.name, "created_at");
    original.type = 1; /* COL_TYPE_FLOAT */
    original.not_null = 1;
    original.primary_key = 0;
    original.autoincrement = 0;
    strcpy(original.default_val, "CURRENT_TIMESTAMP");

    uint8_t buf[208];
    memset(buf, 0, sizeof(buf));

    int ret = serialize_column_info(&original, buf);
    assert_eq_int(ret, SUCCESS);

    ColumnInfo restored;
    memset(&restored, 0, sizeof(restored));
    ret = deserialize_column_info(buf, &restored);
    assert_eq_int(ret, SUCCESS);

    assert_str_eq(restored.name, "created_at");
    assert_eq_int(restored.type, 1);
    assert_eq_int(restored.not_null, 1);
    assert_eq_int(restored.primary_key, 0);
    assert_eq_int(restored.autoincrement, 0);
    assert_str_eq(restored.default_val, "CURRENT_TIMESTAMP");
}

test(entry_serialize_deserialize) {
    CatalogEntry original;
    memset(&original, 0, sizeof(original));

    original.type = CATALOG_TYPE_TABLE;
    strcpy(original.name, "products");
    strcpy(original.tbl_name, "products");
    strcpy(original.sql, "CREATE TABLE products (id INTEGER, name TEXT)");
    original.root_page = 42;
    original.is_valid = 1;
    original.column_count = 2;

    original.columns = calloc(2, sizeof(ColumnInfo));
    assert_non_null(original.columns);

    strcpy(original.columns[0].name, "id");
    original.columns[0].type = 0;
    original.columns[0].not_null = 1;
    original.columns[0].primary_key = 1;

    strcpy(original.columns[1].name, "name");
    original.columns[1].type = 2;
    original.columns[1].not_null = 0;
    original.columns[1].primary_key = 0;

    uint8_t buf[2048];  /* base(656) + 2*col(416) = 1072, use 2048 for safety */
    size_t out_size = 0;

    int ret = serialize_entry(&original, buf, sizeof(buf), &out_size);
    assert_eq_int(ret, SUCCESS);
    assert_true(out_size > 656); /* at least base + 2 columns */

    /* Deserialize */
    CatalogEntry restored;
    memset(&restored, 0, sizeof(restored));

    ret = deserialize_entry(buf, out_size, &restored);
    /* If FAIL here, check: out_size must be <= sizeof(buf) and column_count matches */
    assert_eq_int(ret, SUCCESS);

    assert_eq_int(restored.type, CATALOG_TYPE_TABLE);
    assert_str_eq(restored.name, "products");
    assert_str_eq(restored.tbl_name, "products");
    assert_eq_int(restored.root_page, 42);
    assert_eq_int(restored.is_valid, 1);
    assert_eq_int(restored.column_count, 2);
    assert_non_null(restored.columns);

    assert_str_eq(restored.columns[0].name, "id");
    assert_eq_int(restored.columns[0].type, 0);
    assert_eq_int(restored.columns[0].primary_key, 1);
    assert_str_eq(restored.columns[1].name, "name");
    assert_eq_int(restored.columns[1].type, 2);

    free(original.columns);
    free(restored.columns);
}

test(entry_serialize_round_trip) {
    /* Entry with no columns */
    CatalogEntry original;
    memset(&original, 0, sizeof(original));

    original.type = CATALOG_TYPE_INDEX;
    strcpy(original.name, "idx_users_email");
    strcpy(original.tbl_name, "users");
    strcpy(original.sql, "CREATE INDEX idx_users_email ON users(email)");
    original.root_page = 10;
    original.is_valid = 1;
    original.column_count = 0;
    original.columns = NULL;

    uint8_t buf[2048];  /* base(656) + 2*col(416) = 1072, use 2048 for safety */
    size_t out_size = 0;

    int ret = serialize_entry(&original, buf, sizeof(buf), &out_size);
    assert_eq_int(ret, SUCCESS);

    CatalogEntry restored;
    memset(&restored, 0, sizeof(restored));

    ret = deserialize_entry(buf, out_size, &restored);
    assert_eq_int(ret, SUCCESS);

    assert_eq_int(restored.type, CATALOG_TYPE_INDEX);
    assert_str_eq(restored.name, "idx_users_email");
    assert_str_eq(restored.tbl_name, "users");
    assert_eq_int(restored.root_page, 10);
    assert_eq_int(restored.is_valid, 1);
    assert_eq_int(restored.column_count, 0);
    assert_null(restored.columns);
}

/*============================================================================
 * Full catalog lifecycle tests (with real BTree)
 *============================================================================*/

test(catalog_open_close) {
    char* path = get_test_db_path("open_close");
    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    Catalog* catalog = catalog_open(pager, cache);
    assert_non_null(catalog);
    assert_eq_int(catalog->is_open, 1);

    catalog_close(catalog);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(catalog_init_and_reopen) {
    char* path = get_test_db_path("init_reopen");
    remove_test_db(path);

    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    Catalog* catalog = catalog_open(pager, cache);
    assert_non_null(catalog);

    int ret = catalog_init(catalog);
    assert_eq_int(ret, SUCCESS);

    /* Verify tinydb_master entry exists and cursor works */
    CatalogCursor* cursor = catalog_cursor_create(catalog);
    assert_non_null(cursor);
    assert_eq_int(catalog_cursor_valid(cursor), 1);

    CatalogEntry* entry = catalog_cursor_get(cursor);
    assert_non_null(entry);
    /* Name check verifies entry was loaded from storage */
    assert_str_eq(entry->name, CATALOG_TABLE_NAME);
    free(entry);

    catalog_cursor_free(cursor);
    catalog_close(catalog);

    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(catalog_insert_entry) {
    char* path = get_test_db_path("insert");
    remove_test_db(path);

    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    Catalog* catalog = catalog_open(pager, cache);
    assert_non_null(catalog);

    int ret = catalog_init(catalog);
    assert_eq_int(ret, SUCCESS);

    /* Insert a table entry */
    CatalogEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.type = CATALOG_TYPE_TABLE;
    strcpy(entry.name, "orders");
    strcpy(entry.tbl_name, "orders");
    strcpy(entry.sql, "CREATE TABLE orders (id INTEGER, amount REAL)");
    entry.root_page = 5;
    entry.is_valid = 1;

    ret = catalog_insert(catalog, &entry);
    assert_eq_int(ret, SUCCESS);

    catalog_close(catalog);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(catalog_lookup_by_type_name) {
    char* path = get_test_db_path("lookup");
    remove_test_db(path);

    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    Catalog* catalog = catalog_open(pager, cache);
    assert_non_null(catalog);

    int ret = catalog_init(catalog);
    assert_eq_int(ret, SUCCESS);

    /* Insert a table entry with columns */
    CatalogEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.type = CATALOG_TYPE_TABLE;
    strcpy(entry.name, "customers");
    strcpy(entry.tbl_name, "customers");
    strcpy(entry.sql, "CREATE TABLE customers (id INTEGER, name TEXT)");
    entry.root_page = 3;
    entry.is_valid = 1;
    entry.column_count = 2;
    entry.columns = calloc(2, sizeof(ColumnInfo));
    strcpy(entry.columns[0].name, "id");
    entry.columns[0].type = 0;
    entry.columns[0].primary_key = 1;
    strcpy(entry.columns[1].name, "name");
    entry.columns[1].type = 2;

    ret = catalog_insert(catalog, &entry);
    assert_eq_int(ret, SUCCESS);
    free(entry.columns);

    /* Lookup the table */
    CatalogEntry* found = catalog_lookup_type_name(catalog, CATALOG_TYPE_TABLE, "customers");
    assert_non_null(found);
    assert_str_eq(found->name, "customers");
    assert_str_eq(found->tbl_name, "customers");
    assert_eq_int(found->type, CATALOG_TYPE_TABLE);
    free(found);

    /* Lookup non-existent table */
    CatalogEntry* not_found = catalog_lookup_type_name(catalog, CATALOG_TYPE_TABLE, "nonexistent");
    assert_null(not_found);

    /* Lookup with wrong type */
    CatalogEntry* wrong_type = catalog_lookup_type_name(catalog, CATALOG_TYPE_INDEX, "customers");
    assert_null(wrong_type);

    catalog_close(catalog);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(catalog_get_tables) {
    char* path = get_test_db_path("get_tables");
    remove_test_db(path);

    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    Catalog* catalog = catalog_open(pager, cache);
    assert_non_null(catalog);

    int ret = catalog_init(catalog);
    assert_eq_int(ret, SUCCESS);

    /* Insert two tables */
    CatalogEntry t1;
    memset(&t1, 0, sizeof(t1));
    t1.type = CATALOG_TYPE_TABLE;
    strcpy(t1.name, "users");
    strcpy(t1.tbl_name, "users");
    strcpy(t1.sql, "CREATE TABLE users (id INTEGER)");
    t1.root_page = 2;
    t1.is_valid = 1;
    catalog_insert(catalog, &t1);

    CatalogEntry t2;
    memset(&t2, 0, sizeof(t2));
    t2.type = CATALOG_TYPE_TABLE;
    strcpy(t2.name, "posts");
    strcpy(t2.tbl_name, "posts");
    strcpy(t2.sql, "CREATE TABLE posts (id INTEGER)");
    t2.root_page = 4;
    t2.is_valid = 1;
    catalog_insert(catalog, &t2);

    /* Insert an index (should be excluded from get_tables) */
    CatalogEntry idx;
    memset(&idx, 0, sizeof(idx));
    idx.type = CATALOG_TYPE_INDEX;
    strcpy(idx.name, "idx_posts_user");
    strcpy(idx.tbl_name, "posts");
    idx.is_valid = 1;
    catalog_insert(catalog, &idx);

    int count = 0;
    CatalogEntry** tables = catalog_get_tables(catalog, &count);
    assert_non_null(tables);
    assert_eq_int(count, 2);

    catalog_free_entries(tables, count);

    catalog_close(catalog);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(catalog_get_tables_empty) {
    char* path = get_test_db_path("get_tables_empty");
    remove_test_db(path);

    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    Catalog* catalog = catalog_open(pager, cache);
    assert_non_null(catalog);

    int ret = catalog_init(catalog);
    assert_eq_int(ret, SUCCESS);

    /* Only tinydb_master exists - should not be returned by get_tables */
    int count = 0;
    CatalogEntry** tables = catalog_get_tables(catalog, &count);
    /* catalog_get_tables returns NULL when count is 0 (no user tables) */
    assert_eq_int(count, 0);
    /* tables may be NULL when empty - free NULL is safe */
    catalog_free_entries(tables, count);

    catalog_free_entries(tables, count);

    catalog_close(catalog);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(catalog_cursor_iterate) {
    char* path = get_test_db_path("cursor_iter");
    remove_test_db(path);

    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    Catalog* catalog = catalog_open(pager, cache);
    assert_non_null(catalog);

    int ret = catalog_init(catalog);
    assert_eq_int(ret, SUCCESS);

    CatalogCursor* cursor = catalog_cursor_create(catalog);
    assert_non_null(cursor);
    assert_eq_int(catalog_cursor_valid(cursor), 1);

    CatalogEntry* first = catalog_cursor_get(cursor);
    assert_non_null(first);
    assert_str_eq(first->name, CATALOG_TABLE_NAME);
    free(first);

    catalog_cursor_free(cursor);
    catalog_close(catalog);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(catalog_cursor_next_and_valid) {
    char* path = get_test_db_path("cursor_next");
    remove_test_db(path);

    Pager* pager = pager_create(path);
    assert_non_null(pager);

    PageCache* cache = page_cache_create(16, pager);
    assert_non_null(cache);

    Catalog* catalog = catalog_open(pager, cache);
    assert_non_null(catalog);

    int ret = catalog_init(catalog);
    assert_eq_int(ret, SUCCESS);

    /* Insert a user table */
    CatalogEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.type = CATALOG_TYPE_TABLE;
    strcpy(entry.name, "logs");
    strcpy(entry.tbl_name, "logs");
    entry.is_valid = 1;
    catalog_insert(catalog, &entry);

    /* Iterate with cursor */
    CatalogCursor* cursor = catalog_cursor_create(catalog);
    assert_non_null(cursor);

    int entry_count = 0;
    while (catalog_cursor_valid(cursor)) {
        CatalogEntry* e = catalog_cursor_get(cursor);
        if (e) {
            entry_count++;
            free(e);
        }
        catalog_cursor_next(cursor);
    }

    /* Should have tinydb_master + logs = 2 entries */
    assert_eq_int(entry_count, 2);

    catalog_cursor_free(cursor);
    catalog_close(catalog);
    page_cache_destroy(cache);
    pager_close(pager);
    remove_test_db(path);
    free(path);
}

test(catalog_free_entries) {
    /* Test catalog_free_entries with NULL */
    catalog_free_entries(NULL, 0);

    /* Test with empty array */
    catalog_free_entries(NULL, 5);

    /* Test allocating and freeing entries */
    CatalogEntry** entries = calloc(3, sizeof(CatalogEntry*));
    assert_non_null(entries);

    for (int i = 0; i < 3; i++) {
        entries[i] = calloc(1, sizeof(CatalogEntry));
        strcpy(entries[i]->name, "table");
    }

    catalog_free_entries(entries, 3);
}
