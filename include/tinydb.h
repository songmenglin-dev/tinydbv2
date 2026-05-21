#ifndef TINYDB_H
#define TINYDB_H

#include "types.h"
#include "config.h"

/*============================================================================
 * Error codes
 *============================================================================*/
typedef enum {
    SUCCESS = 0,

    /* Parse errors (1xxx) */
    ERR_PARSE_SYNTAX = 1000,
    ERR_PARSE_UNEXPECTED_TOKEN,
    ERR_PARSE_UNTERMINATED_STRING,
    ERR_PARSE_INVALID_NUMBER,
    ERR_PARSE_INVALID_IDENTIFIER,

    /* Execution errors (2xxx) */
    ERR_EXEC_TABLE_NOT_FOUND = 2000,
    ERR_EXEC_COLUMN_NOT_FOUND,
    ERR_EXEC_DUPLICATE_COLUMN,
    ERR_EXEC_TYPE_MISMATCH,
    ERR_EXEC_NOT_NULL_VIOLATION,
    ERR_EXEC_CONSTRAINT_VIOLATION,
    ERR_EXEC_FOREIGN_KEY_VIOLATION,
    ERR_EXEC_UNIQUE_VIOLATION,

    /* Transaction errors (3xxx) */
    ERR_TX_ALREADY_ACTIVE = 3000,
    ERR_TX_NOT_ACTIVE,
    ERR_TX_LOCK_TIMEOUT,
    ERR_TX_DEADLOCK,
    ERR_TX_ROLLBACK,

    /* Storage errors (4xxx) */
    ERR_STORAGE_IO = 4000,
    ERR_STORAGE_CORRUPT,
    ERR_STORAGE_FULL,
    ERR_STORAGE_PERMISSION,
    ERR_STORAGE_NOT_FOUND,

    /* Protocol errors (5xxx) */
    ERR_PROTO_INVALID_REQUEST = 5000,
    ERR_PROTO_CONNECTION_CLOSED,
    ERR_PROTO_TIMEOUT,

    /* Internal errors (9xxx) */
    ERR_INTERNAL = 9000,
    ERR_OUT_OF_MEMORY,
    ERR_ASSERTION_FAILED
} ErrorCode;

/*============================================================================
 * Error structure
 *============================================================================*/
typedef struct {
    ErrorCode code;
    char message[256];
    char context[256];
    const char* file;
    int line;
} Error;

/*============================================================================
 * Public API
 *============================================================================*/

/* Storage handle - opaque pointer */
typedef struct Storage Storage;

/* Connection handle - opaque pointer */
typedef struct Connection Connection;

/* Database lifecycle */
int tinydb_open(Storage** storage, const char* path);
void tinydb_close(Storage* storage);

/* Transaction control */
int tinydb_begin(Storage* storage);
int tinydb_commit(Storage* storage);
int tinydb_rollback(Storage* storage);

/* DML operations */
int tinydb_insert(Storage* storage, const char* table, void* row);
int tinydb_update(Storage* storage, const char* table, void* row, void* where);
int tinydb_delete(Storage* storage, const char* table, void* where);

/* Query operations - results returned via callback */
typedef void (*ResultCallback)(void* data, int argc, char** argv, char** names);
int tinydb_select(Storage* storage, const char* table, void* where,
                  ResultCallback callback, void* data);

/* Schema operations */
int tinydb_create_table(Storage* storage, const char* sql);
int tinydb_drop_table(Storage* storage, const char* table);
int tinydb_create_index(Storage* storage, const char* sql);
int tinydb_drop_index(Storage* storage, const char* table, const char* index_name);

/* Connection management */
Connection* tinydb_connect(Storage* storage);
void tinydb_disconnect(Connection* conn);
int tinydb_set_isolation(Connection* conn, IsolationLevel level);

/* Error handling */
Error* tinydb_error(Storage* storage);
void tinydb_error_clear(Storage* storage);

#endif /* TINYDB_H */
