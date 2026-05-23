#define _POSIX_C_SOURCE 200809L

#include "catalog.h"
#include "../util/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*============================================================================
 * Column Serialization Helpers
 *============================================================================*/

/* Serialized column info: name(64) + type(4) + not_null(4) + primary_key(4) + autoincrement(4) + default_val(128) = 208 bytes */
#define COLUMN_INFO_SIZE 208

static int serialize_column_info(const ColumnInfo* col, void* buf) {
    if (!col || !buf) return ERR_INTERNAL;

    uint8_t* data = (uint8_t*)buf;
    uint32_t offset = 0;

    /* name: 64 bytes */
    memset(data + offset, 0, 64);
    strncpy((char*)(data + offset), col->name, 63);
    offset += 64;

    /* type: 4 bytes */
    *(int*)(data + offset) = col->type;
    offset += 4;

    /* not_null: 4 bytes */
    *(int*)(data + offset) = col->not_null;
    offset += 4;

    /* primary_key: 4 bytes */
    *(int*)(data + offset) = col->primary_key;
    offset += 4;

    /* autoincrement: 4 bytes */
    *(int*)(data + offset) = col->autoincrement;
    offset += 4;

    /* default_val: 128 bytes */
    memset(data + offset, 0, 128);
    if (col->default_val[0] != '\0') {
        strncpy((char*)(data + offset), col->default_val, 127);
    }
    offset += 128;

    (void)offset;
    return SUCCESS;
}

static int deserialize_column_info(const void* buf, ColumnInfo* col) {
    if (!buf || !col) return ERR_INTERNAL;

    const uint8_t* data = (const uint8_t*)buf;

    /* name: 64 bytes */
    memset(col->name, 0, 64);
    strncpy(col->name, (const char*)data, 63);
    data += 64;

    /* type: 4 bytes */
    col->type = *(int*)data;
    data += 4;

    /* not_null: 4 bytes */
    col->not_null = *(int*)data;
    data += 4;

    /* primary_key: 4 bytes */
    col->primary_key = *(int*)data;
    data += 4;

    /* autoincrement: 4 bytes */
    col->autoincrement = *(int*)data;
    data += 4;

    /* default_val: 128 bytes */
    memset(col->default_val, 0, 128);
    strncpy(col->default_val, (const char*)data, 127);

    return SUCCESS;
}

/*============================================================================
 * Catalog Entry Serialization
 *============================================================================*/

/*
 * Catalog entry serialized format:
 * type(4) + name(64) + tbl_name(64) + sql(512) + root_page(4) + is_valid(4) + column_count(4) + columns(var)
 * Minimum size: 656 bytes (no columns)
 * With columns: 656 + 4 + column_count * 208 bytes
 */
#define CATALOG_ENTRY_BASE_SIZE 656
#define CATALOG_ENTRY_MAX_SIZE (CATALOG_ENTRY_BASE_SIZE + MAX_TABLE_COLUMNS * COLUMN_INFO_SIZE)

static int serialize_entry(const CatalogEntry* entry, void* buf, size_t buf_size, size_t* out_size) {
    if (!entry || !buf) return ERR_INTERNAL;

    uint8_t* data = (uint8_t*)buf;
    uint32_t offset = 0;

    /* type: 4 bytes */
    *(uint32_t*)(data + offset) = entry->type;
    offset += 4;

    /* name: 64 bytes */
    memset(data + offset, 0, 64);
    strncpy((char*)(data + offset), entry->name, 63);
    offset += 64;

    /* tbl_name: 64 bytes */
    memset(data + offset, 0, 64);
    strncpy((char*)(data + offset), entry->tbl_name, 63);
    offset += 64;

    /* sql: 512 bytes */
    memset(data + offset, 0, 512);
    if (entry->sql[0] != '\0') {
        strncpy((char*)(data + offset), entry->sql, 511);
    }
    offset += 512;

    /* root_page: 4 bytes */
    *(uint32_t*)(data + offset) = entry->root_page;
    offset += 4;

    /* is_valid: 4 bytes */
    *(uint32_t*)(data + offset) = entry->is_valid ? 1 : 0;
    offset += 4;

    /* column_count: 4 bytes */
    *(int*)(data + offset) = entry->column_count;
    offset += 4;

    /* column data: variable - column_count * COLUMN_INFO_SIZE bytes */
    for (int i = 0; i < entry->column_count; i++) {
        if (offset + COLUMN_INFO_SIZE > buf_size) {
            return ERR_INTERNAL;
        }
        serialize_column_info(&entry->columns[i], data + offset);
        offset += COLUMN_INFO_SIZE;
    }

    *out_size = offset;
    return SUCCESS;
}

