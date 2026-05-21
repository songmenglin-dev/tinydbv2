#define _POSIX_C_SOURCE 200809L

#include "schema.h"
#include "../util/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*============================================================================
 * Column Helpers
 *============================================================================*/

static uint32_t column_size(ColumnType type) {
    switch (type) {
        case COL_TYPE_INTEGER: return 8;
        case COL_TYPE_FLOAT: return 8;
        case COL_TYPE_TEXT: return PAGE_SIZE;  /* Max text size */
        case COL_TYPE_BLOB: return PAGE_SIZE; /* Max blob size */
        default: return 0;
    }
}

static const char* column_type_name(ColumnType type) {
    switch (type) {
        case COL_TYPE_INTEGER: return "INTEGER";
        case COL_TYPE_FLOAT: return "FLOAT";
        case COL_TYPE_TEXT: return "TEXT";
        case COL_TYPE_BLOB: return "BLOB";
        default: return "UNKNOWN";
    }
}

/*============================================================================
 * Schema Lifecycle
 *============================================================================*/

Schema* schema_create(const char* table_name, Column* columns, int col_count) {
    if (!table_name || col_count <= 0 || !columns) return NULL;

    Schema* schema = calloc(1, sizeof(Schema));
    if (!schema) return NULL;

    strncpy(schema->table_name, table_name, MAX_TABLE_NAME - 1);
    schema->col_count = col_count;

    /* Clone columns array */
    schema->columns = calloc(col_count, sizeof(Column));
    if (!schema->columns) {
        free(schema);
        return NULL;
    }

    for (int i = 0; i < col_count; i++) {
        memcpy(&schema->columns[i], &columns[i], sizeof(Column));
    }

    schema->auto_increment = 1;
    schema->is_open = 1;

    return schema;
}

void schema_destroy(Schema* schema) {
    if (!schema) return;
    if (schema->columns) {
        free(schema->columns);
    }
    free(schema);
}

Schema* schema_clone(const Schema* original) {
    if (!original) return NULL;
    return schema_create(original->table_name, original->columns, original->col_count);
}

/*============================================================================
 * Schema Serialization
 *============================================================================*/

/* Serialization format:
 * [4 bytes: magic]
 * [4 bytes: version]
 * [64 bytes: table_name]
 * [4 bytes: col_count]
 * [4 bytes: auto_increment high]
 * [4 bytes: auto_increment low]
 * Per column:
 *   [64 bytes: name]
 *   [4 bytes: type]
 *   [4 bytes: not_null]
 *   [4 bytes: primary_key]
 *   [4 bytes: auto_increment]
 *   [256 bytes: default_value]
 */

#define SCHEMA_MAGIC 0x5343484D  /* 'SCHM' */
#define SCHEMA_VERSION 1

int schema_serialize(const Schema* schema, void* buf, uint32_t* size) {
    if (!schema || !buf || !size) return ERR_INTERNAL;

    uint8_t* data = (uint8_t*)buf;
    uint32_t offset = 0;

    /* Magic */
    *(uint32_t*)(data + offset) = SCHEMA_MAGIC;
    offset += 4;

    /* Version */
    *(uint32_t*)(data + offset) = SCHEMA_VERSION;
    offset += 4;

    /* Table name */
    memset(data + offset, 0, 64);
    strncpy((char*)(data + offset), schema->table_name, 63);
    offset += 64;

    /* Column count */
    *(uint32_t*)(data + offset) = (uint32_t)schema->col_count;
    offset += 4;

    /* Auto increment (upper and lower 32 bits) */
    *(uint32_t*)(data + offset) = (uint32_t)(schema->auto_increment >> 32);
    offset += 4;
    *(uint32_t*)(data + offset) = (uint32_t)(schema->auto_increment & 0xFFFFFFFF);
    offset += 4;

    /* Columns */
    for (int i = 0; i < schema->col_count; i++) {
        Column* col = &schema->columns[i];

        /* Name */
        memset(data + offset, 0, 64);
        strncpy((char*)(data + offset), col->name, 63);
        offset += 64;

        /* Type */
        *(uint32_t*)(data + offset) = (uint32_t)col->type;
        offset += 4;

        /* Not null */
        *(uint32_t*)(data + offset) = col->not_null ? 1 : 0;
        offset += 4;

        /* Primary key */
        *(uint32_t*)(data + offset) = col->primary_key ? 1 : 0;
        offset += 4;

        /* Auto increment */
        *(uint32_t*)(data + offset) = col->auto_increment ? 1 : 0;
        offset += 4;

        /* Default value */
        memset(data + offset, 0, 256);
        if (col->default_value[0]) {
            strncpy((char*)(data + offset), col->default_value, 255);
        }
        offset += 256;
    }

    *size = offset;
    return SUCCESS;
}

