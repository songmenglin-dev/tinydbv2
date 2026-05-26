#ifndef TINYDB_EXECUTOR_H
#define TINYDB_EXECUTOR_H

#include "../../include/tinydb.h"
#include "ast.h"
#include "expression.h"
#include <pthread.h>

/*============================================================================
 * ResultSet structure for query results
 *============================================================================*/
typedef struct ResultSet {
    Value** rows;            /* Array of row Value arrays */
    int row_count;           /* Number of rows */
    int column_count;       /* Number of columns per row */
    char** column_names;     /* Column names */
    ColumnType* column_types; /* Column types */
    int capacity;            /* Allocated capacity */
} ResultSet;

/*============================================================================
 * Transaction states
 *============================================================================*/
typedef enum {
    TXN_NONE = 0,
    TXN_ACTIVE,
    TXN_COMMITTED,
    TXN_ROLLED_BACK
} TxnState;

/*============================================================================
 * Executor structure
 *============================================================================*/
typedef struct Executor {
    Storage* storage;       /* Storage handle */
    int in_transaction;     /* Currently in a transaction */
    TxnState txn_state;      /* Transaction state */

    /* Statistics */
    uint64_t rows_read;
    uint64_t rows_written;
    uint64_t scans_performed;
} Executor;

/*============================================================================
 * Executor lifecycle
 *============================================================================*/

/* Create a new executor */
Executor* executor_create(Storage* storage);

/* Destroy executor and free resources */
void executor_destroy(Executor* exec);

/*============================================================================
 * Main execution function
 *============================================================================*/

/* Execute an AST statement and return results via callback or ResultSet */
int executor_exec(Executor* exec, AstNode* stmt, ResultCallback callback, void* data);

/* Shorthand for SELECT queries that returns a ResultSet */
ResultSet* executor_select(Executor* exec, AstSelect* select);

/*============================================================================
 * Statement handlers
 *============================================================================*/

/* CREATE TABLE */
int executor_exec_create_table(Executor* exec, AstCreateTable* stmt);

/* DROP TABLE */
int executor_exec_drop_table(Executor* exec, AstDropTable* stmt);

/* CREATE INDEX */
int executor_exec_create_index(Executor* exec, AstCreateIndex* stmt);

/* DROP INDEX */
int executor_exec_drop_index(Executor* exec, AstDropIndex* stmt);

/* INSERT */
int executor_exec_insert(Executor* exec, AstInsert* stmt);

/* UPDATE */
int executor_exec_update(Executor* exec, AstUpdate* stmt);

/* DELETE */
int executor_exec_delete(Executor* exec, AstDelete* stmt);

/* SELECT */
int executor_exec_select(Executor* exec, AstSelect* stmt, ResultCallback callback, void* data);

/* SHOW TABLES */
int executor_exec_show_tables(Executor* exec, AstShowTables* stmt, ResultCallback callback, void* data);

/* DESCRIBE TABLE */
int executor_exec_describe_table(Executor* exec, AstDescribeTable* stmt, ResultCallback callback, void* data);

/*============================================================================
 * SELECT result structure (passed via data for server communication)
 *============================================================================*/
typedef struct SelectResult {
    char*   result_buf;   /* Formatted ROW: lines */
    size_t  result_len;   /* Length of result_buf */
    int64_t row_count;    /* Number of rows returned (SELECT) or rows affected (INSERT/UPDATE/DELETE) */
} SelectResult;

/* INSERT */
int executor_exec_insert(Executor* exec, AstInsert* stmt);

/* BEGIN */
int executor_exec_begin(Executor* exec, AstTransaction* stmt);

/* COMMIT */
int executor_exec_commit(Executor* exec, AstTransaction* stmt);

/* ROLLBACK */
int executor_exec_rollback(Executor* exec, AstTransaction* stmt);

/* Evaluate a subquery expression */
Value* executor_evaluate_subquery(Executor* exec, AstNode* query);

/*============================================================================
 * ResultSet management
 *============================================================================*/

/* Create an empty result set */
ResultSet* result_set_create(int initial_capacity);

/* Add a row to the result set */
int result_set_add_row(ResultSet* rs, Value* row);

/* Free a result set */
void result_set_free(ResultSet* rs);

/*============================================================================
 * SELECT processing helpers
 *============================================================================*/

/* Initialize a SELECT query - returns row data array */
Value* select_init_query(Executor* exec, AstSelect* select, char*** column_names, ColumnType** column_types, int* row_capacity);

/* Apply WHERE clause filtering */
int select_apply_where(Value* row_data, Expression* where, int column_count, const char** column_names);

/* Apply DISTINCT */
void select_apply_distinct(ResultSet* rs);

/* Apply ORDER BY */
void select_apply_order_by(ResultSet* rs, OrderByItem* order_by, char** column_names, ColumnType* column_types);

/* Apply LIMIT */
void select_apply_limit(ResultSet* rs, Expression* limit, Expression* offset);

/* Project columns (SELECT specific columns or *) */
Value* select_project_columns(Value* row_data, int src_column_count, ColumnList* columns, const char** src_column_names, char*** out_column_names, ColumnType** out_column_types, int* out_column_count);

#endif /* TINYDB_EXECUTOR_H */