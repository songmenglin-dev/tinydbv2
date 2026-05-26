#define _POSIX_C_SOURCE 200809L

#include "../../src/sql/schema.h"
#include "mini_test.h"
#include <stdlib.h>
#include <string.h>

/* Test schema creation */
test(schema_creation) {
    Column cols[3];
    memset(cols, 0, sizeof(cols));

    strcpy(cols[0].name, "id");
    cols[0].type = COL_TYPE_INTEGER;
    cols[0].primary_key = 1;
    cols[0].auto_increment = 1;

    strcpy(cols[1].name, "name");
    cols[1].type = COL_TYPE_TEXT;
    cols[1].not_null = 1;

    strcpy(cols[2].name, "age");
    cols[2].type = COL_TYPE_INTEGER;
    cols[2].not_null = 0;

    Schema* schema = schema_create("users", cols, 3);
    assert_non_null(schema);

    assert_eq_int(schema->col_count, 3);
    assert_str_eq(schema->table_name, "users");
    assert_true(schema->is_open);

    schema_destroy(schema);
}

/* Test schema column index lookup */
test(schema_column_index) {
    Column cols[4];
    memset(cols, 0, sizeof(cols));

    strcpy(cols[0].name, "id");
    cols[0].type = COL_TYPE_INTEGER;
    strcpy(cols[1].name, "name");
    cols[1].type = COL_TYPE_TEXT;
    strcpy(cols[2].name, "email");
    cols[2].type = COL_TYPE_TEXT;
    strcpy(cols[3].name, "age");
    cols[3].type = COL_TYPE_INTEGER;

    Schema* schema = schema_create("test_table", cols, 4);
    assert_non_null(schema);

    assert_eq_int(schema_column_index(schema, "id"), 0);
    assert_eq_int(schema_column_index(schema, "name"), 1);
    assert_eq_int(schema_column_index(schema, "email"), 2);
    assert_eq_int(schema_column_index(schema, "age"), 3);
    assert_eq_int(schema_column_index(schema, "nonexistent"), -1);

    schema_destroy(schema);
}

/* Test schema column retrieval */
test(schema_column) {
    Column cols[2];
    memset(cols, 0, sizeof(cols));

    strcpy(cols[0].name, "first");
    cols[0].type = COL_TYPE_TEXT;
    strcpy(cols[1].name, "second");
    cols[1].type = COL_TYPE_INTEGER;

    Schema* schema = schema_create("my_table", cols, 2);
    assert_non_null(schema);

    Column* col = schema_column(schema, "first");
    assert_non_null(col);
    assert_eq_int(col->type, COL_TYPE_TEXT);

    col = schema_column(schema, "second");
    assert_non_null(col);
    assert_eq_int(col->type, COL_TYPE_INTEGER);

    col = schema_column(schema, "missing");
    assert_null(col);

    schema_destroy(schema);
}

/* Test schema serialization */
test(schema_serialization) {
    Column cols[2];
    memset(cols, 0, sizeof(cols));

    strcpy(cols[0].name, "id");
    cols[0].type = COL_TYPE_INTEGER;
    cols[0].primary_key = 1;
    strcpy(cols[1].name, "data");
    cols[1].type = COL_TYPE_TEXT;
    cols[1].not_null = 1;

    Schema* schema = schema_create("serial_test", cols, 2);
    assert_non_null(schema);

    /* Serialize */
    uint8_t buf[SCHEMA_MAX_SIZE];
    uint32_t size = 0;

    int ret = schema_serialize(schema, buf, &size);
    assert_eq_int(ret, SUCCESS);
    assert_true(size > 0);
    assert_true(size < SCHEMA_MAX_SIZE);

    /* Deserialize */
    Schema* restored = schema_deserialize(buf, size);
    assert_non_null(restored);
    assert_eq_int(restored->col_count, 2);
    assert_str_eq(restored->table_name, "serial_test");

    /* Verify column details */
    assert_str_eq(restored->columns[0].name, "id");
    assert_eq_int(restored->columns[0].type, COL_TYPE_INTEGER);
    assert_str_eq(restored->columns[1].name, "data");
    assert_eq_int(restored->columns[1].type, COL_TYPE_TEXT);

    schema_destroy(schema);
    schema_destroy(restored);
}

/* Test schema clone */
test(schema_clone) {
    Column cols[1];
    memset(cols, 0, sizeof(cols));
    strcpy(cols[0].name, "original");
    cols[0].type = COL_TYPE_BLOB;

    Schema* original = schema_create("to_clone", cols, 1);
    assert_non_null(original);

    Schema* clone = schema_clone(original);
    assert_non_null(clone);
    assert_str_eq(clone->table_name, original->table_name);
    assert_eq_int(clone->col_count, original->col_count);
    assert_eq_int(clone->columns[0].type, original->columns[0].type);

    schema_destroy(original);
    schema_destroy(clone);
}

