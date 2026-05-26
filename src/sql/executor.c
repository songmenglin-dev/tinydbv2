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
        case AST_SHOW_TABLES:
            return executor_exec_show_tables(exec, AST_CAST(AstShowTables, stmt), callback, data);
        case AST_DESCRIBE_TABLE:
            return executor_exec_describe_table(exec, AST_CAST(AstDescribeTable, stmt), callback, data);
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

    /* Build SQL string for catalog and count columns */
    int column_count = 0;
    ColumnDef* col_count = stmt->columns;
    while (col_count) {
        column_count++;
        col_count = col_count->next;
    }

    /* Allocate column info array */
    ColumnInfo* columns = NULL;
    if (column_count > 0) {
        columns = calloc(column_count, sizeof(ColumnInfo));
        if (!columns) return ERR_INTERNAL;
    }

    /* Build SQL string for catalog and populate column info */
    char sql[512];
    int offset = snprintf(sql, sizeof(sql), "CREATE TABLE %s (", stmt->table_name);

    ColumnDef* col = stmt->columns;
    int idx = 0;
    int first = 1;
    while (col && offset < (int)sizeof(sql) - 50) {
        if (!first) {
            offset += snprintf(sql + offset, sizeof(sql) - offset, ", ");
        }
        first = 0;
        offset += snprintf(sql + offset, sizeof(sql) - offset, "%s ", col->name);

        /* Populate column info */
        if (idx < column_count && columns) {
            strncpy(columns[idx].name, col->name, 63);
            columns[idx].type = col->type;
            columns[idx].not_null = col->not_null;
            columns[idx].primary_key = col->primary_key;
            columns[idx].autoincrement = col->autoincrement;
            /* Default value - serialize expression to string */
            if (col->default_value) {
                expr_to_sql_string(col->default_value,
                                   columns[idx].default_val,
                                   sizeof(columns[idx].default_val));
            } else {
                columns[idx].default_val[0] = '\0';
            }
        }

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
        idx++;
    }
    if (offset < (int)sizeof(sql) - 2) {
        offset += snprintf(sql + offset, sizeof(sql) - offset, ")");
    }

    /* Create B+tree for table data */
    BTree* table_tree = btree_create(pager, cache);
    if (!table_tree) {
        free(columns);
        return ERR_STORAGE_IO;
    }
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
    entry.columns = columns;
    entry.column_count = column_count;

    /* Insert into catalog */
    int ret = catalog_insert(catalog, &entry);
    btree_close(table_tree);

    /* Note: columns memory is owned by entry and will be freed when entry is freed */
    /* But if catalog_insert doesn't copy the columns, we need to free them */
    /* For now, columns is stack-allocated in entry, so no extra cleanup needed */

    return ret;
}

int executor_exec_drop_table(Executor* exec, AstDropTable* stmt) {
    if (!exec || !stmt || !stmt->table_name) return ERR_INTERNAL;
    if (!exec->storage) return ERR_INTERNAL;

    /* Get catalog */
    Catalog* catalog = storage_get_catalog(exec->storage);
    if (!catalog) return ERR_INTERNAL;

    /* Delete the table from catalog */
    int ret = catalog_delete(catalog, CATALOG_TYPE_TABLE, stmt->table_name);
    if (ret != SUCCESS) {
        return ERR_EXEC_TABLE_NOT_FOUND;
    }

    return SUCCESS;
}

int executor_exec_create_index(Executor* exec, AstCreateIndex* stmt) {
    if (!exec || !stmt || !stmt->index_name || !stmt->table_name || !stmt->column_name) {
        return ERR_INTERNAL;
    }
    if (!exec->storage) return ERR_INTERNAL;

    Catalog* catalog = storage_get_catalog(exec->storage);
    Pager* pager = storage_get_pager(exec->storage);
    PageCache* cache = storage_get_cache(exec->storage);
    if (!catalog || !pager || !cache) return ERR_INTERNAL;

    /* Build SQL string */
    char sql[512];
    snprintf(sql, sizeof(sql), "CREATE %sINDEX %s ON %s (%s)",
            stmt->unique ? "UNIQUE " : "",
            stmt->index_name, stmt->table_name, stmt->column_name);

    /* Create B+tree for index */
    BTree* index_tree = btree_create(pager, cache);
    if (!index_tree) return ERR_STORAGE_IO;
    uint32_t root_page = index_tree->root_page;

    /* Create catalog entry */
    CatalogEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.type = CATALOG_TYPE_INDEX;
    strncpy(entry.name, stmt->index_name, 63);
    strncpy(entry.tbl_name, stmt->table_name, 63);
    strncpy(entry.sql, sql, 511);
    entry.root_page = root_page;
    entry.is_valid = 1;
    entry.columns = NULL;
    entry.column_count = 0;

    int ret = catalog_insert(catalog, &entry);
    btree_close(index_tree);
    return ret;
}

