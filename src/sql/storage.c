#include "../../include/tinydb.h"
#include "../util/error.h"
#include <stdlib.h>
#include <string.h>

/*============================================================================
 * Storage stub implementation
 * This provides minimal stubs for tinydb_* functions until full storage
 * is implemented in Phase 5.
 *============================================================================*/

/* Storage structure - opaque, stub for now */
struct Storage {
    int is_open;
    char* path;
};

/* Connection structure - opaque, stub for now */
struct Connection {
    Storage* storage;
    int is_connected;
};

int tinydb_open(Storage** storage, const char* path) {
    if (!storage) return ERR_INTERNAL;

    Storage* s = malloc(sizeof(Storage));
    if (!s) return ERR_OUT_OF_MEMORY;

    memset(s, 0, sizeof(Storage));
    s->is_open = 1;
    if (path) {
        s->path = malloc(strlen(path) + 1);
        strcpy(s->path, path);
    }

    *storage = s;
    return SUCCESS;
}

void tinydb_close(Storage* storage) {
    if (!storage) return;
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