/* Test schema equality */
test(schema_equality) {
    Column cols1[2];
    memset(cols1, 0, sizeof(cols1));
    strcpy(cols1[0].name, "a");
    cols1[0].type = COL_TYPE_INTEGER;
    strcpy(cols1[1].name, "b");
    cols1[1].type = COL_TYPE_TEXT;

    Column cols2[2];
    memset(cols2, 0, sizeof(cols2));
    strcpy(cols2[0].name, "a");
    cols2[0].type = COL_TYPE_INTEGER;
    strcpy(cols2[1].name, "b");
    cols2[1].type = COL_TYPE_TEXT;

    Schema* s1 = schema_create("same", cols1, 2);
    Schema* s2 = schema_create("same", cols2, 2);

    assert_true(schema_equal(s1, s2));

    /* Different table names should fail */
    Schema* s3 = schema_create("different", cols1, 2);
    assert_false(schema_equal(s1, s3));

    schema_destroy(s1);
    schema_destroy(s2);
    schema_destroy(s3);
}

/* Helper: build binary row buffer matching executor format:
 * [type(1) + len(4) + data] per column
 * type: 0=int64, 1=double, 2=text, 3=null */
static int build_row_buf(uint8_t* buf, int* cols_info, int col_count) {
    int offset = 0;
    for (int i = 0; i < col_count; i++) {
        int type = cols_info[i * 2];
        int len = cols_info[i * 2 + 1];

        buf[offset++] = (uint8_t)type;
        *(uint32_t*)(buf + offset) = (uint32_t)len;
        offset += 4;

        if (type == 0) { /* INTEGER */
            *(int64_t*)(buf + offset) = (int64_t)len;
            offset += sizeof(int64_t);
        } else if (type == 1) { /* FLOAT */
            *(double*)(buf + offset) = (double)len;
            offset += sizeof(double);
        } else if (type == 2) { /* TEXT */
            /* len is actually the string length; data must be set by caller */
            offset += len;
        } else if (type == 3) { /* NULL: len=0, no data */
            /* nothing to add */
        }
    }
    return offset;
}

/* Build a simple 2-column row: (INTEGER, INTEGER) */
static int build_simple_row(uint8_t* buf, int64_t v0, int64_t v1) {
    int offset = 0;
    buf[offset++] = 0; /* INTEGER */
    *(uint32_t*)(buf + offset) = sizeof(int64_t);
    offset += 4;
    *(int64_t*)(buf + offset) = v0;
    offset += sizeof(int64_t);

    buf[offset++] = 0; /* INTEGER */
    *(uint32_t*)(buf + offset) = sizeof(int64_t);
    offset += 4;
    *(int64_t*)(buf + offset) = v1;
    offset += sizeof(int64_t);

    return offset;
}

/* Test row validation with valid row */
test(schema_validate_row_valid) {
    Column cols[2];
    memset(cols, 0, sizeof(cols));
    strcpy(cols[0].name, "a");
    cols[0].type = COL_TYPE_INTEGER;
    strcpy(cols[1].name, "b");
    cols[1].type = COL_TYPE_INTEGER;

    Schema* schema = schema_create("t", cols, 2);
    assert_non_null(schema);

    uint8_t buf[64];
    int len = build_simple_row(buf, 10, 20);
    int ret = schema_validate_row(schema, buf, (uint32_t)len);
    assert_eq_int(ret, SUCCESS);

    schema_destroy(schema);
}

/* Test row validation with NULL in NOT NULL column */
test(schema_validate_row_not_null_violation) {
    Column cols[2];
    memset(cols, 0, sizeof(cols));
    strcpy(cols[0].name, "id");
    cols[0].type = COL_TYPE_INTEGER;
    cols[0].not_null = 1;
    strcpy(cols[1].name, "name");
    cols[1].type = COL_TYPE_TEXT;

    Schema* schema = schema_create("t", cols, 2);
    assert_non_null(schema);

    /* Row: INTEGER (not null) + NULL (not null col) */
    uint8_t buf[64];
    int offset = 0;
    buf[offset++] = 0; /* INTEGER */
    *(uint32_t*)(buf + offset) = sizeof(int64_t);
    offset += 4;
    *(int64_t*)(buf + offset) = 42;
    offset += sizeof(int64_t);

    buf[offset++] = 3; /* NULL */
    *(uint32_t*)(buf + offset) = 0;
    offset += 4;

    int ret = schema_validate_row(schema, buf, (uint32_t)offset);
    assert_eq_int(ret, ERR_EXEC_NOT_NULL_VIOLATION);

    schema_destroy(schema);
}

