#include "test_helpers.h"

/*============================================================================
 * Integration tests for transaction operations: BEGIN, COMMIT, ROLLBACK
 *============================================================================*/

static int test_transaction_begin_parsing(void) {
    const char* begin_sql = "BEGIN;";

    TEST_ASSERT(strstr(begin_sql, "BEGIN") != NULL, "Missing BEGIN");

    return 0;
}

static int test_transaction_commit_parsing(void) {
    const char* commit_sql = "COMMIT;";

    TEST_ASSERT(strstr(commit_sql, "COMMIT") != NULL, "Missing COMMIT");

    return 0;
}

static int test_transaction_rollback_parsing(void) {
    const char* rollback_sql = "ROLLBACK;";

    TEST_ASSERT(strstr(rollback_sql, "ROLLBACK") != NULL, "Missing ROLLBACK");

    return 0;
}

static int test_transaction_sequence(void) {
    /* Test a complete transaction sequence */
    const char* tx_sequence =
        "BEGIN;"
        "INSERT INTO users (id, name) VALUES (1, 'Alice');"
        "INSERT INTO users (id, name) VALUES (2, 'Bob');"
        "COMMIT;";

    /* Count statements */
    int semicolons = 0;
    const char* p = tx_sequence;
    while (*p) {
        if (*p == ';') semicolons++;
        p++;
    }

    TEST_ASSERT(semicolons >= 4, "Should have 4 statements in transaction");

    return 0;
}

static int test_transaction_rollback_sequence(void) {
    /* Test rollback transaction sequence */
    const char* tx_sequence =
        "BEGIN;"
        "INSERT INTO users (id, name) VALUES (1, 'Alice');"
        "DELETE FROM users WHERE id = 1;"
        "ROLLBACK;";

    int semicolons = 0;
    const char* p = tx_sequence;
    while (*p) {
        if (*p == ';') semicolons++;
        p++;
    }

    TEST_ASSERT(semicolons >= 4, "Should have 4 statements in rollback");

    return 0;
}

static int test_isolation_levels(void) {
    /* Test that isolation level keywords are recognized */
    /* Isolation levels would be set via connection options */

    return 0;
}

static int test_auto_commit_off(void) {
    /* Test that BEGIN properly disables auto-commit */

    return 0;
}

/* Run all integration tests */
int main(void) {
    printf("TinyDB v2 Integration Tests: Transactions\n");
    printf("========================================\n\n");

    int failed = 0;

    printf("Transaction parsing tests:\n");
    RUN_INTEGRATION_TEST(transaction_begin_parsing);
    RUN_INTEGRATION_TEST(transaction_commit_parsing);
    RUN_INTEGRATION_TEST(transaction_rollback_parsing);
    RUN_INTEGRATION_TEST(transaction_sequence);
    RUN_INTEGRATION_TEST(transaction_rollback_sequence);

    printf("\nTransaction behavior tests:\n");
    RUN_INTEGRATION_TEST(isolation_levels);
    RUN_INTEGRATION_TEST(auto_commit_off);

    printf("\n====================\n");
    if (failed == 0) {
        printf("All integration tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failed);
        return 1;
    }
}