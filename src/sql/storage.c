#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "../../include/tinydb.h"
#include "../util/error.h"
#include <pthread.h>
#include "../storage/pager.h"
#include "../storage/page_cache.h"
#include "../storage/btree.h"
#include "catalog.h"
#include <stdlib.h>
#include <string.h>

/*============================================================================
 * Storage implementation with real pager, cache, and catalog
 *============================================================================*/

struct Storage {
    Pager* pager;
    PageCache* cache;
    Catalog* catalog;
    char* path;
    int is_open;
};

/* Connection structure */
struct Connection {
    Storage* storage;
    int is_connected;
};

int tinydb_open(Storage** storage, const char* path) {
    if (!storage) return ERR_INTERNAL;

    Storage* s = calloc(1, sizeof(Storage));
    if (!s) return ERR_OUT_OF_MEMORY;

    s->path = path ? malloc(strlen(path) + 1) : NULL;
    if (path && s->path) {
        strcpy(s->path, path);
    }

    /* Open or create pager */
    if (path) {
        s->pager = pager_open(path);
        if (!s->pager) {
            s->pager = pager_create(path);
        }
        if (!s->pager) {
            free(s->path);
            free(s);
            return ERR_STORAGE_IO;
        }
    } else {
        s->pager = pager_create(":memory:");
        if (!s->pager) {
            free(s);
            return ERR_STORAGE_IO;
        }
    }

    /* Create page cache */
    s->cache = page_cache_create(256, s->pager);
    if (!s->cache) {
        pager_close(s->pager);
        free(s->path);
        free(s);
        return ERR_STORAGE_IO;
    }

    /* Open catalog */
    s->catalog = catalog_open(s->pager, s->cache);
    if (!s->catalog) {
        page_cache_destroy(s->cache);
        pager_close(s->pager);
        free(s->path);
        free(s);
        return ERR_STORAGE_IO;
    }

    s->is_open = 1;
    *storage = s;
    return SUCCESS;
}

void tinydb_close(Storage* storage) {
    if (!storage) return;
    if (storage->catalog) {
        catalog_close(storage->catalog);
    }
    if (storage->cache) {
        page_cache_destroy(storage->cache);
    }
    if (storage->pager) {
        pager_close(storage->pager);
    }
    free(storage->path);
    free(storage);
}

int tinydb_begin(Storage* storage) {
    if (!storage) return ERR_INTERNAL;
    if (!storage->is_open) return ERR_INTERNAL;
    return SUCCESS;
}

int tinydb_commit(Storage* storage) {
    if (!storage) return ERR_INTERNAL;
    if (!storage->is_open) return ERR_INTERNAL;
    return SUCCESS;
}

int tinydb_rollback(Storage* storage) {
    if (!storage) return ERR_INTERNAL;
    if (!storage->is_open) return ERR_INTERNAL;
    return SUCCESS;
}

/* DML stubs */
int tinydb_insert(Storage* storage, const char* table, void* row) {
    (void)storage;
    (void)table;
    (void)row;
    return SUCCESS;
}

int tinydb_update(Storage* storage, const char* table, void* row, void* where) {
    (void)storage;
    (void)table;
    (void)row;
    (void)where;
    return SUCCESS;
}

int tinydb_delete(Storage* storage, const char* table, void* where) {
    (void)storage;
    (void)table;
    (void)where;
    return SUCCESS;
}

int tinydb_select(Storage* storage, const char* table, void* where,
                  ResultCallback callback, void* data) {
    (void)storage;
    (void)table;
    (void)where;
    (void)callback;
    (void)data;
    return SUCCESS;
}

/* Schema stubs */
int tinydb_create_table(Storage* storage, const char* sql) {
    (void)storage;
    (void)sql;
    return SUCCESS;
}

int tinydb_drop_table(Storage* storage, const char* table) {
    (void)storage;
    (void)table;
    return SUCCESS;
}

int tinydb_create_index(Storage* storage, const char* sql) {
    (void)storage;
    (void)sql;
    return SUCCESS;
}

int tinydb_drop_index(Storage* storage, const char* table, const char* index_name) {
    (void)storage;
    (void)table;
    (void)index_name;
    return SUCCESS;
}

/* Connection management */
Connection* tinydb_connect(Storage* storage) {
    if (!storage) return NULL;

    Connection* conn = malloc(sizeof(Connection));
    if (!conn) return NULL;

    memset(conn, 0, sizeof(Connection));
    conn->storage = storage;
    conn->is_connected = 1;

    return conn;
}

void tinydb_disconnect(Connection* conn) {
    if (!conn) return;
    free(conn);
}

int tinydb_set_isolation(Connection* conn, IsolationLevel level) {
    (void)conn;
    (void)level;
    return SUCCESS;
}

/* Error handling */
Error* tinydb_error(Storage* storage) {
    (void)storage;
    return NULL;
}

void tinydb_error_clear(Storage* storage) {
    (void)storage;
}

/* Get catalog from storage (for executor access) */
Catalog* storage_get_catalog(Storage* storage) {
    if (!storage) return NULL;
    return storage->catalog;
}

/* Get pager from storage */
Pager* storage_get_pager(Storage* storage) {
    if (!storage) return NULL;
    return storage->pager;
}

/* Get page cache from storage */
PageCache* storage_get_cache(Storage* storage) {
    if (!storage) return NULL;
    return storage->cache;
}