/* Test row validation with type mismatch */
test(schema_validate_row_type_mismatch) {
    Column cols[2];
    memset(cols, 0, sizeof(cols));
    strcpy(cols[0].name, "val");
    cols[0].type = COL_TYPE_FLOAT;  /* schema expects FLOAT */
    strcpy(cols[1].name, "extra");
    cols[1].type = COL_TYPE_INTEGER;

    Schema* schema = schema_create("t", cols, 2);
    assert_non_null(schema);

    /* Row: INTEGER (type=0) when FLOAT (type=1) expected */
    uint8_t buf[64];
    int offset = 0;
    buf[offset++] = 0; /* INTEGER */
    *(uint32_t*)(buf + offset) = sizeof(int64_t);
    offset += 4;
    *(int64_t*)(buf + offset) = 99;
    offset += sizeof(int64_t);

    buf[offset++] = 0; /* INTEGER */
    *(uint32_t*)(buf + offset) = sizeof(int64_t);
    offset += 4;
    *(int64_t*)(buf + offset) = 1;
    offset += sizeof(int64_t);

    int ret = schema_validate_row(schema, buf, (uint32_t)offset);
    assert_eq_int(ret, ERR_EXEC_TYPE_MISMATCH);

    schema_destroy(schema);
}

/* Test row validation with insufficient columns */
test(schema_validate_row_column_count_mismatch) {
    Column cols[3];
    memset(cols, 0, sizeof(cols));
    strcpy(cols[0].name, "a");
    cols[0].type = COL_TYPE_INTEGER;
    strcpy(cols[1].name, "b");
    cols[1].type = COL_TYPE_INTEGER;
    strcpy(cols[2].name, "c");
    cols[2].type = COL_TYPE_INTEGER;

    Schema* schema = schema_create("t", cols, 3);
    assert_non_null(schema);

    /* Row has only 2 columns, schema expects 3 */
    uint8_t buf[64];
    int len = build_simple_row(buf, 1, 2);
    int ret = schema_validate_row(schema, buf, (uint32_t)len);
    assert_eq_int(ret, ERR_EXEC_TYPE_MISMATCH);

    schema_destroy(schema);
}

/* Test row validation with corrupt buffer (overflow) */
test(schema_validate_row_corrupt_buffer) {
    Column cols[2];
    memset(cols, 0, sizeof(cols));
    strcpy(cols[0].name, "a");
    cols[0].type = COL_TYPE_INTEGER;
    strcpy(cols[1].name, "b");
    cols[1].type = COL_TYPE_INTEGER;

    Schema* schema = schema_create("t", cols, 2);
    assert_non_null(schema);

    /* Truncated buffer: only 3 bytes of an INTEGER header */
    uint8_t buf[3];
    buf[0] = 0; /* INTEGER type */
    buf[1] = 8; /* len high byte */
    buf[2] = 0;

    int ret = schema_validate_row(schema, buf, 3);
    assert_eq_int(ret, ERR_STORAGE_CORRUPT);

    schema_destroy(schema);
}

/* Test row validation with NULL in nullable column (should pass) */
test(schema_validate_row_null_nullable) {
    Column cols[2];
    memset(cols, 0, sizeof(cols));
    strcpy(cols[0].name, "id");
    cols[0].type = COL_TYPE_INTEGER;
    cols[0].not_null = 0;  /* nullable */
    strcpy(cols[1].name, "data");
    cols[1].type = COL_TYPE_TEXT;
    cols[1].not_null = 0;

    Schema* schema = schema_create("t", cols, 2);
    assert_non_null(schema);

    /* Row: INTEGER(42) + NULL */
    uint8_t buf[64];
    int offset = 0;
    buf[offset++] = 0; /* INTEGER */
    *(uint32_t*)(buf + offset) = sizeof(int64_t);
    offset += 4;
    *(int64_t*)(buf + offset) = 42;
    offset += sizeof(int64_t);

    buf[offset++] = 3; /* NULL */
    *(uint32_t*)(buf + offset) = 0;
    offset += 4;

    int ret = schema_validate_row(schema, buf, (uint32_t)offset);
    assert_eq_int(ret, SUCCESS);

    schema_destroy(schema);
}

/* Run all tests */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("Running Schema tests...\n");

    run(schema_creation);
    run(schema_column_index);
    run(schema_column);
    run(schema_serialization);
    run(schema_clone);
    run(schema_equality);
    run(schema_validate_row_valid);
    run(schema_validate_row_not_null_violation);
    run(schema_validate_row_type_mismatch);
    run(schema_validate_row_column_count_mismatch);
    run(schema_validate_row_corrupt_buffer);
    run(schema_validate_row_null_nullable);

    printf("\nAll Schema tests passed!\n");
    return 0;
}