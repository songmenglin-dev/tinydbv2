#define _POSIX_C_SOURCE 200809L

#include "catalog.h"
#include "../util/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*============================================================================
 * Catalog Entry Serialization
 *============================================================================*/

/* Serialized format: type(4) + name(64) + tbl_name(64) + sql(512) = 644 bytes */
#define CATALOG_ENTRY_SIZE 644

static int serialize_entry(const CatalogEntry* entry, void* buf) {
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

    (void)offset; /* unused - kept for clarity */
    return SUCCESS;
}

static int deserialize_entry(const void* buf, CatalogEntry* entry) {
    if (!buf || !entry) return ERR_INTERNAL;

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
    char buf[CATALOG_ENTRY_SIZE];
    serialize_entry(&entry, buf);

    int ret = btree_insert(catalog->tree, 0, buf, CATALOG_ENTRY_SIZE);
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

    /* Use type + name as key for uniqueness */
    uint64_t key = 0;
    if (entry->type == CATALOG_TYPE_TABLE) {
        key = 1 + (uint64_t)strlen(entry->name);
    } else {
        key = 0x80000000 + (uint64_t)strlen(entry->name);
    }

    char buf[CATALOG_ENTRY_SIZE];
    serialize_entry(entry, buf);

    return btree_insert(catalog->tree, key, buf, CATALOG_ENTRY_SIZE);
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
        if (entry && entry->type == CATALOG_TYPE_TABLE) {
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
        if (entries[i]) free(entries[i]);
    }
    free(entries);
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

    static CatalogEntry entry;
    uint64_t key;
    char buf[CATALOG_ENTRY_SIZE];
    uint32_t len = sizeof(buf);

    if (btree_get(cursor->btree_cursor, &key, buf, &len) != SUCCESS) {
        return NULL;
    }

    if (deserialize_entry(buf, &entry) != SUCCESS) {
        return NULL;
    }

    return &entry;
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