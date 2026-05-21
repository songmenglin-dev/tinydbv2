#define _POSIX_C_SOURCE 200809L

#include "executor.h"
#include "expression.h"
#include "../util/error.h"
#include "../../include/tinydb.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*============================================================================
 * Memory allocation helper
 *============================================================================*/
static void* xmalloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return ptr;
}

/*============================================================================
 * Executor lifecycle
 *============================================================================*/
Executor* executor_create(Storage* storage) {
    Executor* exec = xmalloc(sizeof(Executor));
    memset(exec, 0, sizeof(Executor));
    exec->storage = storage;
    exec->in_transaction = 0;
    exec->txn_state = TXN_NONE;
    exec->rows_read = 0;
    exec->rows_written = 0;
    exec->scans_performed = 0;
    return exec;
}

void executor_destroy(Executor* exec) {
    if (exec) {
        free(exec);
    }
}

/*============================================================================
 * ResultSet management
 *============================================================================*/
ResultSet* result_set_create(int initial_capacity) {
    ResultSet* rs = xmalloc(sizeof(ResultSet));
    memset(rs, 0, sizeof(ResultSet));
    rs->capacity = initial_capacity > 0 ? initial_capacity : 16;
    rs->rows = xmalloc(sizeof(Value*) * rs->capacity);
    rs->column_names = NULL;
    rs->column_types = NULL;
    return rs;
}

int result_set_add_row(ResultSet* rs, Value* row) {
    if (rs->row_count >= rs->capacity) {
        rs->capacity *= 2;
        rs->rows = realloc(rs->rows, sizeof(Value*) * rs->capacity);
        if (!rs->rows) return -1;
    }
    rs->rows[rs->row_count++] = row;
    return 0;
}

void result_set_free(ResultSet* rs) {
    if (!rs) return;
    for (int i = 0; i < rs->row_count; i++) {
        if (rs->rows[i]) {
            for (int j = 0; j < rs->column_count; j++) {
                value_free(&rs->rows[i][j]);
            }
            free(rs->rows[i]);
        }
    }
    free(rs->rows);
    free(rs->column_names);
    free(rs->column_types);
    free(rs);
}

/*============================================================================
 * Transaction dispatch (forward declaration)
 *============================================================================*/
static int executor_exec_transaction(Executor* exec, AstTransaction* stmt);

/*============================================================================
 * Main execution dispatcher
 *============================================================================*/
int executor_exec(Executor* exec, AstNode* stmt, ResultCallback callback, void* data) {
    if (!stmt) return ERR_INTERNAL;

    switch (stmt->type) {
        case AST_CREATE_TABLE:
            return executor_exec_create_table(exec, AST_CAST(AstCreateTable, stmt));
        case AST_DROP_TABLE:
            return executor_exec_drop_table(exec, AST_CAST(AstDropTable, stmt));
        case AST_CREATE_INDEX:
            return executor_exec_create_index(exec, AST_CAST(AstCreateIndex, stmt));
        case AST_DROP_INDEX:
            return executor_exec_drop_index(exec, AST_CAST(AstDropIndex, stmt));
        case AST_INSERT:
            return executor_exec_insert(exec, AST_CAST(AstInsert, stmt));
        case AST_UPDATE:
            return executor_exec_update(exec, AST_CAST(AstUpdate, stmt));
        case AST_DELETE:
            return executor_exec_delete(exec, AST_CAST(AstDelete, stmt));
        case AST_SELECT:
            return executor_exec_select(exec, AST_CAST(AstSelect, stmt), callback, data);
        case AST_TRANSACTION:
            return executor_exec_transaction(exec, AST_CAST(AstTransaction, stmt));
        default:
            return ERR_INTERNAL;
    }
}

int executor_exec_transaction(Executor* exec, AstTransaction* stmt) {
    if (!exec || !stmt) return ERR_INTERNAL;

    switch (stmt->transaction_type) {
        case TX_BEGIN:
            return executor_exec_begin(exec, stmt);
        case TX_COMMIT:
            return executor_exec_commit(exec, stmt);
        case TX_ROLLBACK:
            return executor_exec_rollback(exec, stmt);
        default:
            return ERR_INTERNAL;
    }
}

/*============================================================================
 * Transaction execution
 *============================================================================*/
int executor_exec_begin(Executor* exec, AstTransaction* stmt) {
    (void)stmt;
    if (exec->in_transaction) {
        return ERR_TX_ALREADY_ACTIVE;
    }
    int result = tinydb_begin(exec->storage);
    if (result == SUCCESS) {
        exec->in_transaction = 1;
        exec->txn_state = TXN_ACTIVE;
    }
    return result;
}

