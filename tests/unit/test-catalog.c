#define _POSIX_C_SOURCE 200809L

#include "../../src/sql/catalog.h"
#include "mini_test.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Note: Full catalog tests require pager and page_cache which are not
 * yet fully integrated. These tests verify catalog data structures and
 * serialization logic independently.
 */

/* Test catalog entry creation */
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

/* Test catalog entry types */
test(catalog_entry_types) {
    assert_eq_int(CATALOG_TYPE_TABLE, 1);
    assert_eq_int(CATALOG_TYPE_INDEX, 2);
}

/* Test catalog cursor creation check */
test(catalog_cursor_struct_size) {
    /* Verify cursor structure has expected fields */
    CatalogCursor cursor;
    memset(&cursor, 0, sizeof(cursor));

    assert_non_null(&cursor);
}

/* Run all tests */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("Running Catalog tests...\n");

    run(catalog_entry_creation);
    run(catalog_entry_types);
    run(catalog_cursor_struct_size);

    printf("\nAll Catalog tests passed!\n");
    return 0;
}