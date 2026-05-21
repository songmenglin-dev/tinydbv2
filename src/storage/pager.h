#ifndef TINYDB_PAGER_H
#define TINYDB_PAGER_H

#include "../../include/tinydb.h"
#include "../../include/types.h"
#include <sys/types.h>
#include <pthread.h>

/*============================================================================
 * Pager Layer - File I/O and Page Management
 *============================================================================*/

#define PAGER_MAX_PAGES 65536

/* Pager structure - manages database file I/O */
typedef struct Pager {
    int fd;                     /* File descriptor */
    char* path;                 /* Database file path */
    uint32_t page_count;        /* Total pages in file */
    uint32_t first_free_page;   /* Head of freelist */
    uint32_t free_page_count;   /* Number of free pages */

    /* File size tracking */
    off_t file_size;

    /* Thread safety */
    pthread_mutex_t mutex;

    /* File is open */
    int is_open;
} Pager;

/* Pager statistics */
typedef struct PagerStats {
    uint32_t page_count;
    uint32_t first_free_page;
    uint32_t free_page_count;
    off_t file_size;
} PagerStats;

/*============================================================================
 * Pager lifecycle
 *============================================================================*/

/* Open an existing database file */
Pager* pager_open(const char* path);

/* Create a new database file */
Pager* pager_create(const char* path);

/* Close the pager and release resources */
void pager_close(Pager* pager);

/* Flush all cached pages to disk */
int pager_flush(Pager* pager);

/* Get pager statistics */
void pager_stats(Pager* pager, PagerStats* stats);

/*============================================================================
 * Page I/O operations
 *============================================================================*/

/* Read a page into the provided buffer (caller allocates) */
int pager_read_page(Pager* pager, uint32_t page_num, void* buf);

/* Write a page from the provided buffer */
int pager_write_page(Pager* pager, uint32_t page_num, const void* buf);

/* Allocate a new page (returns page number) */
int pager_allocate_page(Pager* pager, uint32_t* page_num);

/* Free a page (add to freelist) */
int pager_free_page(Pager* pager, uint32_t page_num);

/* Sync file to disk */
int pager_sync(Pager* pager);

/*============================================================================
 * Header page operations
 *============================================================================*/

/* Read header page (page 0) */
int pager_read_header(Pager* pager, void* buf);

/* Write header page (page 0) */
int pager_write_header(Pager* pager, const void* buf);

/* Validate database magic number */
int pager_validate_magic(Pager* pager);

/* Update page count in header */
int pager_update_page_count(Pager* pager, uint32_t count);

/* Update freelist in header */
int pager_update_freelist(Pager* pager, uint32_t first_free, uint32_t count);

/*============================================================================
 * Utility functions
 *============================================================================*/

/* Check if a page number is valid */
int pager_is_valid_page(Pager* pager, uint32_t page_num);

/* Get page offset in file */
off_t pager_get_page_offset(uint32_t page_num);

/* Truncate file to specified size */
int pager_truncate(Pager* pager, off_t size);

#endif /* TINYDB_PAGER_H */