int executor_exec_drop_index(Executor* exec, AstDropIndex* stmt) {
    if (!exec || !stmt || !stmt->index_name) return ERR_INTERNAL;
    if (!exec->storage) return ERR_INTERNAL;

    Catalog* catalog = storage_get_catalog(exec->storage);
    if (!catalog) return ERR_INTERNAL;

    return catalog_delete(catalog, CATALOG_TYPE_INDEX, stmt->index_name);
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

    /* Serialize row data: type_byte(1) + length(4) + data(variable) per column */
    /* type: 1=integer, 2=float, 3=string, 0=null */
    static _Atomic uint64_t rowid_counter = 0;
    char value_buf[1024];

    /* FIX: Each ValueList node is a separate row. Insert one row per ValueList.
     * Previously all ValueList nodes were merged into one entry, causing
     * INSERT INTO t VALUES (1),(2) to store a single row with 4 columns [1|2|1|2]. */
    int rows_inserted = 0;

    ValueList* vl = stmt->values;
    while (vl) {
        int offset = 0;
        for (int i = 0; i < vl->count && i < 16; i++) {
            Expression* expr = vl->values[i];
            if (expr->type == EXPR_LITERAL_INT) {
                value_buf[offset++] = 0;  /* type = integer (COL_TYPE_INTEGER) */
                *(uint32_t*)(value_buf + offset) = sizeof(int64_t);
                offset += 4;
                *(int64_t*)(value_buf + offset) = expr->as_int;
                offset += sizeof(int64_t);
            } else if (expr->type == EXPR_LITERAL_FLOAT) {
                value_buf[offset++] = 1;  /* type = float (COL_TYPE_FLOAT) */
                *(uint32_t*)(value_buf + offset) = sizeof(double);
                offset += 4;
                *(double*)(value_buf + offset) = expr->as_float;
                offset += sizeof(double);
            } else if (expr->type == EXPR_LITERAL_STRING && expr->as_string.str) {
                size_t slen = strlen(expr->as_string.str);
                size_t remaining = sizeof(value_buf) - offset - 5;
                if (slen > remaining) slen = remaining;
                value_buf[offset++] = 2;  /* type = text (COL_TYPE_TEXT) */
                *(uint32_t*)(value_buf + offset) = (uint32_t)slen;
                offset += 4;
                memcpy(value_buf + offset, expr->as_string.str, slen);
                offset += slen;
            } else {
                /* NULL or unknown type */
                value_buf[offset++] = 3;  /* type = null (COL_TYPE_BLOB as null marker) */
                *(uint32_t*)(value_buf + offset) = 0;
                offset += 4;
            }
        }

        /* Generate a unique rowid for this row and insert it */
        uint64_t rowid = ++rowid_counter;
        int ret = btree_insert(table_tree, rowid, value_buf, offset);
        if (ret != SUCCESS) {
            btree_close(table_tree);
            free(table_entry);
            return ret;
        }
        rows_inserted++;
        vl = vl->next;
    }

    btree_close(table_tree);
    free(table_entry);
    (void)rows_inserted;
    return SUCCESS;
}