static int deserialize_entry(const void* buf, size_t buf_size, CatalogEntry* entry) {
    if (!buf || !entry) return ERR_INTERNAL;

    /* Initialize columns to NULL for proper cleanup on error */
    entry->columns = NULL;
    entry->column_count = 0;

    const uint8_t* data = (const uint8_t*)buf;

    /* type: 4 bytes */
    entry->type = *(uint32_t*)data;
    data += 4;

    /* name: 64 bytes */
    memset(entry->name, 0, 64);
    strncpy(entry->name, (const char*)data, 63);
    data += 64;

    /* tbl_name: 64 bytes */
    memset(entry->tbl_name, 0, 64);
    strncpy(entry->tbl_name, (const char*)data, 63);
    data += 64;

    /* sql: 512 bytes */
    memset(entry->sql, 0, 512);
    strncpy(entry->sql, (const char*)data, 511);
    data += 512;

    /* root_page: 4 bytes */
    entry->root_page = *(uint32_t*)data;
    data += 4;

    /* is_valid: 4 bytes */
    entry->is_valid = *(uint32_t*)data != 0;
    data += 4;

    /* column_count: 4 bytes */
    entry->column_count = *(int*)data;
    data += 4;

    /* column data: variable - column_count * COLUMN_INFO_SIZE bytes */
    if (entry->column_count > 0 && entry->column_count <= MAX_TABLE_COLUMNS) {
        /* Verify we have enough data */
        size_t consumed = data - (const uint8_t*)buf;
        size_t needed = consumed + entry->column_count * COLUMN_INFO_SIZE;
        if (needed > buf_size) {
            entry->column_count = 0;
            return ERR_INTERNAL;
        }

        entry->columns = malloc(entry->column_count * sizeof(ColumnInfo));
        if (!entry->columns) {
            entry->column_count = 0;
            return ERR_INTERNAL;
        }

        for (int i = 0; i < entry->column_count; i++) {
            if (deserialize_column_info(data, &entry->columns[i]) != SUCCESS) {
                free(entry->columns);
                entry->columns = NULL;
                entry->column_count = 0;
                return ERR_INTERNAL;
            }
            data += COLUMN_INFO_SIZE;
        }
    } else {
        entry->columns = NULL;
        entry->column_count = 0;
    }

    return SUCCESS;
}

/*============================================================================
 * Catalog Lifecycle
 *============================================================================*/

Catalog* catalog_open(Pager* pager, PageCache* cache) {
    if (!pager || !cache) return NULL;

    Catalog* catalog = calloc(1, sizeof(Catalog));
    if (!catalog) return NULL;

    catalog->pager = pager;
    catalog->cache = cache;

    /* Open or create B+Tree for catalog */
    catalog->tree = btree_create(pager, cache);
    if (!catalog->tree) {
        free(catalog);
        return NULL;
    }

    pthread_mutex_init(&catalog->mutex, NULL);
    catalog->is_open = 1;

    return catalog;
}

void catalog_close(Catalog* catalog) {
    if (!catalog) return;

    if (catalog->tree) {
        btree_close(catalog->tree);
    }

    pthread_mutex_destroy(&catalog->mutex);
    free(catalog);
}

