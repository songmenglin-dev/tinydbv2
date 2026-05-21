#include "../../src/sql/executor.h"
#include "../../src/sql/expression.h"
#include "../../src/sql/ast.h"
#include "mini_test.h"
#include <stdlib.h>
#include <string.h>

static void* test_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        printf("FAIL: Out of memory\n");
        exit(1);
    }
    return ptr;
}

test(test_result_set_create) {
    ResultSet* rs = result_set_create(16);
    assert_non_null(rs);
    assert_eq(rs->row_count, 0);
    assert_eq(rs->column_count, 0);
    assert_eq(rs->capacity, 16);
    result_set_free(rs);
    printf("  test_result_set_create passed\n");
}

test(test_result_set_add_row) {
    ResultSet* rs = result_set_create(2);

    Value* row1 = test_malloc(sizeof(Value) * 2);
    row1[0].type = VALUE_INTEGER;
    row1[0].as_int = 1;
    row1[1].type = VALUE_INTEGER;
    row1[1].as_int = 100;

    Value* row2 = test_malloc(sizeof(Value) * 2);
    row2[0].type = VALUE_INTEGER;
    row2[0].as_int = 2;
    row2[1].type = VALUE_INTEGER;
    row2[1].as_int = 200;

    assert_eq(result_set_add_row(rs, row1), 0);
    assert_eq(rs->row_count, 1);

    assert_eq(result_set_add_row(rs, row2), 0);
    assert_eq(rs->row_count, 2);

    result_set_free(rs);
    printf("  test_result_set_add_row passed\n");
}

test(test_executor_create) {
    Executor* exec = executor_create(NULL);
    assert_non_null(exec);
    assert_eq(exec->in_transaction, 0);
    assert_eq(exec->txn_state, TXN_NONE);
    executor_destroy(exec);
    printf("  test_executor_create passed\n");
}

test(test_select_apply_where_no_filter) {
    Value row_data[2] = {
        { .type = VALUE_INTEGER, .as_int = 5 },
        { .type = VALUE_INTEGER, .as_int = 10 }
    };

    int result = select_apply_where(row_data, NULL, 2, NULL);
    assert_eq(result, 1);
    printf("  test_select_apply_where_no_filter passed\n");
}

int main(void) {
    printf("Executor Tests (simplified)\n");
    printf("===============\n");

    run(test_result_set_create);
    run(test_result_set_add_row);
    run(test_executor_create);
    run(test_select_apply_where_no_filter);

    printf("\nAll tests passed!\n");
    return 0;
}