int executor_exec_commit(Executor* exec, AstTransaction* stmt) {
    (void)stmt;
    if (!exec->in_transaction) {
        return ERR_TX_NOT_ACTIVE;
    }
    int result = tinydb_commit(exec->storage);
    if (result == SUCCESS) {
        exec->in_transaction = 0;
        exec->txn_state = TXN_COMMITTED;
    }
    return result;
}

int executor_exec_rollback(Executor* exec, AstTransaction* stmt) {
    (void)stmt;
    if (!exec->in_transaction) {
        return ERR_TX_NOT_ACTIVE;
    }
    int result = tinydb_rollback(exec->storage);
    if (result == SUCCESS) {
        exec->in_transaction = 0;
        exec->txn_state = TXN_ROLLED_BACK;
    }
    return result;
}

/*============================================================================
 * Schema execution stubs
 *============================================================================*/
int executor_exec_create_table(Executor* exec, AstCreateTable* stmt) {
    (void)exec;
    (void)stmt;
    /* TODO: Implement using storage layer */
    return SUCCESS;
}

int executor_exec_drop_table(Executor* exec, AstDropTable* stmt) {
    (void)exec;
    (void)stmt;
    /* TODO: Implement using storage layer */
    return SUCCESS;
}

int executor_exec_create_index(Executor* exec, AstCreateIndex* stmt) {
    (void)exec;
    (void)stmt;
    /* TODO: Implement using storage layer */
    return SUCCESS;
}

int executor_exec_drop_index(Executor* exec, AstDropIndex* stmt) {
    (void)exec;
    (void)stmt;
    /* TODO: Implement using storage layer */
    return SUCCESS;
}

/*============================================================================
 * DML execution stubs
 *============================================================================*/
int executor_exec_insert(Executor* exec, AstInsert* stmt) {
    (void)exec;
    (void)stmt;
    /* TODO: Implement using storage layer */
    return SUCCESS;
}

int executor_exec_update(Executor* exec, AstUpdate* stmt) {
    (void)exec;
    (void)stmt;
    /* TODO: Implement using storage layer */
    return SUCCESS;
}

int executor_exec_delete(Executor* exec, AstDelete* stmt) {
    (void)exec;
    (void)stmt;
    /* TODO: Implement using storage layer */
    return SUCCESS;
}

/*============================================================================
 * SELECT processing
 *============================================================================*/

/* Initialize a SELECT query - prepare row data array */
Value* select_init_query(Executor* exec, AstSelect* select, char*** column_names, ColumnType** column_types, int* row_capacity) {
    (void)exec;

    /* Determine column count */
    int column_count = 0;
    if (select->columns) {
        /* Count selected columns or * means all columns */
        ColumnList* col = select->columns;
        while (col) {
            column_count++;
            col = col->next;
        }
    } else {
        /* SELECT * - need all columns from table metadata */
        column_count = 16;  /* Default max */
    }

    *column_names = xmalloc(sizeof(char*) * column_count);
    *column_types = xmalloc(sizeof(ColumnType) * column_count);
    *row_capacity = column_count;

    /* Initialize column names from column list */
    if (select->columns) {
        ColumnList* col = select->columns;
        int i = 0;
        while (col && i < column_count) {
            if (col->name) {
                (*column_names)[i] = col->name;
            } else {
                (*column_names)[i] = "";
            }
            (*column_types)[i] = COL_TYPE_TEXT;  /* Default type */
            col = col->next;
            i++;
        }
    }

    Value* row_data = xmalloc(sizeof(Value) * column_count);
    memset(row_data, 0, sizeof(Value) * column_count);
    return row_data;
}

/* Apply WHERE clause filtering */
int select_apply_where(Value* row_data, Expression* where, int column_count, const char** column_names) {
    if (!where) return 1;  /* No WHERE clause - all rows pass */

    Value* result = expr_eval(where, row_data, column_count, column_names);
    int truthy = value_is_truthy(result);
    value_free(result);
    return truthy;
}

/* Apply DISTINCT - remove duplicate rows */
void select_apply_distinct(ResultSet* rs) {
    if (!rs || rs->row_count == 0) return;

    /* Simple O(n^2) distinct - compare each pair */
    int write_idx = 0;
    for (int read_idx = 0; read_idx < rs->row_count; read_idx++) {
        int is_duplicate = 0;
        for (int cmp_idx = 0; cmp_idx < write_idx; cmp_idx++) {
            int same = 1;
            for (int col_idx = 0; col_idx < rs->column_count; col_idx++) {
                if (value_compare(&rs->rows[read_idx][col_idx], &rs->rows[cmp_idx][col_idx]) != 0) {
                    same = 0;
                    break;
                }
            }
            if (same) {
                is_duplicate = 1;
                break;
            }
        }
        if (!is_duplicate) {
            if (write_idx != read_idx) {
                rs->rows[write_idx] = rs->rows[read_idx];
            }
            write_idx++;
        }
    }
    rs->row_count = write_idx;
}