int catalog_init(Catalog* catalog) {
    if (!catalog) return ERR_INTERNAL;

    /* Initialize tinydb_master table entry */
    CatalogEntry entry;
    memset(&entry, 0, sizeof(entry));

    entry.type = CATALOG_TYPE_TABLE;
    strcpy(entry.name, CATALOG_TABLE_NAME);
    strcpy(entry.tbl_name, CATALOG_TABLE_NAME);
    strcpy(entry.sql, "CREATE TABLE tinydb_master (type TEXT, name TEXT, tbl_name TEXT, sql TEXT)");

    /* Insert into catalog tree with special key (0) */
    char buf[CATALOG_ENTRY_MAX_SIZE];
    size_t buf_size = 0;
    serialize_entry(&entry, buf, sizeof(buf), &buf_size);

    int ret = btree_insert(catalog->tree, 0, buf, (uint32_t)buf_size);
    if (ret != SUCCESS) {
        return ret;
    }

    return SUCCESS;
}

/*============================================================================
 * Catalog Operations
 *============================================================================*/

int catalog_insert(Catalog* catalog, const CatalogEntry* entry) {
    if (!catalog || !entry) return ERR_INTERNAL;

    /* Use monotonic counter for unique keys - avoids collisions from same-length names */
    uint64_t key = catalog->next_key++;

    char buf[CATALOG_ENTRY_MAX_SIZE];
    size_t buf_size = 0;
    serialize_entry(entry, buf, sizeof(buf), &buf_size);

    return btree_insert(catalog->tree, key, buf, (uint32_t)buf_size);
}

int catalog_delete(Catalog* catalog, CatalogEntryType type, const char* name) {
    (void)catalog;
    (void)type;
    (void)name;
    /* TODO: Implement catalog delete */
    return ERR_INTERNAL;
}

CatalogEntry* catalog_lookup_type_name(Catalog* catalog,
                                       CatalogEntryType type,
                                       const char* name) {
    if (!catalog || !name) return NULL;

    /* Search for entry with matching type and name */
    /* For now, do a linear scan */
    CatalogCursor* cursor = catalog_cursor_create(catalog);
    if (!cursor) return NULL;

    CatalogEntry* result = NULL;

    while (catalog_cursor_valid(cursor)) {
        CatalogEntry* entry = catalog_cursor_get(cursor);
        if (entry) {
            int type_match = (entry->type == type);
            int name_match = (strcmp(entry->name, name) == 0);

            if (type_match && name_match) {
                result = malloc(sizeof(CatalogEntry));
                if (result) {
                    memcpy(result, entry, sizeof(CatalogEntry));
                }
                break;
            }
        }
        catalog_cursor_next(cursor);
    }

    catalog_cursor_free(cursor);
    return result;
}

CatalogEntry** catalog_get_tables(Catalog* catalog, int* count) {
    if (!catalog || !count) return NULL;

    *count = 0;
    CatalogEntry** entries = NULL;

    CatalogCursor* cursor = catalog_cursor_create(catalog);
    if (!cursor) return NULL;

    while (catalog_cursor_valid(cursor)) {
        CatalogEntry* entry = catalog_cursor_get(cursor);
        if (entry && entry->type == CATALOG_TYPE_TABLE &&
            strcmp(entry->name, CATALOG_TABLE_NAME) != 0) {
            CatalogEntry** new_entries = realloc(entries, (*count + 1) * sizeof(CatalogEntry*));
            if (!new_entries) {
                catalog_free_entries(entries, *count);
                catalog_cursor_free(cursor);
                return NULL;
            }
            entries = new_entries;
            entries[*count] = malloc(sizeof(CatalogEntry));
            if (entries[*count]) {
                memcpy(entries[*count], entry, sizeof(CatalogEntry));
                (*count)++;
            }
            free(entry);  /* catalog_cursor_get allocates fresh copy each call */
        }
        catalog_cursor_next(cursor);
    }

    catalog_cursor_free(cursor);
    return entries;
}

