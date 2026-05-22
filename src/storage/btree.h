#ifndef TINYDB_BTREE_H
#define TINYDB_BTREE_H

#include "../../include/tinydb.h"
#include "../../include/types.h"
#include "pager.h"
#include "page_cache.h"
#include <pthread.h>

/*============================================================================
 * B+Tree Implementation
 *============================================================================*/

/* B+Tree constants - derived from page size */
#define BTREE_MAX_DEPTH 16
#define BTREE_LEAF_HEADER_SIZE 10  /* type(1) + cell_count(2) + content_start(2) + fragmented(1) */
#define BTREE_INTERNAL_HEADER_SIZE 14  /* + right_child(4) */
#define BTREE_CELL_POINTER_SIZE 2
#define BTREE_MAX_KEY_SIZE 256

/* Calculate B+Tree order based on page size */
#define BTREE_ORDER ((PAGE_SIZE - BTREE_LEAF_HEADER_SIZE - BTREE_CELL_POINTER_SIZE) / \
                     (sizeof(uint32_t) + BTREE_MAX_KEY_SIZE + sizeof(uint32_t)))

/* Minimum keys in non-root node */
#define BTREE_MIN_KEYS ((BTREE_ORDER) / 2)

/* B+Tree page types */
#define BTREE_PAGE_TYPE_LEAF 0x0D
#define BTREE_PAGE_TYPE_INTERNAL 0x0A

/* B+Tree node header (10 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t page_type;         /* Page type (leaf or internal) */
    uint16_t cell_count;       /* Number of cells in this page */
    uint16_t content_start;    /* Offset to start of cell content area */
    uint8_t fragmented_bytes;  /* Number of fragmented free bytes */
} BTreeNodeHeader;

/* B+Tree leaf page header (14 bytes) - includes right sibling pointer */
typedef struct __attribute__((packed)) {
    uint8_t page_type;         /* Page type (leaf = 0x0D) */
    uint16_t cell_count;       /* Number of cells in this page */
    uint16_t content_start;    /* Offset to start of cell content area */
    uint8_t fragmented_bytes;  /* Number of fragmented free bytes */
    uint32_t right_sibling;    /* Page number of next leaf (0 if none) */
} BTreeLeafHeader;

/* B+Tree internal node header (14 bytes) - extends leaf header */
typedef struct __attribute__((packed)) {
    uint8_t page_type;         /* Page type (leaf or internal) */
    uint16_t cell_count;       /* Number of cells in this page */
    uint16_t content_start;    /* Offset to start of cell content area */
    uint8_t fragmented_bytes;  /* Number of fragmented free bytes */
    uint32_t right_child;      /* Rightmost child pointer (internal nodes only) */
} BTreeInternalHeader;

/* Leaf cell structure (variable size) */
typedef struct __attribute__((packed)) {
    uint32_t payload_size;     /* Size of entire payload */
    uint32_t key_size;         /* Size of key data */
    /* Followed by: key_data[key_size], value_data[payload_size - key_size - 8] */
} BTreeLeafCellHeader;

/* Internal cell structure (variable size) */
typedef struct __attribute__((packed)) {
    uint32_t child_page;       /* Child page number */
    uint16_t key_size;         /* Size of key data */
    /* Followed by: key_data[key_size] */
} BTreeInternalCellHeader;

/* B+Tree structure */
typedef struct BTree {
    Pager* pager;             /* Pager for I/O */
    PageCache* cache;          /* Page cache */

    uint32_t root_page;        /* Page number of root node */
    uint32_t page_count;       /* Total pages in tree */

    /* Thread safety */
    pthread_mutex_t mutex;

    /* Tree metadata */
    int is_open;
} BTree;

/* B+Tree cursor for iteration */
typedef struct BTreeCursor {
    BTree* tree;

    /* Current position */
    uint32_t page;            /* Current page number */
    int cell;                 /* Cell index within page (-1 = before first) */
    int is_end;               /* At end of tree */

    /* Traversal stack */
    int depth;                /* Current stack depth (0 = at root) */
    struct {
        uint32_t page;        /* Page number at this level */
        int cell;             /* Cell index at this level */
    } stack[BTREE_MAX_DEPTH];

    /* Iteration direction */
    int is_forward;           /* 1 = forward, 0 = backward */
} BTreeCursor;

/* B+Tree range scan structure */
typedef struct BTreeRange {
    BTree* tree;
    BTreeCursor* start;
    BTreeCursor* end;
    uint64_t start_key;
    uint64_t end_key;
} BTreeRange;

/*============================================================================
 * B+Tree lifecycle
 *============================================================================*/

/* Open an existing B+tree */
BTree* btree_open(Pager* pager, PageCache* cache, uint32_t root_page);

/* Create a new B+tree with specified root */
BTree* btree_create(Pager* pager, PageCache* cache);

/* Close B+tree and flush dirty pages */
int btree_close(BTree* tree);

/*============================================================================
 * B+Tree operations
 *============================================================================*/

/* Insert a key-value pair */
int btree_insert(BTree* tree, uint64_t key, const void* value, uint32_t len);

/* Delete a key */
int btree_delete(BTree* tree, uint64_t key);

/* Update value for a key (in-place) */
int btree_update(BTree* tree, uint64_t key, const void* value, uint32_t len);

/* Find a key, return cursor */
BTreeCursor* btree_find(BTree* tree, uint64_t key);

/* Get cursor to first key */
BTreeCursor* btree_first(BTree* tree);

/* Get cursor to last key */
BTreeCursor* btree_last(BTree* tree);

/*============================================================================
 * Cursor operations
 *============================================================================*/

/* Move cursor forward */
void btree_cursor_next(BTreeCursor* cursor);

/* Move cursor backward */
void btree_cursor_prev(BTreeCursor* cursor);

/* Check if cursor is at valid position */
int btree_cursor_valid(BTreeCursor* cursor);

/* Free cursor */
void btree_cursor_free(BTreeCursor* cursor);

/* Get current key-value (advances cursor) */
int btree_get(BTreeCursor* cursor, uint64_t* key, void* buf, uint32_t* len);

/* Peek at current key-value (without advancing) */
int btree_cursor_peek(BTreeCursor* cursor, uint64_t* key, void* buf, uint32_t* len);

/*============================================================================
 * Range scan
 *============================================================================*/

/* Create a range scan */
BTreeRange* btree_range_new(BTree* tree, uint64_t start_key, uint64_t end_key);

/* Free a range scan */
void btree_range_free(BTreeRange* range);

/*============================================================================
 * Utility functions
 *============================================================================*/

/* Get page count */
uint32_t btree_page_count(BTree* tree);

/* Verify tree structure */
int btree_verify(BTree* tree);

/* Print tree structure (debug) */
void btree_print(BTree* tree);

#endif /* TINYDB_BTREE_H */