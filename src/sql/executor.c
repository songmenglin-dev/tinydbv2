#define _POSIX_C_SOURCE 200809L

#include "executor.h"
#include "expression.h"
#include "storage.h"
#include "catalog.h"
#include "../storage/pager.h"
#include "../storage/page_cache.h"
#include "../storage/btree.h"
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
        size_t new_capacity = rs->capacity * 2;
        void* new_rows = realloc(rs->rows, sizeof(Value*) * new_capacity);
        if (!new_rows) return -1;
        rs->rows = new_rows;
        rs->capacity = new_capacity;
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
 * Schema execution
 *============================================================================*/
int executor_exec_create_table(Executor* exec, AstCreateTable* stmt) {
    if (!exec || !stmt || !stmt->table_name) return ERR_INTERNAL;
    if (!exec->storage) return ERR_INTERNAL;

    /* Get storage components */
    Catalog* catalog = storage_get_catalog(exec->storage);
    Pager* pager = storage_get_pager(exec->storage);
    PageCache* cache = storage_get_cache(exec->storage);
    if (!catalog || !pager || !cache) return ERR_INTERNAL;

    /* Build SQL string for catalog */
    char sql[512];
    int offset = snprintf(sql, sizeof(sql), "CREATE TABLE %s (", stmt->table_name);

    ColumnDef* col = stmt->columns;
    int first = 1;
    while (col && offset < (int)sizeof(sql) - 50) {
        if (!first) {
            offset += snprintf(sql + offset, sizeof(sql) - offset, ", ");
        }
        first = 0;
        offset += snprintf(sql + offset, sizeof(sql) - offset, "%s ", col->name);
        switch (col->type) {
            case COL_TYPE_INTEGER: offset += snprintf(sql + offset, sizeof(sql) - offset, "INT"); break;
            case COL_TYPE_FLOAT: offset += snprintf(sql + offset, sizeof(sql) - offset, "FLOAT"); break;
            case COL_TYPE_TEXT: offset += snprintf(sql + offset, sizeof(sql) - offset, "TEXT"); break;
            case COL_TYPE_BLOB: offset += snprintf(sql + offset, sizeof(sql) - offset, "BLOB"); break;
            default: offset += snprintf(sql + offset, sizeof(sql) - offset, "TEXT"); break;
        }
        if (col->not_null) offset += snprintf(sql + offset, sizeof(sql) - offset, " NOT NULL");
        if (col->primary_key) offset += snprintf(sql + offset, sizeof(sql) - offset, " PRIMARY KEY");
        col = col->next;
    }
    if (offset < (int)sizeof(sql) - 2) {
        offset += snprintf(sql + offset, sizeof(sql) - offset, ")");
    }

    /* Create B+tree for table data */
    BTree* table_tree = btree_create(pager, cache);
    if (!table_tree) return ERR_STORAGE_IO;
    uint32_t root_page = table_tree->root_page;  /* Get root page from created tree */

    /* Create catalog entry */
    CatalogEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.type = CATALOG_TYPE_TABLE;
    strncpy(entry.name, stmt->table_name, 63);
    strncpy(entry.tbl_name, stmt->table_name, 63);
    strncpy(entry.sql, sql, 511);
    entry.root_page = root_page;
    entry.is_valid = 1;

    /* Insert into catalog */
    int ret = catalog_insert(catalog, &entry);
    btree_close(table_tree);
    return ret;
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
 * DML execution
 *============================================================================*/
int executor_exec_insert(Executor* exec, AstInsert* stmt) {
    if (!exec || !stmt || !stmt->table_name) return ERR_INTERNAL;
    if (!exec->storage) return ERR_INTERNAL;

    /* Get storage components */
    Catalog* catalog = storage_get_catalog(exec->storage);
    Pager* pager = storage_get_pager(exec->storage);
    PageCache* cache = storage_get_cache(exec->storage);
    if (!catalog || !pager || !cache) {
        return ERR_INTERNAL;
    }

    /* Lookup table in catalog */
    CatalogEntry* table_entry = catalog_lookup_type_name(catalog, CATALOG_TYPE_TABLE, stmt->table_name);
    if (!table_entry) {
        return ERR_EXEC_TABLE_NOT_FOUND;
    }

    /* Get table's B+tree */
    BTree* table_tree = NULL;
    if (table_entry->root_page > 0) {
        table_tree = btree_open(pager, cache, table_entry->root_page);
    }

    if (!table_tree) {
        free(table_entry);
        return ERR_STORAGE_IO;
    }

    /* Serialize row data: interleave key (rowid) and value (column data) */
    /* For simplicity, use auto-incrementing rowid */
    static _Atomic uint64_t rowid_counter = 0;
    uint64_t rowid = ++rowid_counter;

    /* Serialize values into buffer */
    char value_buf[1024];
    int offset = 0;

    ValueList* vl = stmt->values;
    while (vl) {
        for (int i = 0; i < vl->count && i < 16; i++) {
            Expression* expr = vl->values[i];
            if (expr->type == EXPR_LITERAL_INT) {
                *(int64_t*)(value_buf + offset) = expr->as_int;
                offset += sizeof(int64_t);
            } else if (expr->type == EXPR_LITERAL_FLOAT) {
                *(double*)(value_buf + offset) = expr->as_float;
                offset += sizeof(double);
            } else if (expr->type == EXPR_LITERAL_STRING && expr->as_string.str) {
                size_t slen = strlen(expr->as_string.str);
                size_t remaining = sizeof(value_buf) - offset;
                if (slen >= remaining) slen = remaining - 1;
                memcpy(value_buf + offset, expr->as_string.str, slen);
                offset += slen;
                value_buf[offset] = '\0';
                offset++;
            } else {
                /* NULL or unknown type */
                value_buf[offset++] = 0;
            }
        }
        vl = vl->next;
    }

    /* Insert into table's B+tree */
    int ret = btree_insert(table_tree, rowid, value_buf, offset);
    if (ret != SUCCESS) {
        btree_close(table_tree);
        free(table_entry);
        return ret;
    }
    btree_close(table_tree);
    free(table_entry);
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
    if (!exec || !stmt) return ERR_INTERNAL;
    if (!exec->storage) return ERR_INTERNAL;

    /* Handle scalar SELECT (no table) */
    if (!stmt->table_name) {
        /* Scalar SELECT like SELECT 1 - just return success with 1 row */
        (void)callback;
        (void)data;
        return SUCCESS;
    }

    /* Get storage components */
    Catalog* catalog = storage_get_catalog(exec->storage);
    Pager* pager = storage_get_pager(exec->storage);
    PageCache* cache = storage_get_cache(exec->storage);
    if (!catalog || !pager || !cache) return ERR_INTERNAL;

    /* Lookup table in catalog */
    CatalogEntry* table_entry = catalog_lookup_type_name(catalog, CATALOG_TYPE_TABLE, stmt->table_name);
    if (!table_entry) {
        return ERR_EXEC_TABLE_NOT_FOUND;
    }

    /* Get table's B+tree - open it if we have a root page */
    BTree* table_tree = NULL;
    if (table_entry->root_page > 0) {
        table_tree = btree_open(pager, cache, table_entry->root_page);
    }

    if (!table_tree) {
        free(table_entry);
        /* Empty table - return success with no rows */
        return SUCCESS;
    }

    /* Scan all rows from the table's B+tree and collect data */
    BTreeCursor* cursor = btree_first(table_tree);
    int row_count = 0;

    /* Buffer to accumulate all row data for server to send */
    /* Format: "ROW:col1\tcol2\tcol3\n" per row */
    char* result_buf = NULL;
    size_t result_len = 0;
    size_t result_capacity = 0;

    while (cursor && btree_cursor_valid(cursor)) {
        uint64_t key;
        uint32_t len;
        char row_buf[2048];

        int ret = btree_get(cursor, &key, row_buf, &len);
        if (ret == SUCCESS && len > 0) {
            row_count++;

            /* Deserialize row data and format as tab-separated string */
            /* Format: key (rowid) followed by column values */
            char line_buf[4096];
            int offset = 0;
            int col_idx = 0;

            /* Read rowid first */
            if (len >= sizeof(uint64_t)) {
                uint64_t rowid = *(uint64_t*)row_buf;
                offset += sizeof(uint64_t);

                /* Format: rowid as first column */
                size_t remaining = sizeof(line_buf) - offset;
                offset += snprintf(line_buf + offset, remaining, "%llu", (unsigned long long)rowid);

                /* Try to read column values - assume at least some data */
                while (offset < (int)len && col_idx < 16) {
                    /* Try to determine type and read value */
                    remaining = sizeof(line_buf) - offset;
                    if (offset + sizeof(int64_t) <= (size_t)len) {
                        int64_t int_val = *(int64_t*)(row_buf + offset);
                        offset += sizeof(int64_t);
                        offset += snprintf(line_buf + offset, remaining, "\t%lld", (long long)int_val);
                        col_idx++;
                    } else if (offset + sizeof(double) <= (size_t)len) {
                        double float_val = *(double*)(row_buf + offset);
                        offset += sizeof(double);
                        offset += snprintf(line_buf + offset, remaining, "\t%g", float_val);
                        col_idx++;
                    } else {
                        /* Try to read as null-terminated string */
                        char* str_val = row_buf + offset;
                        size_t str_len = strlen(str_val);
                        if (str_len > 0 && offset + str_len + 1 <= (size_t)len) {
                            offset += str_len + 1;
                            offset += snprintf(line_buf + offset, remaining, "\t%s", str_val);
                            col_idx++;
                        } else {
                            break;
                        }
                    }
                }
            }

            /* Append ROW: line to result buffer - preallocate to avoid O(n^2) */
            size_t line_len = strlen(line_buf);
            size_t needed = result_len + line_len + 16;
            if (result_capacity < needed) {
                size_t new_cap = result_capacity == 0 ? 4096 : result_capacity * 2;
                if (new_cap < needed) new_cap = needed;
                result_buf = realloc(result_buf, new_cap);
                result_capacity = new_cap;
            }
            result_len += snprintf(result_buf + result_len, result_capacity - result_len, "ROW:%s\n", line_buf);
        }
        btree_cursor_next(cursor);
    }

    if (cursor) btree_cursor_free(cursor);
    btree_close(table_tree);
    free(table_entry);

    /* Store result buffer in data for server to retrieve */
    if (data && result_buf) {
        /* data is actually char** - the pointer to select_result_buf */
        char** result_ptr = (char**)data;
        *result_ptr = result_buf;
    } else if (result_buf) {
        free(result_buf);
    }

    /* Note: callback would be called with each row for full implementation */
    return SUCCESS;
}

ResultSet* executor_select(Executor* exec, AstSelect* select) {
    (void)exec;
    (void)select;
    /* TODO: Implement SELECT returning ResultSet */
    return result_set_create(16);
}