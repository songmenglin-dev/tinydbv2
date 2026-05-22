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

/* Get right sibling page number from leaf page */
static uint32_t get_right_sibling(void* page_data) {
    return read_u32(page_data, 6);
}

/* Set right sibling page number in leaf page */
static void set_right_sibling(void* page_data, uint32_t page_num) {
    write_u32(page_data, 6, page_num);
}

/* Get cell pointer at index - auto-detects page type from first byte */
static uint16_t get_cell_pointer(void* page_data, int index) {
    uint8_t page_type = read_u8(page_data, 0);
    int header_size = (page_type == BTREE_PAGE_TYPE_INTERNAL) ? BTREE_INTERNAL_HEADER_SIZE : BTREE_LEAF_HEADER_SIZE;
    off_t offset = header_size + index * BTREE_CELL_POINTER_SIZE;
    return read_u16(page_data, offset);
}

/* Set cell pointer at index - auto-detects page type from first byte */
static void set_cell_pointer(void* page_data, int index, uint16_t value) {
    uint8_t page_type = read_u8(page_data, 0);
    int header_size = (page_type == BTREE_PAGE_TYPE_INTERNAL) ? BTREE_INTERNAL_HEADER_SIZE : BTREE_LEAF_HEADER_SIZE;
    off_t offset = header_size + index * BTREE_CELL_POINTER_SIZE;
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
    write_u32(page->data, 6, 0);  /* right_sibling = 0 (none) */

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

            /* Find cell with key using binary search */
            int cell_count = header.cell_count;
            int found = -1;

            if (cell_count > 0) {
                int left = 0;
                int right = cell_count - 1;

                while (left <= right) {
                    int mid = left + (right - left) / 2;
                    off_t cell_offset = get_cell_pointer(page->data, mid);
                    uint32_t key_size = read_u32(page->data, cell_offset + 4);

                    if (key_size == sizeof(uint64_t)) {
                        uint64_t* cell_key = (uint64_t*)((uint8_t*)page->data + cell_offset + 8);
                        if (*cell_key == key) {
                            found = mid;
                            break;
                        } else if (*cell_key < key) {
                            left = mid + 1;
                        } else {
                            right = mid - 1;
                        }
                    } else {
                        /* Invalid key size, treat as not found */
                        break;
                    }
                }
            }

            if (found >= 0) {
                cursor->cell = found;
            } else {
                cursor->cell = cell_count;
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
                    uint64_t* cell_key = (uint64_t*)((uint8_t*)page->data + cell_offset + 6);
                    if (key < *cell_key) {
                        target_cell = i;
                        break;
                    }
                }
                target_cell = i + 1;
            }

            uint32_t child_page;
            if (target_cell < cell_count) {
                off_t cell_data_offset = get_cell_pointer(page->data, target_cell);
                child_page = read_u32(page->data, cell_data_offset);
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
                child_page = read_u32(page->data, cell_offset);  /* child_page at offset 0 of internal cell */
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
        /* Try to move to next page via right_sibling */
        uint32_t sibling = get_right_sibling(page->data);
        page_unpin(page);

        if (sibling != 0) {
            cursor->page = sibling;
            cursor->cell = 0;
            cursor->is_end = 0;
            return;
        }

        /* No sibling - mark end of iteration */
        cursor->is_end = 1;
        cursor->cell = cell_count;
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
    uint32_t value_len = payload_size - key_size;
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

/* Find the position where a key should be inserted in a leaf page */
static int find_insert_position(Page* page, uint64_t key) {
    BTreeNodeHeader header = get_node_header(page->data);
    int cell_count = header.cell_count;

    if (cell_count == 0) return 0;

    int left = 0;
    int right = cell_count;

    while (left < right) {
        int mid = left + (right - left) / 2;
        off_t cell_offset = get_cell_pointer(page->data, mid);
        uint32_t key_size = read_u32(page->data, cell_offset + 4);

        if (key_size == sizeof(uint64_t)) {
            uint64_t* cell_key = (uint64_t*)((uint8_t*)page->data + cell_offset + 8);
            if (*cell_key < key) {
                left = mid + 1;
            } else {
                right = mid;
            }
        } else {
            left = mid + 1;
        }
    }
    return left;
}

/* Write a cell at a given offset */
static void write_leaf_cell(void* page_data, off_t offset,
                            uint32_t payload_size, uint32_t key_size,
                            uint64_t key, const void* value) {
    write_u32(page_data, offset, payload_size);
    write_u32(page_data, offset + 4, key_size);
    memcpy((uint8_t*)page_data + offset + 8, &key, sizeof(uint64_t));
    if (value && payload_size > key_size) {
        memcpy((uint8_t*)page_data + offset + 8 + key_size, value, payload_size - key_size);
    }
}

/* Compact remaining cells in a page after removing one */
__attribute__((unused))
static int compact_page_cells(Page* page, int remaining_cells, int removed_idx) {
    BTreeNodeHeader header = get_node_header(page->data);
    uint16_t new_content_start = BTREE_LEAF_HEADER_SIZE + remaining_cells * BTREE_CELL_POINTER_SIZE;
    uint16_t new_data_start = PAGE_SIZE;

    for (int i = remaining_cells - 1; i >= 0; i--) {
        int src_idx = (i >= removed_idx) ? i + 1 : i;
        uint16_t src_ptr = get_cell_pointer(page->data, src_idx);

        if (src_ptr < BTREE_LEAF_HEADER_SIZE || src_ptr >= PAGE_SIZE) {
            return -1;
        }

        uint32_t payload_size = read_u32(page->data, src_ptr);
        uint32_t cell_size = payload_size + 8;

        if (cell_size > (uint32_t)new_data_start ||
            (uint32_t)(new_data_start - cell_size) < (uint32_t)new_content_start) {
            return -1;
        }

        new_data_start -= cell_size;
        memcpy((uint8_t*)page->data + new_data_start,
               (uint8_t*)page->data + src_ptr, cell_size);
        set_cell_pointer(page->data, i, new_data_start);
    }

    header.content_start = new_content_start;
    header.cell_count = remaining_cells;
    set_node_header(page->data, &header);
    return 0;
}

/* Split a leaf page that doesn't have enough space */
static int split_leaf_page(BTree* tree, Page* page, int cell_count,
                           uint32_t cell_size, int insert_idx,
                           BTreeCursor* cursor) {
    (void)cell_size;
    (void)insert_idx;
    uint32_t new_page_num;
    if (pager_allocate_page(tree->pager, &new_page_num) < 0) {
        return -1;
    }

    Page* new_page = page_pin(tree->cache, new_page_num);
    if (!new_page) {
        return -1;
    }

    memset(new_page->data, 0, PAGE_SIZE);
    write_u8(new_page->data, 0, BTREE_PAGE_TYPE_LEAF);
    write_u16(new_page->data, 1, 0);
    write_u16(new_page->data, 3, PAGE_SIZE);
    write_u8(new_page->data, 5, 0);
    page_mark_dirty(new_page);

    int original_count = cell_count;
    int cells_to_move = (original_count + 1) / 2;
    if (cells_to_move == 0) cells_to_move = 1;
    uint16_t new_content_start = PAGE_SIZE;

    for (int i = 0; i < cells_to_move; i++) {
        int src_idx = original_count - cells_to_move + i;
        if (src_idx >= 0 && src_idx < original_count) {
            uint16_t src_ptr = get_cell_pointer(page->data, src_idx);
            uint32_t payload_size = read_u32(page->data, src_ptr);
            uint32_t needed = payload_size + 8;
            if (needed > (uint32_t)new_content_start ||
                new_content_start < BTREE_LEAF_HEADER_SIZE + (uint16_t)needed) {
                page_unpin(new_page);
                return -1;
            }
            uint16_t new_ptr = (uint16_t)(new_content_start - payload_size - 8);
            new_content_start = new_ptr;
            memcpy((uint8_t*)new_page->data + new_ptr,
                   (uint8_t*)page->data + src_ptr, payload_size + 8);
            set_cell_pointer(new_page->data, i, new_ptr);
        }
    }

    BTreeNodeHeader new_header;
    new_header.page_type = BTREE_PAGE_TYPE_LEAF;
    new_header.cell_count = cells_to_move;
    new_header.content_start = new_content_start;
    new_header.fragmented_bytes = 0;
    set_node_header(new_page->data, &new_header);
    set_right_sibling(new_page->data, get_right_sibling(page->data));
    page_mark_dirty(new_page);

    int remaining_cells = original_count - cells_to_move;
    if (remaining_cells > 0) {
        uint16_t new_content_start = BTREE_LEAF_HEADER_SIZE + remaining_cells * BTREE_CELL_POINTER_SIZE;
        uint16_t new_data_start = PAGE_SIZE;

        for (int i = remaining_cells - 1; i >= 0; i--) {
            int src_idx = i;
            uint16_t src_ptr = get_cell_pointer(page->data, src_idx);

            if (src_ptr < BTREE_LEAF_HEADER_SIZE || src_ptr >= PAGE_SIZE) {
                page_unpin(new_page);
                return -1;
            }

            uint32_t payload_size = read_u32(page->data, src_ptr);
            uint32_t cell_size = payload_size + 8;

            if (cell_size > (uint32_t)new_data_start ||
                (uint32_t)(new_data_start - cell_size) < (uint32_t)new_content_start) {
                page_unpin(new_page);
                return -1;
            }

            new_data_start -= cell_size;
            memcpy((uint8_t*)page->data + new_data_start,
                   (uint8_t*)page->data + src_ptr, cell_size);
            set_cell_pointer(page->data, i, new_data_start);
        }
    }

    BTreeNodeHeader header;
    header.page_type = BTREE_PAGE_TYPE_LEAF;
    header.cell_count = remaining_cells;
    header.content_start = (remaining_cells > 0) ?
        (BTREE_LEAF_HEADER_SIZE + remaining_cells * BTREE_CELL_POINTER_SIZE) : PAGE_SIZE;
    header.fragmented_bytes = 0;
    set_node_header(page->data, &header);
    set_right_sibling(page->data, new_page_num);
    page_mark_dirty(page);
    page_unpin(new_page);

    page = page_pin(tree->cache, cursor->page);
    if (!page) {
        return -1;
    }
    return 0;
}

int btree_insert(BTree* tree, uint64_t key, const void* value, uint32_t len) {
    if (!tree || !tree->is_open) return -1;

    /* Find leaf page for key */
    BTreeCursor* cursor = btree_find(tree, key);
    if (!cursor) {
        return -1;
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
    uint32_t total_payload = sizeof(uint64_t) + len;  /* key + value */
    /* Check for overflow in cell_size calculation */
    if (total_payload > UINT32_MAX - 8) {
        page_unpin(page);
        btree_cursor_free(cursor);
        return -1;
    }
    uint32_t cell_size = 8 + total_payload;

    /* Check if we have space, if not, split the page */
    off_t content_start = header.content_start;
    off_t available_space = content_start - (BTREE_LEAF_HEADER_SIZE + cell_count * BTREE_CELL_POINTER_SIZE);

    if ((uint32_t)cell_size > (uint32_t)available_space) {
        if (split_leaf_page(tree, page, cell_count, cell_size, cursor->cell, cursor) < 0) {
            page_unpin(page);
            btree_cursor_free(cursor);
            return -1;
        }
        page_unpin(page);
        page = page_pin(tree->cache, cursor->page);
        if (!page) {
            btree_cursor_free(cursor);
            return -1;
        }
        header = get_node_header(page->data);
        content_start = header.content_start;
        cell_count = header.cell_count;
    }

    /* Make room for new cell */
    int insert_idx = find_insert_position(page, key);
    for (int i = cell_count - 1; i >= insert_idx; i--) {
        set_cell_pointer(page->data, i + 1, get_cell_pointer(page->data, i));
    }

    /* Write cell */
    uint16_t cell_offset = (uint16_t)(content_start - cell_size);
    set_cell_pointer(page->data, insert_idx, cell_offset);
    write_leaf_cell(page->data, cell_offset, total_payload, sizeof(uint64_t), key, value);

    /* Update header */
    header.cell_count++;
    header.content_start = cell_offset;
    set_node_header(page->data, &header);

    page_mark_dirty(page);
    page_unpin(page);
    btree_cursor_free(cursor);

    return 0;
}

/* Delete a key from the B+tree */
int btree_delete(BTree* tree, uint64_t key) {
    if (!tree || !tree->is_open) return -1;

    /* Find the leaf page containing the key */
    BTreeCursor* cursor = btree_find(tree, key);
    if (!cursor) {
        return -1;
    }

    /* Key not found */
    if (cursor->is_end) {
        btree_cursor_free(cursor);
        return -1;
    }

    Page* page = page_pin(tree->cache, cursor->page);
    if (!page) {
        btree_cursor_free(cursor);
        return -1;
    }

    BTreeNodeHeader header = get_node_header(page->data);
    int cell_count = header.cell_count;
    int delete_idx = cursor->cell;

    if (delete_idx < 0 || delete_idx >= cell_count) {
        page_unpin(page);
        btree_cursor_free(cursor);
        return -1;
    }

    /* Remove cell by shifting pointers */
    for (int i = delete_idx; i < cell_count - 1; i--) {
        set_cell_pointer(page->data, i, get_cell_pointer(page->data, i + 1));
    }

    header.cell_count--;
    set_node_header(page->data, &header);
    page_mark_dirty(page);

    /* Check if page is now underfull */
    int min_keys = BTREE_MIN_KEYS;
    if (header.cell_count < min_keys && header.cell_count > 0) {
        /* Page is underfull - need to borrow or merge */
        uint32_t right_sibling = get_right_sibling(page->data);
        /* Try right sibling first */
        if (right_sibling != 0) {
            Page* right_page = page_pin(tree->cache, right_sibling);
            if (right_page) {
                BTreeNodeHeader right_header = get_node_header(right_page->data);
                if (right_header.cell_count > min_keys) {
                    /* Borrow one cell from right sibling */
                    int right_count = right_header.cell_count;
                    off_t right_cell_offset = get_cell_pointer(right_page->data, 0);
                    uint32_t payload_size = read_u32(right_page->data, right_cell_offset);
                    uint32_t cell_size = payload_size + 8;

                    /* Check if we have space */
                    off_t content_start = header.content_start;
                    off_t available_space = content_start - (BTREE_LEAF_HEADER_SIZE + header.cell_count * BTREE_CELL_POINTER_SIZE);

                    if (cell_size <= (uint32_t)available_space) {
                        /* Shift existing cells to make room */
                        for (int i = header.cell_count - 1; i >= 0; i--) {
                            set_cell_pointer(page->data, i + 1, get_cell_pointer(page->data, i));
                        }

                        /* Copy first cell from right sibling */
                        uint16_t new_offset = (uint16_t)(content_start - cell_size);
                        memcpy((uint8_t*)page->data + new_offset,
                               (uint8_t*)right_page->data + right_cell_offset, cell_size);
                        set_cell_pointer(page->data, 0, new_offset);

                        /* Update both headers */
                        header.cell_count++;
                        header.content_start = new_offset;
                        set_node_header(page->data, &header);
                        page_mark_dirty(page);

                        /* Remove first cell from right sibling */
                        for (int i = 0; i < right_count - 1; i++) {
                            set_cell_pointer(right_page->data, i, get_cell_pointer(right_page->data, i + 1));
                        }
                        right_header.cell_count--;
                        set_node_header(right_page->data, &right_header);
                        page_mark_dirty(right_page);
                    }
                }
                page_unpin(right_page);
            }
        }
    }

    /* Handle empty root case */
    if (header.cell_count == 0) {
        /* Root is empty - could promote first child as new root, but for now just leave it */
    }

    page_unpin(page);
    btree_cursor_free(cursor);
    return 0;
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