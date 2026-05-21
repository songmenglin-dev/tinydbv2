#define _POSIX_C_SOURCE 200809L

#include "btree.h"
#include "../util/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*============================================================================
 * Constants
 *============================================================================*/

/* Node header is at start of page */
#define NODE_START 0

/* Empty tree has root page 0 as leaf */
#define EMPTY_TREE_ROOT 0

/*============================================================================
 * Helper functions
 *============================================================================*/

/* Read a byte from page data */
static uint8_t read_u8(void* page_data, off_t offset) {
    return ((uint8_t*)page_data)[offset];
}

/* Write a byte to page data */
static void write_u8(void* page_data, off_t offset, uint8_t value) {
    ((uint8_t*)page_data)[offset] = value;
}

/* Read 16-bit unsigned from page data (big-endian) */
static uint16_t read_u16(void* page_data, off_t offset) {
    uint8_t* data = (uint8_t*)page_data;
    return ((uint16_t)data[offset] << 8) | ((uint16_t)data[offset + 1]);
}

/* Write 16-bit unsigned to page data (big-endian) */
static void write_u16(void* page_data, off_t offset, uint16_t value) {
    uint8_t* data = (uint8_t*)page_data;
    data[offset] = (uint8_t)(value >> 8);
    data[offset + 1] = (uint8_t)(value & 0xFF);
}

/* Read 32-bit unsigned from page data (big-endian) */
static uint32_t read_u32(void* page_data, off_t offset) {
    uint8_t* data = (uint8_t*)page_data;
    return ((uint32_t)data[offset] << 24) |
           ((uint32_t)data[offset + 1] << 16) |
           ((uint32_t)data[offset + 2] << 8) |
           ((uint32_t)data[offset + 3]);
}

/* Write 32-bit unsigned to page data (big-endian) */
static void write_u32(void* page_data, off_t offset, uint32_t value) {
    uint8_t* data = (uint8_t*)page_data;
    data[offset] = (uint8_t)(value >> 24);
    data[offset + 1] = (uint8_t)(value >> 16);
    data[offset + 2] = (uint8_t)(value >> 8);
    data[offset + 3] = (uint8_t)(value & 0xFF);
}

/* Get node header from page data */
static BTreeNodeHeader get_node_header(void* page_data) {
    BTreeNodeHeader header;
    header.page_type = read_u8(page_data, 0);
    header.cell_count = read_u16(page_data, 1);
    header.content_start = read_u16(page_data, 3);
    header.fragmented_bytes = read_u8(page_data, 5);
    return header;
}

/* Set node header in page data */
static void set_node_header(void* page_data, BTreeNodeHeader* header) {
    write_u8(page_data, 0, header->page_type);
    write_u16(page_data, 1, header->cell_count);
    write_u16(page_data, 3, header->content_start);
    write_u8(page_data, 5, header->fragmented_bytes);
}

/* Get cell pointer at index */
static uint16_t get_cell_pointer(void* page_data, int index) {
    off_t offset = BTREE_LEAF_HEADER_SIZE + index * BTREE_CELL_POINTER_SIZE;
    return read_u16(page_data, offset);
}

/* Set cell pointer at index */
static void set_cell_pointer(void* page_data, int index, uint16_t value) {
    off_t offset = BTREE_LEAF_HEADER_SIZE + index * BTREE_CELL_POINTER_SIZE;
    write_u16(page_data, offset, value);
}

/* Read leaf cell header at offset */
__attribute__((unused))
static void get_leaf_cell(void* page_data, off_t offset,
                         uint32_t* payload_size, uint32_t* key_size,
                         void** key_data, void** value_data) {
    *payload_size = read_u32(page_data, offset);
    *key_size = read_u32(page_data, offset + 4);

    *key_data = (uint8_t*)page_data + offset + 8;
    *value_data = (uint8_t*)page_data + offset + 8 + *key_size;
}

/* Check if page is leaf type */
static int is_leaf_page(uint8_t page_type) {
    return page_type == BTREE_PAGE_TYPE_LEAF;
}