int executor_exec_update(Executor* exec, AstUpdate* stmt) {
    if (!exec || !stmt || !stmt->table_name) return ERR_INTERNAL;
    if (!exec->storage) return ERR_INTERNAL;

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

    /* Get table's B+tree */
    BTree* table_tree = NULL;
    if (table_entry->root_page > 0) {
        table_tree = btree_open(pager, cache, table_entry->root_page);
    }

    if (!table_tree) {
        free(table_entry);
        return SUCCESS;  /* Empty table */
    }

    /* Build column names/indices mapping from table metadata */
    int column_count = table_entry->column_count > 0 ? table_entry->column_count : 0;
    char* column_names[16];
    for (int i = 0; i < column_count && i < 16; i++) {
        column_names[i] = table_entry->columns[i].name;
    }

    /* Build set clause column -> index map */
    int set_col_idx[16];
    memset(set_col_idx, -1, sizeof(set_col_idx));
    SetClause* sc = stmt->set_clauses;
    int set_count = 0;
    while (sc && set_count < 16) {
        for (int i = 0; i < column_count; i++) {
            if (strcmp(sc->column_name, column_names[i]) == 0) {
                set_col_idx[set_count] = i;
                break;
            }
        }
        set_count++;
        sc = sc->next;
    }

    /* Scan all rows and update those matching WHERE clause */
    int updated_count = 0;
    BTreeCursor* cursor = btree_first(table_tree);

    while (cursor && btree_cursor_valid(cursor)) {
        uint64_t key;
        uint32_t len;
        char row_buf[2048];

        int ret = btree_get(cursor, &key, row_buf, &len);
        if (ret == SUCCESS && len > 0) {
            /* Deserialize row data into Value array */
            Value row_data[16];
            memset(row_data, 0, sizeof(row_data));
            int col_idx = 0;
            int offset = 0;

            while (offset < (int)len && col_idx < 16) {
                if ((size_t)offset + 5 > len) break;
                uint8_t col_type = (uint8_t)row_buf[offset];
                uint32_t col_len = *(uint32_t*)(row_buf + offset + 1);

                if (col_type == 3 || col_len == 0) break;
                if ((size_t)offset + 5 + col_len > len) break;

                offset += 5;

                if (col_type == 0 && col_len == sizeof(int64_t)) {
                    row_data[col_idx].type = VALUE_INTEGER;
                    row_data[col_idx].as_int = *(int64_t*)(row_buf + offset);
                    offset += sizeof(int64_t);
                    col_idx++;
                } else if (col_type == 1 && col_len == sizeof(double)) {
                    row_data[col_idx].type = VALUE_FLOAT;
                    row_data[col_idx].as_float = *(double*)(row_buf + offset);
                    offset += sizeof(double);
                    col_idx++;
                } else if (col_type == 2) {
                    row_data[col_idx].type = VALUE_TEXT;
                    row_data[col_idx].as_text.str = NULL;
                    row_data[col_idx].as_text.len = 0;
                    offset += col_len;
                    col_idx++;
                } else {
                    row_data[col_idx].type = VALUE_NULL;
                    col_idx++;
                }
            }

            /* Evaluate WHERE clause */
            int should_update = 1;
            if (stmt->where) {
                Value* result = expr_eval(stmt->where, row_data, column_count, (const char**)column_names);
                should_update = value_is_truthy(result);
                value_free(result);
            }

            if (should_update) {
                /* Apply SET clauses */
                sc = stmt->set_clauses;
                int sc_idx = 0;
                while (sc && sc_idx < 16) {
                    int target = set_col_idx[sc_idx];
                    if (target >= 0) {
                        Value* new_val = expr_eval(sc->value, row_data, column_count, (const char**)column_names);
                        if (new_val) {
                            value_free(&row_data[target]);
                            row_data[target] = *new_val;
                            free(new_val);
                        }
                    }
                    sc_idx++;
                    sc = sc->next;
                }

                /* Re-serialize updated row data */
                char new_buf[2048];
                int new_offset = 0;
                for (int i = 0; i < col_idx && i < 16; i++) {
                    Value* v = &row_data[i];
                    if (v->type == VALUE_INTEGER) {
                        new_buf[new_offset++] = 0;
                        *(uint32_t*)(new_buf + new_offset) = sizeof(int64_t);
                        new_offset += 4;
                        *(int64_t*)(new_buf + new_offset) = v->as_int;
                        new_offset += sizeof(int64_t);
                    } else if (v->type == VALUE_FLOAT) {
                        new_buf[new_offset++] = 1;
                        *(uint32_t*)(new_buf + new_offset) = sizeof(double);
                        new_offset += 4;
                        *(double*)(new_buf + new_offset) = v->as_float;
                        new_offset += sizeof(double);
                    } else if (v->type == VALUE_TEXT && v->as_text.str) {
                        size_t slen = strlen(v->as_text.str);
                        size_t remaining = sizeof(new_buf) - new_offset - 5;
                        if (slen > remaining) slen = remaining;
                        new_buf[new_offset++] = 2;
                        *(uint32_t*)(new_buf + new_offset) = (uint32_t)slen;
                        new_offset += 4;
                        memcpy(new_buf + new_offset, v->as_text.str, slen);
                        new_offset += slen;
                    } else {
                        new_buf[new_offset++] = 3;
                        *(uint32_t*)(new_buf + new_offset) = 0;
                        new_offset += 4;
                    }
                }

                /* Update the row in B+tree */
                btree_update(table_tree, key, new_buf, new_offset);

                /* Free updated text value copies */
                for (int i = 0; i < col_idx && i < 16; i++) {
                    if (row_data[i].type == VALUE_TEXT && row_data[i].as_text.str) {
                        free(row_data[i].as_text.str);
                    }
                }

                updated_count++;
                /* After update, stay on current row - next iteration will move forward */
                btree_cursor_next(cursor);
            } else {
                btree_cursor_next(cursor);
            }
        } else {
            btree_cursor_next(cursor);
        }
    }

    if (cursor) btree_cursor_free(cursor);
    btree_close(table_tree);
    free(table_entry);

    (void)updated_count;
    return SUCCESS;
}