/* Apply ORDER BY - sort result set */
void select_apply_order_by(ResultSet* rs, OrderByItem* order_by, char** column_names, ColumnType* column_types) {
    (void)rs;
    (void)order_by;
    (void)column_names;
    (void)column_types;
    /* TODO: Implement proper quicksort with ORDER BY */
}

/* Apply LIMIT and OFFSET */
void select_apply_limit(ResultSet* rs, Expression* limit, Expression* offset) {
    if (!rs || rs->row_count == 0) return;

    int offset_val = 0;
    int limit_val = rs->row_count;  /* Default: no limit */

    if (offset) {
        Value* offset_v = expr_eval(offset, NULL, 0, NULL);
        if (offset_v && offset_v->type == VALUE_INTEGER) {
            offset_val = (int)offset_v->as_int;
        }
        if (offset_v) value_free(offset_v);
    }

    if (limit) {
        Value* limit_v = expr_eval(limit, NULL, 0, NULL);
        if (limit_v && limit_v->type == VALUE_INTEGER) {
            limit_val = (int)limit_v->as_int;
        }
        if (limit_v) value_free(limit_v);
    }

    if (offset_val >= rs->row_count) {
        rs->row_count = 0;
        return;
    }

    if (offset_val + limit_val > rs->row_count) {
        limit_val = rs->row_count - offset_val;
    }

    /* Remove leading rows if offset > 0 */
    if (offset_val > 0) {
        for (int i = offset_val; i < rs->row_count; i++) {
            rs->rows[i - offset_val] = rs->rows[i];
        }
        rs->row_count -= offset_val;
    }

    /* Limit row count */
    if (limit_val < rs->row_count) {
        rs->row_count = limit_val;
    }
}

/* Project columns (SELECT specific columns or *) */
Value* select_project_columns(Value* row_data, int src_column_count, ColumnList* columns, const char** src_column_names, char*** out_column_names, ColumnType** out_column_types, int* out_column_count) {
    if (!columns) {
        /* SELECT * - return all columns */
        *out_column_names = xmalloc(sizeof(char*) * src_column_count);
        *out_column_types = xmalloc(sizeof(ColumnType) * src_column_count);
        for (int i = 0; i < src_column_count; i++) {
            (*out_column_names)[i] = (char*)src_column_names[i];
            (*out_column_types)[i] = COL_TYPE_TEXT;
        }
        *out_column_count = src_column_count;
        return row_data;
    }

    /* Count columns */
    int count = 0;
    ColumnList* col = columns;
    while (col) {
        count++;
        col = col->next;
    }

    *out_column_count = count;
    *out_column_names = xmalloc(sizeof(char*) * count);
    *out_column_types = xmalloc(sizeof(ColumnType) * count);

    Value* result = xmalloc(sizeof(Value) * count);
    memset(result, 0, sizeof(Value) * count);

    col = columns;
    int out_idx = 0;
    while (col) {
        /* Find column by name */
        int src_idx = -1;
        for (int i = 0; i < src_column_count; i++) {
            if (src_column_names[i] && strcmp(col->name, src_column_names[i]) == 0) {
                src_idx = i;
                break;
            }
        }

        if (src_idx >= 0) {
            (*out_column_names)[out_idx] = (char*)src_column_names[src_idx];
            (*out_column_types)[out_idx] = COL_TYPE_TEXT;
            result[out_idx] = row_data[src_idx];
        } else {
            /* Column not found - return NULL */
            result[out_idx].type = VALUE_NULL;
        }

        col = col->next;
        out_idx++;
    }

    return result;
}

/* Execute a SELECT statement */
int executor_exec_select(Executor* exec, AstSelect* stmt, ResultCallback callback, void* data) {
    (void)exec;
    (void)stmt;
    (void)callback;
    (void)data;

    /* TODO: Implement full SELECT with storage layer scan */
    /* For now, return empty result - will be implemented with storage integration */

    return SUCCESS;
}

ResultSet* executor_select(Executor* exec, AstSelect* select) {
    (void)exec;
    (void)select;
    /* TODO: Implement SELECT returning ResultSet */
    return result_set_create(16);
}