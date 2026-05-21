#ifndef TINYDB_SCHEMA_H
#define TINYDB_SCHEMA_H

#include "../../include/tinydb.h"
#include <pthread.h>
#include <stdbool.h>

/*============================================================================
 * Column Definition
 *============================================================================*/

typedef struct Column {
    char name[MAX_COLUMN_NAME];    /* Column name */
    ColumnType type;              /* Column type */
    int not_null;                 /* NOT NULL constraint */
    int primary_key;              /* PRIMARY KEY constraint */
    int auto_increment;           /* AUTOINCREMENT flag */
    char default_value[256];       /* Default value string */
} Column;

/*============================================================================
 * Schema Definition
 *============================================================================*/

typedef struct Schema {
    char table_name[MAX_TABLE_NAME];  /* Table name */
    Column* columns;                   /* Array of columns */
    int col_count;                     /* Number of columns */
    uint32_t root_page;                /* Root page of table B+Tree */
    uint64_t auto_increment;           /* Next AUTOINCREMENT value */
    int is_open;
} Schema;

/*============================================================================
 * Schema Lifecycle
 *============================================================================*/

/* Create schema from column definitions */
Schema* schema_create(const char* table_name, Column* columns, int col_count);

/* Destroy schema and free resources */
void schema_destroy(Schema* schema);

/* Clone schema */
Schema* schema_clone(const Schema* original);

/*============================================================================
 * Schema Serialization
 *============================================================================*/

/* Maximum serialized schema size */
#define SCHEMA_MAX_SIZE 8192

/* Serialize schema to buffer (caller provides buffer of SCHEMA_MAX_SIZE) */
int schema_serialize(const Schema* schema, void* buf, uint32_t* size);

/* Deserialize schema from buffer */
Schema* schema_deserialize(const void* buf, uint32_t size);

/*============================================================================
 * Schema Operations
 *============================================================================*/

/* Find column index by name (-1 if not found) */
int schema_column_index(const Schema* schema, const char* name);

/* Get column by name (NULL if not found) */
Column* schema_column(const Schema* schema, const char* name);

/* Validate row data against schema */
int schema_validate_row(const Schema* schema, const void* row_data, uint32_t size);

/* Get row size for this schema */
uint32_t schema_row_size(const Schema* schema);

/*============================================================================
 * Schema Utilities
 *============================================================================*/

/* Compare two schemas for equality */
int schema_equal(const Schema* a, const Schema* b);

/* Print schema (debug) */
void schema_print(const Schema* schema);

#endif /* TINYDB_SCHEMA_H */