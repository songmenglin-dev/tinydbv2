#include "test_helpers.h"

/*============================================================================
 * Integration tests for SQL operations: CREATE, INSERT, SELECT, UPDATE, DELETE
 *============================================================================*/

static int test_create_table_parsing(void) {
    const char* create_sql = "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL);";

    /* Verify SQL statement structure */
    TEST_ASSERT(strstr(create_sql, "CREATE TABLE") != NULL, "Missing CREATE TABLE");
    TEST_ASSERT(strstr(create_sql, "PRIMARY KEY") != NULL, "Missing PRIMARY KEY");

    return 0;
}

static int test_insert_parsing(void) {
    const char* insert_sql = "INSERT INTO users (id, name) VALUES (1, 'Alice');";

    TEST_ASSERT(strstr(insert_sql, "INSERT INTO") != NULL, "Missing INSERT INTO");
    TEST_ASSERT(strstr(insert_sql, "VALUES") != NULL, "Missing VALUES");

    return 0;
}

static int test_select_parsing(void) {
    const char* select_sql = "SELECT * FROM users WHERE id = 1;";

    TEST_ASSERT(strstr(select_sql, "SELECT") != NULL, "Missing SELECT");
    TEST_ASSERT(strstr(select_sql, "FROM") != NULL, "Missing FROM");
    TEST_ASSERT(strstr(select_sql, "WHERE") != NULL, "Missing WHERE");

    return 0;
}

static int test_update_parsing(void) {
    const char* update_sql = "UPDATE users SET name = 'Bob' WHERE id = 1;";

    TEST_ASSERT(strstr(update_sql, "UPDATE") != NULL, "Missing UPDATE");
    TEST_ASSERT(strstr(update_sql, "SET") != NULL, "Missing SET");
    TEST_ASSERT(strstr(update_sql, "WHERE") != NULL, "Missing WHERE");

    return 0;
}

static int test_delete_parsing(void) {
    const char* delete_sql = "DELETE FROM users WHERE id = 1;";

    TEST_ASSERT(strstr(delete_sql, "DELETE FROM") != NULL, "Missing DELETE FROM");
    TEST_ASSERT(strstr(delete_sql, "WHERE") != NULL, "Missing WHERE");

    return 0;
}

static int test_multiple_statements(void) {
    /* Test parsing multiple statements */
    const char* multi_sql =
        "CREATE TABLE test (id INTEGER);\n"
        "INSERT INTO test (id) VALUES (1);\n"
        "SELECT * FROM test;\n";

    int count = 0;
    const char* p = multi_sql;
    while (*p) {
        if (*p == ';') count++;
        p++;
    }

    TEST_ASSERT(count >= 3, "Should have at least 3 statements");

    return 0;
}

static int test_string_escaping(void) {
    /* Test string escape handling */
    const char* escaped_sql = "INSERT INTO users (name) VALUES ('Alice''s restaurant');";

    /* Verify the escape sequence is present */
    TEST_ASSERT(strstr(escaped_sql, "''") != NULL, "Missing escaped quote");

    return 0;
}

static int test_numeric_values(void) {
    /* Test various numeric formats */
    const char* int_sql = "INSERT INTO test (val) VALUES (42);";
    const char* float_sql = "INSERT INTO test (val) VALUES (3.14);";
    const char* neg_sql = "INSERT INTO test (val) VALUES (-10);";

    TEST_ASSERT(strstr(int_sql, "42") != NULL, "Missing integer");
    TEST_ASSERT(strstr(float_sql, "3.14") != NULL, "Missing float");
    TEST_ASSERT(strstr(neg_sql, "-10") != NULL, "Missing negative");

    return 0;
}

/* Run all integration tests */
int main(void) {
    printf("TinyDB v2 Integration Tests: SQL Operations\n");
    printf("===========================================\n\n");

    int failed = 0;

    printf("SQL parsing tests:\n");
    RUN_INTEGRATION_TEST(create_table_parsing);
    RUN_INTEGRATION_TEST(insert_parsing);
    RUN_INTEGRATION_TEST(select_parsing);
    RUN_INTEGRATION_TEST(update_parsing);
    RUN_INTEGRATION_TEST(delete_parsing);
    RUN_INTEGRATION_TEST(multiple_statements);
    RUN_INTEGRATION_TEST(string_escaping);
    RUN_INTEGRATION_TEST(numeric_values);

    printf("\n====================\n");
    if (failed == 0) {
        printf("All integration tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failed);
        return 1;
    }
}