/* Check if page is internal type */
static int is_internal_page(uint8_t page_type) {
    return page_type == BTREE_PAGE_TYPE_INTERNAL;
}

/* Get right child from internal node */
static uint32_t get_right_child(void* page_data) {
    return read_u32(page_data, 6);
}

/* Set right child in internal node */
__attribute__((unused))
static void set_right_child(void* page_data, uint32_t child) {
    write_u32(page_data, 6, child);
}

/*============================================================================
 * B+Tree lifecycle
 *============================================================================*/

BTree* btree_open(Pager* pager, PageCache* cache, uint32_t root_page) {
    if (!pager || !cache) return NULL;

    BTree* tree = calloc(1, sizeof(BTree));
    if (!tree) return NULL;

    tree->pager = pager;
    tree->cache = cache;
    tree->root_page = root_page;
    tree->is_open = 1;

    pthread_mutex_init(&tree->mutex, NULL);

    return tree;
}

BTree* btree_create(Pager* pager, PageCache* cache) {
    /* Allocate a new page for root */
    uint32_t root_page;
    if (pager_allocate_page(pager, &root_page) < 0) {
        return NULL;
    }

    /* Initialize root as empty leaf page */
    Page* page = page_pin(cache, root_page);
    if (!page) {
        return NULL;
    }

    memset(page->data, 0, PAGE_SIZE);
    write_u8(page->data, 0, BTREE_PAGE_TYPE_LEAF);
    write_u16(page->data, 1, 0);  /* cell_count = 0 */
    write_u16(page->data, 3, PAGE_SIZE);  /* content_start = end of page */
    write_u8(page->data, 5, 0);  /* fragmented_bytes = 0 */

    page_mark_dirty(page);
    page_unpin(page);

    BTree* tree = btree_open(pager, cache, root_page);
    if (!tree) {
        return NULL;
    }

    return tree;
}

int btree_close(BTree* tree) {
    if (!tree) return -1;

    /* Flush all dirty pages */
    if (tree->cache) {
        page_cache_flush(tree->cache);
    }

    pthread_mutex_destroy(&tree->mutex);
    free(tree);

    return 0;
}

/*============================================================================
 * Search operations
 *============================================================================*/

/* Find cell index where key should be inserted/found */
__attribute__((unused))
static int find_cell_index(void* page_data, uint64_t key, int is_leaf) {
    BTreeNodeHeader header = get_node_header(page_data);
    int cell_count = header.cell_count;

    int left = 0;
    int right = cell_count;

    while (left < right) {
        int mid = (left + right) / 2;

        if (is_leaf) {
            off_t cell_offset = get_cell_pointer(page_data, mid);
            uint32_t key_size = read_u32(page_data, cell_offset + 4);
            (void)key_size;

            /* Read key value at this cell */
            void* key_data = (uint8_t*)page_data + cell_offset + 8;
            uint64_t cell_key = *(uint64_t*)key_data;

            if (cell_key < key) {
                left = mid + 1;
            } else {
                right = mid;
            }
        } else {
            /* Internal node - key at mid+1 position */
            uint32_t child = read_u32(page_data, BTREE_LEAF_HEADER_SIZE + mid * BTREE_CELL_POINTER_SIZE);
            (void)child;  /* We compare against stored key values */
            /* For now, simple comparison */
            uint16_t key_size = read_u16(page_data, BTREE_LEAF_HEADER_SIZE + mid * BTREE_CELL_POINTER_SIZE + 4);
            (void)key_size;
            /* Just do linear search for simplicity */
            left = mid + 1;
        }
    }

    return left;
}

