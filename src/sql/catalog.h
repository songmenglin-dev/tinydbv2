#ifndef TINYDB_CATALOG_H
#define TINYDB_CATALOG_H

#include "../../include/tinydb.h"
#include "../storage/btree.h"
#include "../storage/pager.h"
#include "../storage/page_cache.h"
#include <pthread.h>

/*============================================================================
 * Catalog Constants
 *============================================================================*/

/* Catalog table name */
#define CATALOG_TABLE_NAME "tinydb_master"

/* Catalog entry types */
typedef enum {
    CATALOG_TYPE_TABLE = 1,
    CATALOG_TYPE_INDEX = 2
} CatalogEntryType;

/*============================================================================
 * Catalog Entry (tinydb_master row)
 *============================================================================*/

typedef struct {
    CatalogEntryType type;      /* TABLE or INDEX */
    char name[64];             /* Object name */
    char tbl_name[64];         /* Table name (for indexes) */
    char sql[512];             /* CREATE statement */
} CatalogEntry;

/*============================================================================
 * Catalog Structure
 *============================================================================*/

typedef struct Catalog {
    BTree* tree;               /* B+Tree storing catalog */
    Pager* pager;              /* Pager reference */
    PageCache* cache;           /* Page cache reference */

    pthread_mutex_t mutex;
    int is_open;
} Catalog;

/*============================================================================
 * Catalog Lifecycle
 *============================================================================*/

/* Open catalog (creates if needed) */
Catalog* catalog_open(Pager* pager, PageCache* cache);

/* Close catalog */
void catalog_close(Catalog* catalog);

/* Initialize catalog with tinydb_master table */
int catalog_init(Catalog* catalog);

/*============================================================================
 * Catalog Operations
 *============================================================================*/

/* Insert entry into catalog */
int catalog_insert(Catalog* catalog, const CatalogEntry* entry);

/* Delete entry from catalog */
int catalog_delete(Catalog* catalog, CatalogEntryType type, const char* name);

/* Lookup entry by type and name */
CatalogEntry* catalog_lookup_type_name(Catalog* catalog,
                                       CatalogEntryType type,
                                       const char* name);

/* Get all tables */
CatalogEntry** catalog_get_tables(Catalog* catalog, int* count);

/* Get all indexes for a table */
CatalogEntry** catalog_get_indexes(Catalog* catalog, const char* table_name, int* count);

/* Free catalog entry array */
void catalog_free_entries(CatalogEntry** entries, int count);

/*============================================================================
 * Catalog Iteration
 *============================================================================*/

/* Cursor for iterating catalog entries */
typedef struct CatalogCursor {
    BTreeCursor* btree_cursor;
    int is_end;
} CatalogCursor;

/* Create cursor for catalog iteration */
CatalogCursor* catalog_cursor_create(Catalog* catalog);

/* Move cursor to next entry */
void catalog_cursor_next(CatalogCursor* cursor);

/* Get current entry */
CatalogEntry* catalog_cursor_get(CatalogCursor* cursor);

/* Check if cursor is at valid position */
int catalog_cursor_valid(CatalogCursor* cursor);

/* Free catalog cursor */
void catalog_cursor_free(CatalogCursor* cursor);

#endif /* TINYDB_CATALOG_H */