Schema* schema_deserialize(const void* buf, uint32_t size) {
    if (!buf || size < 148) return NULL;  /* Minimum size */

    const uint8_t* data = (const uint8_t*)buf;
    uint32_t offset = 0;

    /* Magic check */
    if (*(uint32_t*)(data + offset) != SCHEMA_MAGIC) {
        return NULL;
    }
    offset += 4;

    /* Version check */
    if (*(uint32_t*)(data + offset) != SCHEMA_VERSION) {
        return NULL;
    }
    offset += 4;

    /* Table name */
    char table_name[64];
    memset(table_name, 0, 64);
    strncpy(table_name, (const char*)(data + offset), 63);
    offset += 64;

    /* Column count */
    uint32_t col_count = *(uint32_t*)(data + offset);
    offset += 4;

    /* Sanity check */
    if (col_count > MAX_COLUMNS) {
        return NULL;
    }

    /* Auto increment */
    uint64_t auto_inc = 0;
    auto_inc |= (uint64_t)*(uint32_t*)(data + offset) << 32;
    offset += 4;
    auto_inc |= *(uint32_t*)(data + offset);
    offset += 4;

    /* Allocate columns */
    Column* columns = calloc(col_count, sizeof(Column));
    if (!columns) return NULL;

    /* Read columns */
    for (uint32_t i = 0; i < col_count; i++) {
        Column* col = &columns[i];

        /* Name */
        memset(col->name, 0, MAX_COLUMN_NAME);
        strncpy(col->name, (const char*)(data + offset), MAX_COLUMN_NAME - 1);
        offset += 64;

        /* Type */
        col->type = (ColumnType)*(uint32_t*)(data + offset);
        offset += 4;

        /* Not null */
        col->not_null = *(uint32_t*)(data + offset) ? 1 : 0;
        offset += 4;

        /* Primary key */
        col->primary_key = *(uint32_t*)(data + offset) ? 1 : 0;
        offset += 4;

        /* Auto increment */
        col->auto_increment = *(uint32_t*)(data + offset) ? 1 : 0;
        offset += 4;

        /* Default value */
        memset(col->default_value, 0, 256);
        strncpy(col->default_value, (const char*)(data + offset), 255);
        offset += 256;
    }

    Schema* schema = schema_create(table_name, columns, col_count);
    free(columns);

    if (schema) {
        schema->auto_increment = auto_inc;
    }

    return schema;
}

/*============================================================================
 * Schema Operations
 *============================================================================*/

int schema_column_index(const Schema* schema, const char* name) {
    if (!schema || !name) return -1;

    for (int i = 0; i < schema->col_count; i++) {
        if (strcmp(schema->columns[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

Column* schema_column(const Schema* schema, const char* name) {
    if (!schema) return NULL;
    int idx = schema_column_index(schema, name);
    return (idx >= 0) ? &schema->columns[idx] : NULL;
}

int schema_validate_row(const Schema* schema, const void* row_data, uint32_t size) {
    (void)schema;
    (void)row_data;
    (void)size;
    /* TODO: Implement row validation */
    return SUCCESS;
}

uint32_t schema_row_size(const Schema* schema) {
    if (!schema) return 0;

    uint32_t total = 0;
    for (int i = 0; i < schema->col_count; i++) {
        total += column_size(schema->columns[i].type);
    }
    return total;
}

/*============================================================================
 * Schema Utilities
 *============================================================================*/

int schema_equal(const Schema* a, const Schema* b) {
    if (!a || !b) return 0;
    if (strcmp(a->table_name, b->table_name) != 0) return 0;
    if (a->col_count != b->col_count) return 0;

    for (int i = 0; i < a->col_count; i++) {
        Column* ca = &a->columns[i];
        Column* cb = &b->columns[i];
        if (strcmp(ca->name, cb->name) != 0) return 0;
        if (ca->type != cb->type) return 0;
        if (ca->not_null != cb->not_null) return 0;
        if (ca->primary_key != cb->primary_key) return 0;
    }
    return 1;
}

void schema_print(const Schema* schema) {
    if (!schema) return;

    printf("Schema: %s (columns: %d, auto_inc: %lu)\n",
           schema->table_name, schema->col_count, (unsigned long)schema->auto_increment);
    for (int i = 0; i < schema->col_count; i++) {
        Column* col = &schema->columns[i];
        printf("  %d: %s %s%s%s%s\n",
               i, col->name, column_type_name(col->type),
               col->not_null ? " NOT NULL" : "",
               col->primary_key ? " PRIMARY KEY" : "",
               col->auto_increment ? " AUTOINCREMENT" : "");
    }
}