/* Search for key in tree, return cursor */
BTreeCursor* btree_find(BTree* tree, uint64_t key) {
    if (!tree || !tree->is_open) return NULL;

    BTreeCursor* cursor = calloc(1, sizeof(BTreeCursor));
    if (!cursor) return NULL;

    cursor->tree = tree;
    cursor->depth = 0;
    cursor->is_forward = 1;
    cursor->is_end = 0;

    uint32_t page_num = tree->root_page;
    int depth = 0;

    /* Descend tree to find leaf */
    while (depth < BTREE_MAX_DEPTH) {
        Page* page = page_pin(tree->cache, page_num);
        if (!page) {
            free(cursor);
            return NULL;
        }

        BTreeNodeHeader header = get_node_header(page->data);

        if (is_leaf_page(header.page_type)) {
            /* Found leaf page */
            cursor->stack[depth].page = page_num;
            cursor->stack[depth].cell = 0;
            cursor->depth = depth + 1;
            cursor->page = page_num;

            /* Find cell with key */
            int cell_count = header.cell_count;
            int found = -1;

            for (int i = 0; i < cell_count; i++) {
                off_t cell_offset = get_cell_pointer(page->data, i);
                uint32_t key_size = read_u32(page->data, cell_offset + 4);

                if (key_size == sizeof(uint64_t)) {
                    uint64_t* cell_key = (uint64_t*)((uint8_t*)page->data + cell_offset + 8);
                    if (*cell_key == key) {
                        found = i;
                        break;
                    }
                }
            }

            if (found >= 0) {
                cursor->cell = found;
            } else {
                /* Key not found - position at where it would be */
                cursor->cell = cell_count;  /* End marker */
                cursor->is_end = 1;
            }

            page_unpin(page);
            return cursor;
        } else if (is_internal_page(header.page_type)) {
            /* Internal node - traverse to child */
            int cell_count = header.cell_count;
            uint32_t right_child = get_right_child(page->data);

            int target_cell = 0;
            for (int i = 0; i < cell_count; i++) {
                off_t cell_offset = get_cell_pointer(page->data, i);
                uint16_t key_size = read_u16(page->data, cell_offset + 4);

                if (key_size == sizeof(uint64_t)) {
                    uint64_t* cell_key = (uint64_t*)((uint8_t*)page->data + cell_offset + 8);
                    if (*cell_key >= key) {
                        target_cell = i;
                        break;
                    }
                }
                target_cell = i + 1;
            }

            uint32_t child_page;
            if (target_cell < cell_count) {
                child_page = read_u32(page->data, BTREE_LEAF_HEADER_SIZE + target_cell * BTREE_CELL_POINTER_SIZE + 2);
            } else {
                child_page = right_child;
            }

            cursor->stack[depth].page = page_num;
            cursor->stack[depth].cell = target_cell;

            page_unpin(page);
            page_num = child_page;
            depth++;
        } else {
            /* Unknown page type */
            page_unpin(page);
            free(cursor);
            return NULL;
        }
    }

    free(cursor);
    return NULL;
}

/* Get first key in tree */
BTreeCursor* btree_first(BTree* tree) {
    if (!tree || !tree->is_open) return NULL;

    BTreeCursor* cursor = calloc(1, sizeof(BTreeCursor));
    if (!cursor) return NULL;

    cursor->tree = tree;
    cursor->is_forward = 1;

    /* Descend to leftmost leaf */
    uint32_t page_num = tree->root_page;
    int depth = 0;

    while (depth < BTREE_MAX_DEPTH) {
        Page* page = page_pin(tree->cache, page_num);
        if (!page) {
            free(cursor);
            return NULL;
        }

        BTreeNodeHeader header = get_node_header(page->data);

        if (is_leaf_page(header.page_type)) {
            cursor->page = page_num;
            cursor->cell = 0;
            cursor->depth = depth + 1;
            cursor->is_end = (header.cell_count == 0);

            cursor->stack[depth].page = page_num;
            cursor->stack[depth].cell = 0;

            page_unpin(page);
            return cursor;
        } else if (is_internal_page(header.page_type)) {
            /* Go to leftmost child */
            int cell_count = header.cell_count;
            uint32_t right_child = get_right_child(page->data);

            uint32_t child_page;
            if (cell_count > 0) {
                off_t cell_offset = get_cell_pointer(page->data, 0);
                child_page = read_u32(page->data, cell_offset + 2);  /* child_page at offset 2 */
            } else {
                child_page = right_child;
            }

            cursor->stack[depth].page = page_num;
            cursor->stack[depth].cell = 0;

            page_unpin(page);
            page_num = child_page;
            depth++;
        } else {
            page_unpin(page);
            free(cursor);
            return NULL;
        }
    }

    free(cursor);
    return NULL;
}