CatalogEntry** catalog_get_indexes(Catalog* catalog, const char* table_name, int* count) {
    if (!catalog || !table_name || !count) return NULL;

    *count = 0;
    CatalogEntry** entries = NULL;

    CatalogCursor* cursor = catalog_cursor_create(catalog);
    if (!cursor) return NULL;

    while (catalog_cursor_valid(cursor)) {
        CatalogEntry* entry = catalog_cursor_get(cursor);
        if (entry && entry->type == CATALOG_TYPE_INDEX) {
            if (strcmp(entry->tbl_name, table_name) == 0) {
                CatalogEntry** new_entries = realloc(entries, (*count + 1) * sizeof(CatalogEntry*));
                if (!new_entries) {
                    catalog_free_entries(entries, *count);
                    catalog_cursor_free(cursor);
                    return NULL;
                }
                entries = new_entries;
                entries[*count] = malloc(sizeof(CatalogEntry));
                if (entries[*count]) {
                    memcpy(entries[*count], entry, sizeof(CatalogEntry));
                    (*count)++;
                }
            }
        }
        catalog_cursor_next(cursor);
    }

    catalog_cursor_free(cursor);
    return entries;
}

void catalog_free_entries(CatalogEntry** entries, int count) {
    if (!entries) return;
    for (int i = 0; i < count; i++) {
        if (entries[i]) {
            /* Free column data if present */
            if (entries[i]->columns) {
                free(entries[i]->columns);
            }
            free(entries[i]);
        }
    }
    free(entries);
}

ColumnInfo* catalog_get_columns(Catalog* catalog, const char* table_name, int* column_count) {
    if (!catalog || !table_name || !column_count) return NULL;

    *column_count = 0;

    CatalogEntry* entry = catalog_lookup_type_name(catalog, CATALOG_TYPE_TABLE, table_name);
    if (!entry) return NULL;

    if (entry->column_count > 0 && entry->columns) {
        /* Return a copy of the column info */
        ColumnInfo* cols = malloc(entry->column_count * sizeof(ColumnInfo));
        if (cols) {
            memcpy(cols, entry->columns, entry->column_count * sizeof(ColumnInfo));
            *column_count = entry->column_count;
        }
        free(entry);
        return cols;
    }

    free(entry);
    return NULL;
}

/*============================================================================
 * Catalog Iteration
 *============================================================================*/

CatalogCursor* catalog_cursor_create(Catalog* catalog) {
    if (!catalog || !catalog->tree) return NULL;

    CatalogCursor* cursor = calloc(1, sizeof(CatalogCursor));
    if (!cursor) return NULL;

    cursor->btree_cursor = btree_first(catalog->tree);
    cursor->is_end = (cursor->btree_cursor == NULL);

    return cursor;
}

void catalog_cursor_next(CatalogCursor* cursor) {
    if (!cursor || cursor->is_end) return;

    if (cursor->btree_cursor) {
        btree_cursor_next(cursor->btree_cursor);
        if (!btree_cursor_valid(cursor->btree_cursor)) {
            cursor->is_end = 1;
        }
    }
}

CatalogEntry* catalog_cursor_get(CatalogCursor* cursor) {
    if (!cursor || cursor->is_end) return NULL;

    /* Allocate fresh copy each time - caller must free */
    CatalogEntry* entry = calloc(1, sizeof(CatalogEntry));
    if (!entry) return NULL;

    uint64_t key;
    char buf[CATALOG_ENTRY_MAX_SIZE];
    uint32_t len = sizeof(buf);

    if (btree_get(cursor->btree_cursor, &key, buf, &len) != SUCCESS) {
        free(entry);
        return NULL;
    }

    if (deserialize_entry(buf, len, entry) != SUCCESS) {
        free(entry);
        return NULL;
    }

    return entry;
}

int catalog_cursor_valid(CatalogCursor* cursor) {
    if (!cursor) return 0;
    if (cursor->is_end) return 0;
    if (!cursor->btree_cursor) return 0;
    return btree_cursor_valid(cursor->btree_cursor);
}

void catalog_cursor_free(CatalogCursor* cursor) {
    if (cursor) {
        if (cursor->btree_cursor) {
            btree_cursor_free(cursor->btree_cursor);
        }
        free(cursor);
    }
}