int executor_exec_delete(Executor* exec, AstDelete* stmt) {
    if (!exec || !stmt || !stmt->table_name) return ERR_INTERNAL;
    if (!exec->storage) return ERR_INTERNAL;

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

    /* Get table's B+tree */
    BTree* table_tree = NULL;
    if (table_entry->root_page > 0) {
        table_tree = btree_open(pager, cache, table_entry->root_page);
    }

    if (!table_tree) {
        free(table_entry);
        return SUCCESS;  /* Empty table */
    }

    /* Build column names array for WHERE evaluation */
    char* column_names[16];
    int column_count = 0;
    if (table_entry->column_count > 0 && table_entry->columns) {
        for (int i = 0; i < table_entry->column_count && i < 16; i++) {
            column_names[i] = table_entry->columns[i].name;
            column_count++;
        }
    }

    /* Scan all rows and delete those matching WHERE clause */
    int deleted_count = 0;
    BTreeCursor* cursor = btree_first(table_tree);

    while (cursor && btree_cursor_valid(cursor)) {
        uint64_t key;
        uint32_t len;
        char row_buf[2048];

        int ret = btree_get(cursor, &key, row_buf, &len);
        if (ret == SUCCESS && len > 0) {
            /* Deserialize row data */
            Value row_data[16];
            memset(row_data, 0, sizeof(row_data));
            int col_idx = 0;
            int offset = 0;

            while (offset < (int)len && col_idx < column_count) {
                if ((size_t)offset + 5 > len) break;
                uint8_t col_type = (uint8_t)row_buf[offset];
                uint32_t col_len = *(uint32_t*)(row_buf + offset + 1);

                if (col_type == 3 || col_len == 0) break;
                if ((size_t)offset + 5 + col_len > len) break;

                offset += 5;  /* skip type + length */

                if (col_type == 0 && col_len == sizeof(int64_t)) {
                    row_data[col_idx].type = VALUE_INTEGER;
                    row_data[col_idx].as_int = *(int64_t*)(row_buf + offset);
                    offset += sizeof(int64_t);
                    col_idx++;
                } else if (col_type == 1 && col_len == sizeof(double)) {
                    row_data[col_idx].type = VALUE_FLOAT;
                    row_data[col_idx].as_float = *(double*)(row_buf + offset);
                    offset += sizeof(double);
                    col_idx++;
                } else if (col_type == 2) {
                    /* For text columns, skip the data - we only need integer values for WHERE evaluation */
                    /* The as_text.str pointer is NULL after memset, so we can't copy to it */
                    row_data[col_idx].type = VALUE_TEXT;
                    row_data[col_idx].as_text.str = NULL;
                    row_data[col_idx].as_text.len = 0;
                    offset += col_len;
                    col_idx++;
                } else {
                    row_data[col_idx].type = VALUE_NULL;
                    col_idx++;
                }
            }

            /* Evaluate WHERE clause */
            int should_delete = 1;  /* Default: delete if no WHERE */
            if (stmt->where) {
                Value* result = expr_eval(stmt->where, row_data, column_count, (const char**)column_names);
                should_delete = value_is_truthy(result);
                value_free(result);
            }

            if (should_delete) {
                btree_delete(table_tree, key);
                deleted_count++;
                /* After deletion at current position, next cell shifts into this position.
                 * Don't advance cursor - re-read current position on next iteration. */
            } else {
                btree_cursor_next(cursor);
            }
        } else {
            btree_cursor_next(cursor);
        }
    }

    if (cursor) btree_cursor_free(cursor);
    btree_close(table_tree);
    free(table_entry);

    (void)deleted_count;
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

/* Apply ORDER BY - sort result set using quicksort */
void select_apply_order_by(ResultSet* rs, OrderByItem* order_by, char** column_names, ColumnType* column_types) {
    (void)column_types;
    if (!rs || rs->row_count <= 1 || !order_by) return;

    /* Count ORDER BY items and resolve column indices */
    int n = 0;
    int col_indices[16] = { 0 };
    int descending[16] = { 0 };
    for (OrderByItem* item = order_by; item && n < 16; item = item->next) {
        int idx = -1;
        for (int i = 0; i < rs->column_count; i++) {
            if (column_names && strcmp(column_names[i], item->column_name) == 0) {
                idx = i;
                break;
            }
        }
        col_indices[n] = idx;
        descending[n] = item->descending;
        n++;
    }
    if (n == 0) return;

    /* Allocate row index array */
    int* indices = malloc(sizeof(int) * (size_t)rs->row_count);
    if (!indices) return;
    for (int i = 0; i < rs->row_count; i++) {
        indices[i] = i;
    }

    /* Sort by swapping indices in-place using iterative quicksort */
    int lo = 0, hi = rs->row_count - 1;
    int stack[256], sp = 0;
    stack[sp++] = lo;
    stack[sp++] = hi;

    while (sp > 0) {
        hi = stack[--sp];
        lo = stack[--sp];

        if (lo >= hi) continue;

        /* Partition: median-of-three pivot selection */
        int mid = lo + (hi - lo) / 2;
        int pivot = indices[mid];
        indices[mid] = indices[hi];
        indices[hi] = pivot;

        int store = lo;
        for (int i = lo; i < hi; i++) {
            int ai = indices[i];
            int cmp = 0;

            /* Multi-column comparison using ORDER BY items */
            for (int k = 0; k < n; k++) {
                int col = col_indices[k];
                if (col < 0 || col >= rs->column_count) continue;
                cmp = value_compare(&rs->rows[ai][col], &rs->rows[pivot][col]);
                if (cmp != 0) {
                    if (descending[k]) cmp = -cmp;
                    break;
                }
            }

            if (cmp <= 0) {
                int tmp = indices[store];
                indices[store] = indices[i];
                indices[i] = tmp;
                store++;
            }
        }
        indices[hi] = indices[store];
        indices[store] = pivot;

        /* Push larger subarray first to limit stack depth */
        int left_size = store - lo;
        int right_size = hi - store;
        if (left_size > right_size) {
            stack[sp++] = lo;
            stack[sp++] = store - 1;
            stack[sp++] = store + 1;
            stack[sp++] = hi;
        } else {
            stack[sp++] = store + 1;
            stack[sp++] = hi;
            stack[sp++] = lo;
            stack[sp++] = store - 1;
        }
    }

    /* Reorder rows in-place according to sorted index array */
    Value** rows_copy = malloc(sizeof(Value*) * (size_t)rs->row_count);
    if (!rows_copy) {
        free(indices);
        return;
    }
    memcpy(rows_copy, rs->rows, sizeof(Value*) * (size_t)rs->row_count);
    for (int i = 0; i < rs->row_count; i++) {
        rs->rows[i] = rows_copy[indices[i]];
    }

    free(rows_copy);
    free(indices);
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

    /* First, send header row with column names */
    if (table_entry->column_count > 0 && table_entry->columns) {
        char header_buf[1024] = {0};
        int header_len = 0;
        for (int i = 0; i < table_entry->column_count; i++) {
            if (i > 0) {
                header_len += snprintf(header_buf + header_len, sizeof(header_buf) - header_len, "\t");
            }
            header_len += snprintf(header_buf + header_len, sizeof(header_buf) - header_len, "%s",
                                   table_entry->columns[i].name);
        }
        size_t needed = result_len + header_len + 16;
        if (result_capacity < needed) {
            size_t new_cap = result_capacity == 0 ? 4096 : result_capacity * 2;
            if (new_cap < needed) new_cap = needed;
            result_buf = realloc(result_buf, new_cap);
            result_capacity = new_cap;
        }
        result_len += snprintf(result_buf + result_len, result_capacity - result_len, "ROW:%s\n", header_buf);
    }

    while (cursor && btree_cursor_valid(cursor)) {
        uint64_t key;
        uint32_t len;
        char row_buf[2048];

        int ret = btree_get(cursor, &key, row_buf, &len);
        if (ret == SUCCESS && len > 0) {
            row_count++;

            /* Deserialize row data and format as tab-separated string */
            /* row_buf starts directly with first column's type byte (no rowid prefix)
             * Format: [type(1) + len(4) + data(n)] per column, repeated */
            char line_buf[4096] = {0};
            int offset = 0;
            int col_idx = 0;

            /* Read column values using length-prefixed format:
             * each value is: type_byte(1) + length(4) + data(variable)
             * type: 0=integer, 1=float, 2=text, 3=blob (matches ColumnType enum) */
            while (offset < (int)len && col_idx < 16) {
                if ((size_t)offset + 5 > len) break;
                uint8_t col_type = (uint8_t)row_buf[offset];
                uint32_t col_len = *(uint32_t*)(row_buf + offset + 1);

                if (col_type == 3 || col_len == 0) break;
                if ((size_t)offset + 5 + col_len > len) break;

                offset += 5;  /* skip type + length */
                size_t used = strlen(line_buf);

                if (col_type == 0 && col_len == sizeof(int64_t)) {
                    int64_t int_val = *(int64_t*)(row_buf + offset);
                    offset += sizeof(int64_t);
                    if (used > 0) {
                        snprintf(line_buf + used, sizeof(line_buf) - used, "\t%lld", (long long)int_val);
                    } else {
                        snprintf(line_buf + used, sizeof(line_buf) - used, "%lld", (long long)int_val);
                    }
                    col_idx++;
                } else if (col_type == 1 && col_len == sizeof(double)) {
                    double float_val = *(double*)(row_buf + offset);
                    offset += sizeof(double);
                    if (used > 0) {
                        snprintf(line_buf + used, sizeof(line_buf) - used, "\t%g", float_val);
                    } else {
                        snprintf(line_buf + used, sizeof(line_buf) - used, "%g", float_val);
                    }
                    col_idx++;
                } else if (col_type == 2) {
                    char str_buf[1024];
                    size_t copy_len = col_len < sizeof(str_buf) - 1 ? col_len : sizeof(str_buf) - 1;
                    memcpy(str_buf, row_buf + offset, copy_len);
                    str_buf[copy_len] = '\0';
                    offset += col_len;
                    if (used > 0) {
                        snprintf(line_buf + used, sizeof(line_buf) - used, "\t%s", str_buf);
                    } else {
                        snprintf(line_buf + used, sizeof(line_buf) - used, "%s", str_buf);
                    }
                    col_idx++;
                } else {
                    offset += col_len;
                }
            }

            /* Append ROW: line to result buffer */
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

    /* Track rows in executor stats for server to retrieve */
    exec->rows_read += row_count;

    /* Store result buffer in data for server to retrieve */
    if (data && result_buf) {
        SelectResult* sel_result = (SelectResult*)data;
        sel_result->result_buf = result_buf;
        sel_result->result_len = result_len;
        sel_result->row_count = row_count;
    } else if (result_buf) {
        free(result_buf);
    }

    /* Note: callback would be called with each row for full implementation */
    return SUCCESS;
}

/*============================================================================
 * SHOW TABLES processing
 *============================================================================*/
int executor_exec_show_tables(Executor* exec, AstShowTables* stmt, ResultCallback callback, void* data) {
    (void)callback;
    if (!exec || !stmt) return ERR_INTERNAL;
    if (!exec->storage) return ERR_INTERNAL;

    Catalog* catalog = storage_get_catalog(exec->storage);
    if (!catalog) return ERR_INTERNAL;

    int count = 0;
    CatalogEntry** tables = catalog_get_tables(catalog, &count);
    if (tables == NULL && count == 0) {
        /* Empty database - return success with empty result */
        if (data) {
            SelectResult* sel_result = (SelectResult*)data;
            sel_result->result_buf = NULL;
            sel_result->result_len = 0;
            sel_result->row_count = 0;
        }
        return SUCCESS;
    }
    if (!tables) return ERR_INTERNAL;

    /* Buffer to accumulate all ROW: lines for server to send */
    char* result_buf = NULL;
    size_t result_len = 0;
    size_t result_capacity = 0;

    for (int i = 0; i < count; i++) {
        size_t needed = result_len + strlen(tables[i]->name) + 16;
        if (result_capacity < needed) {
            size_t new_cap = result_capacity == 0 ? 4096 : result_capacity * 2;
            if (new_cap < needed) new_cap = needed;
            result_buf = realloc(result_buf, new_cap);
            result_capacity = new_cap;
        }
        result_len += snprintf(result_buf + result_len, result_capacity - result_len, "ROW:%s\n", tables[i]->name);
    }

    catalog_free_entries(tables, count);

    /* Store result buffer in data for server to retrieve */
    if (data && result_buf) {
        SelectResult* sel_result = (SelectResult*)data;
        sel_result->result_buf = result_buf;
        sel_result->result_len = result_len;
        sel_result->row_count = count;
    } else if (result_buf) {
        free(result_buf);
    }

    return SUCCESS;
}

/*============================================================================
 * DESCRIBE TABLE processing
 *============================================================================*/

/* Convert ColumnType to string representation */
static const char* column_type_name(int type) {
    switch (type) {
        case 0: return "integer";
        case 1: return "float";
        case 2: return "text";
        case 3: return "blob";
        default: return "text";
    }
}

int executor_exec_describe_table(Executor* exec, AstDescribeTable* stmt, ResultCallback callback, void* data) {
    (void)callback;
    if (!exec || !stmt) return ERR_INTERNAL;
    if (!exec->storage) return ERR_INTERNAL;
    if (!stmt->table_name) return ERR_INTERNAL;

    Catalog* catalog = storage_get_catalog(exec->storage);
    if (!catalog) return ERR_INTERNAL;

    /* Get columns from catalog metadata */
    int column_count = 0;
    ColumnInfo* columns = catalog_get_columns(catalog, stmt->table_name, &column_count);
    if (!columns) {
        return ERR_EXEC_TABLE_NOT_FOUND;
    }

    /* Buffer to accumulate all ROW: lines */
    char* result_buf = NULL;
    size_t result_len = 0;
    size_t result_capacity = 0;

    for (int i = 0; i < column_count; i++) {
        ColumnInfo* col = &columns[i];

        /* Format: Field\tType\tNull\tKey\tDefault\tExtra (6 columns) */
        char line_buf[512];
        const char* type_str = column_type_name(col->type);
        const char* null_str = col->not_null ? "NO" : "YES";
        const char* key_str = col->primary_key ? "PRI" : "";
        const char* default_str = col->default_val[0] ? col->default_val : "NULL";
        const char* extra_str = col->autoincrement ? "auto_increment" : "";

        /* Build line with exactly 6 fields: name, type, null, key, default, extra */
        snprintf(line_buf, sizeof(line_buf), "%s\t%s\t%s\t%s\t%s\t%s",
                 col->name, type_str, null_str, key_str, default_str, extra_str);

        size_t needed = result_len + strlen(line_buf) + 16;
        if (result_capacity < needed) {
            size_t new_cap = result_capacity == 0 ? 4096 : result_capacity * 2;
            if (new_cap < needed) new_cap = needed;
            result_buf = realloc(result_buf, new_cap);
            result_capacity = new_cap;
        }
        result_len += snprintf(result_buf + result_len, result_capacity - result_len, "ROW:%s\n", line_buf);
    }

    free(columns);

    /* Store result buffer in data for server to retrieve */
    if (data && result_buf) {
        SelectResult* sel_result = (SelectResult*)data;
        sel_result->result_buf = result_buf;
        sel_result->result_len = result_len;
        sel_result->row_count = column_count;
    } else if (result_buf) {
        free(result_buf);
    }

    return SUCCESS;
}

ResultSet* executor_select(Executor* exec, AstSelect* select) {
    if (!exec || !select) return result_set_create(16);
    if (!exec->storage) return result_set_create(16);

    /* Empty result if no table (scalar SELECT) */
    if (!select->table_name) {
        ResultSet* rs = result_set_create(1);
        return rs;
    }

    /* Get storage components */
    Catalog* catalog = storage_get_catalog(exec->storage);
    Pager* pager = storage_get_pager(exec->storage);
    PageCache* cache = storage_get_cache(exec->storage);
    if (!catalog || !pager || !cache) return result_set_create(16);

    /* Lookup table in catalog */
    CatalogEntry* table_entry = catalog_lookup_type_name(catalog, CATALOG_TYPE_TABLE, select->table_name);
    if (!table_entry) return result_set_create(16);

    /* Get table's B+tree */
    BTree* table_tree = NULL;
    if (table_entry->root_page > 0) {
        table_tree = btree_open(pager, cache, table_entry->root_page);
    }

    if (!table_tree) {
        free(table_entry);
        return result_set_create(16);  /* Empty table */
    }

    /* Build column metadata from catalog */
    int table_col_count = table_entry->column_count;
    if (table_col_count > 16) table_col_count = 16;

    const char* src_column_names[16];
    ColumnType src_column_types[16];
    (void)src_column_types;  /* reserved for future type-based filtering */
    for (int i = 0; i < table_col_count; i++) {
        src_column_names[i] = table_entry->columns[i].name;
        src_column_types[i] = table_entry->columns[i].type;
    }

    /* Create result set */
    ResultSet* rs = result_set_create(64);
    rs->column_count = table_col_count;

    /* Scan all rows from B+tree */
    BTreeCursor* cursor = btree_first(table_tree);

    while (cursor && btree_cursor_valid(cursor)) {
        uint64_t key;
        uint32_t len;
        char row_buf[2048];

        int ret = btree_get(cursor, &key, row_buf, &len);
        if (ret == SUCCESS && len > 0) {
            /* Deserialize row into Value array */
            Value row_data[16];
            memset(row_data, 0, sizeof(row_data));
            int col_idx = 0;
            int offset = 0;

            while (offset < (int)len && col_idx < table_col_count) {
                if ((size_t)offset + 5 > len) break;
                uint8_t col_type = (uint8_t)row_buf[offset];
                uint32_t col_len = *(uint32_t*)(row_buf + offset + 1);

                if (col_type == 3 || col_len == 0) break;  /* null */
                if ((size_t)offset + 5 + col_len > len) break;

                offset += 5;  /* skip type(1) + length(4) */

                if (col_type == 0 && col_len == sizeof(int64_t)) {
                    row_data[col_idx].type = VALUE_INTEGER;
                    row_data[col_idx].as_int = *(int64_t*)(row_buf + offset);
                    offset += (int)sizeof(int64_t);
                    col_idx++;
                } else if (col_type == 1 && col_len == sizeof(double)) {
                    row_data[col_idx].type = VALUE_FLOAT;
                    row_data[col_idx].as_float = *(double*)(row_buf + offset);
                    offset += (int)sizeof(double);
                    col_idx++;
                } else if (col_type == 2) {
                    row_data[col_idx].type = VALUE_TEXT;
                    row_data[col_idx].as_text.str = malloc(col_len + 1);
                    if (row_data[col_idx].as_text.str) {
                        memcpy(row_data[col_idx].as_text.str, row_buf + offset, col_len);
                        row_data[col_idx].as_text.str[col_len] = '\0';
                        row_data[col_idx].as_text.len = col_len;
                    } else {
                        row_data[col_idx].as_text.str = NULL;
                        row_data[col_idx].as_text.len = 0;
                    }
                    offset += (int)col_len;
                    col_idx++;
                } else {
                    row_data[col_idx].type = VALUE_NULL;
                    col_idx++;
                }
            }

            /* Apply WHERE clause */
            if (!select_apply_where(row_data, select->where, table_col_count, src_column_names)) {
                /* Free text copies before skipping */
                for (int i = 0; i < col_idx; i++) {
                    if (row_data[i].type == VALUE_TEXT && row_data[i].as_text.str) {
                        free(row_data[i].as_text.str);
                    }
                }
                btree_cursor_next(cursor);
                continue;
            }

            /* Project columns */
            Value* projected = select_project_columns(
                row_data, col_idx, select->columns,
                src_column_names, &rs->column_names,
                &rs->column_types, &rs->column_count);

            /* Add row to result set */
            result_set_add_row(rs, projected);

            /* Free source row text copies (projected columns are copies or aliases) */
            for (int i = 0; i < col_idx; i++) {
                if (row_data[i].type == VALUE_TEXT && row_data[i].as_text.str) {
                    free(row_data[i].as_text.str);
                }
            }
        }

        btree_cursor_next(cursor);
    }

    if (cursor) btree_cursor_free(cursor);
    btree_close(table_tree);
    free(table_entry);

    /* Apply post-scan processing */
    if (select->is_distinct) {
        select_apply_distinct(rs);
    }
    if (select->order_by && rs->column_names) {
        select_apply_order_by(rs, select->order_by, rs->column_names, rs->column_types);
    }
    if (select->limit || select->offset) {
        select_apply_limit(rs, select->limit, select->offset);
    }

    exec->rows_read += (uint64_t)rs->row_count;
    return rs;
}

/*============================================================================
 * Subquery evaluation
 *============================================================================*/

/* Evaluate a scalar subquery and return its first value.
 * Used by expression.c for EXPR_SUBQUERY evaluation.
 * Returns a Value (caller must free), or NULL on error. */
Value* executor_evaluate_subquery(Executor* exec, AstNode* query) {
    /* Subqueries use the caller's executor if available, otherwise create a minimal one */
    if (!exec) {
        /* Subqueries need the same storage context - this will need to be passed from
         * expression evaluation context in a real implementation. For now, return NULL. */
        return value_from_null();
    }

    if (!query || query->type != AST_SELECT) {
        return value_from_null();
    }

    /* Execute the subquery as a SELECT */
    ResultSet* rs = executor_select(exec, (AstSelect*)query);
    if (!rs || rs->row_count == 0) {
        if (rs) result_set_free(rs);
        return value_from_null();
    }

    /* Return first value of first row */
    Value* result = NULL;
    if (rs->rows[0] && rs->column_count > 0) {
        result = value_from_null();
        Value* src = &rs->rows[0][0];
        switch (src->type) {
            case VALUE_INTEGER:
                result = value_from_int(src->as_int);
                break;
            case VALUE_FLOAT:
                result = value_from_float(src->as_float);
                break;
            case VALUE_TEXT:
                result = value_from_text(src->as_text.str, src->as_text.len);
                break;
            default:
                result = value_from_null();
                break;
        }
    }

    result_set_free(rs);
    return result;
}