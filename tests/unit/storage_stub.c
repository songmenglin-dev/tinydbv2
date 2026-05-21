/* Storage stubs for testing */
#include "../../include/tinydb.h"
#include <stdlib.h>
#include <string.h>

/* Minimal stub Storage - actual implementation is in storage.c */
struct Storage {
    int dummy;
};

/* Stub implementations - tests don't actually call these */
int tinydb_begin(Storage* storage) {
    (void)storage;
    return SUCCESS;
}

int tinydb_commit(Storage* storage) {
    (void)storage;
    return SUCCESS;
}

int tinydb_rollback(Storage* storage) {
    (void)storage;
    return SUCCESS;
}

int tinydb_open(Storage** storage, const char* path) {
    (void)storage;
    (void)path;
    return ERR_INTERNAL;
}

void tinydb_close(Storage* storage) {
    (void)storage;
}

int tinydb_insert(Storage* storage, const char* table, void* row) {
    (void)storage;
    (void)table;
    (void)row;
    return ERR_INTERNAL;
}

int tinydb_update(Storage* storage, const char* table, void* row, void* where) {
    (void)storage;
    (void)table;
    (void)row;
    (void)where;
    return ERR_INTERNAL;
}

int tinydb_delete(Storage* storage, const char* table, void* where) {
    (void)storage;
    (void)table;
    (void)where;
    return ERR_INTERNAL;
}

int tinydb_select(Storage* storage, const char* table, void* where,
                  ResultCallback callback, void* data) {
    (void)storage;
    (void)table;
    (void)where;
    (void)callback;
    (void)data;
    return ERR_INTERNAL;
}

int tinydb_create_table(Storage* storage, const char* sql) {
    (void)storage;
    (void)sql;
    return ERR_INTERNAL;
}

int tinydb_drop_table(Storage* storage, const char* table) {
    (void)storage;
    (void)table;
    return ERR_INTERNAL;
}

int tinydb_create_index(Storage* storage, const char* sql) {
    (void)storage;
    (void)sql;
    return ERR_INTERNAL;
}

int tinydb_drop_index(Storage* storage, const char* table, const char* index_name) {
    (void)storage;
    (void)table;
    (void)index_name;
    return ERR_INTERNAL;
}

Connection* tinydb_connect(Storage* storage) {
    (void)storage;
    return NULL;
}

void tinydb_disconnect(Connection* conn) {
    (void)conn;
}

int tinydb_set_isolation(Connection* conn, IsolationLevel level) {
    (void)conn;
    (void)level;
    return SUCCESS;
}

Error* tinydb_error(Storage* storage) {
    (void)storage;
    return NULL;
}

void tinydb_error_clear(Storage* storage) {
    (void)storage;
}