/* Get last key in tree */
BTreeCursor* btree_last(BTree* tree) {
    if (!tree || !tree->is_open) return NULL;

    BTreeCursor* cursor = calloc(1, sizeof(BTreeCursor));
    if (!cursor) return NULL;

    cursor->tree = tree;
    cursor->is_forward = 0;

    /* Descend to rightmost leaf */
    uint32_t page_num = tree->root_page;
    int depth = 0;

    while (depth < BTREE_MAX_DEPTH) {
        Page* page = page_pin(tree->cache, page_num);
        if (!page) {
            free(cursor);
            return NULL;
        }

        BTreeNodeHeader header = get_node_header(page->data);

        if (is_leaf_page(header.page_type)) {
            cursor->page = page_num;
            cursor->cell = header.cell_count > 0 ? header.cell_count - 1 : 0;
            cursor->depth = depth + 1;
            cursor->is_end = (header.cell_count == 0);

            cursor->stack[depth].page = page_num;
            cursor->stack[depth].cell = cursor->cell;

            page_unpin(page);
            return cursor;
        } else if (is_internal_page(header.page_type)) {
            /* Go to rightmost child */
            uint32_t right_child = get_right_child(page->data);
            int cell_count = header.cell_count;

            cursor->stack[depth].page = page_num;
            cursor->stack[depth].cell = cell_count;

            page_unpin(page);
            page_num = right_child;
            depth++;
        } else {
            page_unpin(page);
            free(cursor);
            return NULL;
        }
    }

    free(cursor);
    return NULL;
}

/*============================================================================
 * Cursor operations
 *============================================================================*/

void btree_cursor_next(BTreeCursor* cursor) {
    if (!cursor || cursor->is_end) return;

    BTree* tree = cursor->tree;
    Page* page = page_pin(tree->cache, cursor->page);
    if (!page) return;

    BTreeNodeHeader header = get_node_header(page->data);
    int cell_count = header.cell_count;

    cursor->cell++;

    if (cursor->cell >= cell_count) {
        /* Move to next page - for now, just mark end */
        cursor->is_end = 1;
        cursor->cell = cell_count;
        page_unpin(page);
        return;
    }

    page_unpin(page);
}

void btree_cursor_prev(BTreeCursor* cursor) {
    if (!cursor || cursor->is_end) return;

    (void)cursor;  /* tree is unused but kept for future implementation */

    if (cursor->cell > 0) {
        cursor->cell--;
        return;
    }

    /* Need to move to previous page - for now, just go to start */
    cursor->cell = 0;
}

int btree_cursor_valid(BTreeCursor* cursor) {
    if (!cursor) return 0;
    if (cursor->is_end) return 0;
    return 1;
}

void btree_cursor_free(BTreeCursor* cursor) {
    if (cursor) {
        free(cursor);
    }
}

/*============================================================================
 * Data access
 *============================================================================*/

int btree_get(BTreeCursor* cursor, uint64_t* key, void* buf, uint32_t* len) {
    if (!cursor || !key || !buf || !len) return -1;
    if (cursor->is_end) return -1;

    BTree* tree = cursor->tree;
    Page* page = page_pin(tree->cache, cursor->page);
    if (!page) return -1;

    BTreeNodeHeader header = get_node_header(page->data);
    if (cursor->cell >= header.cell_count) {
        page_unpin(page);
        return -1;
    }

    off_t cell_offset = get_cell_pointer(page->data, cursor->cell);

    uint32_t payload_size = read_u32(page->data, cell_offset);
    uint32_t key_size = read_u32(page->data, cell_offset + 4);

    /* Copy key */
    memcpy(key, (uint8_t*)page->data + cell_offset + 8, sizeof(uint64_t));

    /* Copy value (remaining payload after key) */
    uint32_t value_len = payload_size - key_size - 8;
    if (value_len > 0) {
        memcpy(buf, (uint8_t*)page->data + cell_offset + 8 + key_size, value_len);
    }
    *len = value_len;

    page_unpin(page);
    return 0;
}

