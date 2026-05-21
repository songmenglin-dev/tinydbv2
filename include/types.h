#ifndef TINYDB_TYPES_H
#define TINYDB_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*============================================================================
 * Fixed-width integer types
 *============================================================================*/
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/*============================================================================
 * Boolean type
 *============================================================================*/
typedef int8_t   bool8;

/*============================================================================
 * Size type
 *============================================================================*/
typedef size_t   usize;

/*============================================================================
 * Page size and related constants
 *============================================================================*/
#define PAGE_SIZE 4096
#define PAGE_SIZE_MASK 0xFFF
#define PAGE_SIZE_SHIFT 12

/*============================================================================
 * Page number and row ID types
 *============================================================================*/
typedef uint32_t pagenum_t;
typedef uint32_t rowid_t;

/*============================================================================
 * Page limits
 *============================================================================*/
#define MAX_PAGE_NUMBER 0x7FFFFFFF
#define MAX_PAGES 0x7FFFFFFF

/*============================================================================
 * Page types stored in page header
 *============================================================================*/
#define PAGE_TYPE_HEADER          0x00
#define PAGE_TYPE_TABLE_LEAF      0x0D
#define PAGE_TYPE_TABLE_INTERNAL  0x0A
#define PAGE_TYPE_INDEX_LEAF      0x0A
#define PAGE_TYPE_INDEX_INTERNAL  0x02
#define PAGE_TYPE_FREELIST        0x0E
#define PAGE_TYPE_OVERFLOW        0x0F

/*============================================================================
 * Column type enumeration
 *============================================================================*/
typedef enum {
    COL_TYPE_INTEGER,
    COL_TYPE_FLOAT,
    COL_TYPE_TEXT,
    COL_TYPE_BLOB
} ColumnType;

/*============================================================================
 * SQL value type enumeration
 *============================================================================*/
typedef enum {
    VALUE_NULL = 0,
    VALUE_INTEGER = 1,
    VALUE_FLOAT = 2,
    VALUE_TEXT = 3,
    VALUE_BLOB = 4
} ValueType;

/*============================================================================
 * Isolation level for transactions
 *============================================================================*/
typedef enum {
    ISOLATION_READ_COMMITTED,
    ISOLATION_SERIALIZABLE
} IsolationLevel;

/*============================================================================
 * Common size constants
 *============================================================================*/
#define MAX_ROW_SIZE (PAGE_SIZE - 100)
#define MAX_COLUMNS 64
#define MAX_TABLE_NAME 64
#define MAX_COLUMN_NAME 64
#define MAX_INDEX_NAME 64

/*============================================================================
 * Database header magic number
 *============================================================================*/
#define HEADER_MAGIC 0x54494E59
#define CURRENT_FORMAT_VERSION 1

/*============================================================================
 * B+Tree constants
 *============================================================================*/
#define B_TREE_MAX_DEPTH 16

/*============================================================================
 * Buffer pool constants
 *============================================================================*/
#define BUFFER_POOL_SIZE 256

/*============================================================================
 * WAL constants
 *============================================================================*/
#define WAL_MAGIC 0x377F0682
#define WAL_FORMAT_VERSION 1
#define WAL_CHECKPOINT_THRESHOLD 1000
#define WAL_AUTOCHECKPOINT_ENABLED true

/*============================================================================
 * Protocol constants
 *============================================================================*/
#define PROTOCOL_VERSION 1
#define DEFAULT_SOCKET_PATH "/run/tinydb/tinydb.sock"
#define ALTERNATE_SOCKET_PATH "/var/run/tinydb/tinydb.sock"
#define MAX_SQL_LENGTH (1024 * 1024)
#define MAX_ROWS_IN_RESPONSE 10000
#define MAX_COLUMN_NAME_LENGTH 64
#define MAX_ERROR_MESSAGE_LENGTH 256
#define DEFAULT_TIMEOUT 30
#define QUERY_TIMEOUT 300
#define LOCK_TIMEOUT 10

#endif /* TINYDB_TYPES_H */