int btree_cursor_peek(BTreeCursor* cursor, uint64_t* key, void* buf, uint32_t* len) {
    return btree_get(cursor, key, buf, len);
}

/*============================================================================
 * Insertion (simplified - no node splitting)
 *============================================================================*/

int btree_insert(BTree* tree, uint64_t key, const void* value, uint32_t len) {
    if (!tree || !tree->is_open) return -1;

    /* Find leaf page for key */
    BTreeCursor* cursor = btree_find(tree, key);
    if (!cursor) return -1;

    if (!cursor->is_end) {
        /* Key exists - for now, just update */
        page_unpin(page_pin(tree->cache, cursor->page));
        /* TODO: implement proper update/insert */
        btree_cursor_free(cursor);
        return 0;
    }

    /* Get leaf page */
    Page* page = page_pin(tree->cache, cursor->page);
    if (!page) {
        btree_cursor_free(cursor);
        return -1;
    }

    BTreeNodeHeader header = get_node_header(page->data);
    int cell_count = header.cell_count;

    /* Calculate space needed */
    uint32_t total_payload = sizeof(uint64_t) + len + 8;  /* key + value + header */
    uint16_t cell_size = 2 + total_payload;  /* cell pointer + payload */

    /* Check if we have space */
    off_t content_start = header.content_start;
    off_t available_space = content_start - (BTREE_LEAF_HEADER_SIZE + cell_count * BTREE_CELL_POINTER_SIZE);

    if (cell_size > available_space) {
        /* Need to split - TODO: implement proper splitting */
        page_unpin(page);
        btree_cursor_free(cursor);
        return -1;
    }

    /* Make room for new cell */
    int insert_idx = cursor->cell;
    for (int i = cell_count; i > insert_idx; i--) {
        set_cell_pointer(page->data, i, get_cell_pointer(page->data, i - 1));
    }

    /* Write cell */
    uint16_t cell_offset = content_start - (uint16_t)total_payload;
    set_cell_pointer(page->data, insert_idx, cell_offset);

    write_u32(page->data, cell_offset, total_payload);
    write_u32(page->data, cell_offset + 4, sizeof(uint64_t));
    memcpy((uint8_t*)page->data + cell_offset + 8, &key, sizeof(uint64_t));
    memcpy((uint8_t*)page->data + cell_offset + 8 + sizeof(uint64_t), value, len);

    /* Update header */
    header.cell_count++;
    header.content_start = cell_offset;
    set_node_header(page->data, &header);

    page_mark_dirty(page);
    page_unpin(page);
    btree_cursor_free(cursor);

    return 0;
}

int btree_delete(BTree* tree, uint64_t key) {
    (void)tree;
    (void)key;
    /* TODO: implement delete */
    return -1;
}

int btree_update(BTree* tree, uint64_t key, const void* value, uint32_t len) {
    return btree_insert(tree, key, value, len);
}

/*============================================================================
 * Range scan
 *============================================================================*/

BTreeRange* btree_range_new(BTree* tree, uint64_t start_key, uint64_t end_key) {
    if (!tree) return NULL;

    BTreeRange* range = calloc(1, sizeof(BTreeRange));
    if (!range) return NULL;

    range->tree = tree;
    range->start_key = start_key;
    range->end_key = end_key;
    range->start = btree_find(tree, start_key);
    range->end = btree_find(tree, end_key);

    return range;
}

void btree_range_free(BTreeRange* range) {
    if (range) {
        if (range->start) btree_cursor_free(range->start);
        if (range->end) btree_cursor_free(range->end);
        free(range);
    }
}

/*============================================================================
 * Utility functions
 *============================================================================*/

uint32_t btree_page_count(BTree* tree) {
    if (!tree) return 0;
    return tree->page_count;
}

int btree_verify(BTree* tree) {
    (void)tree;
    /* TODO: implement verify */
    return 0;
}

void btree_print(BTree* tree) {
    if (!tree) return;
    printf("BTree: root=%u, pages=%u\n", tree->root_page, tree->page_count);
}