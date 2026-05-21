#define _POSIX_C_SOURCE 200809L

#include "pager.h"
#include "../util/error.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

/*============================================================================
 * Constants
 *============================================================================*/

#define HEADER_PAGE_NUM 0
#define PAGER_MAGIC 0x54494E59  /* 'TINY' */

/*============================================================================
 * Database Header Structure (48 bytes)
 *============================================================================*/
typedef struct __attribute__((packed)) {
    uint32_t magic;             /* Offset 0-3: Magic number 'TINY' */
    uint32_t version;           /* Offset 4-7: Page version */
    uint32_t format_version;    /* Offset 8-11: File format version */
    uint32_t reserved;          /* Offset 12-15: Reserved */
    uint32_t page_count;        /* Offset 16-19: Page count */
    uint32_t first_free_page;   /* Offset 20-23: First free page */
    uint32_t free_page_count;   /* Offset 24-27: Free page count */
    uint32_t schema_root;       /* Offset 28-31: Schema root page */
    uint32_t wal_frame_count;   /* Offset 32-35: WAL frame count */
    uint32_t wal_checksum_1;    /* Offset 36-39: WAL checksum part 1 */
    uint32_t wal_checksum_2;    /* Offset 40-43: WAL checksum part 2 */
    uint32_t database_size;     /* Offset 44-47: Database size in pages */
    uint8_t  reserved_space[PAGE_SIZE - 48];  /* Offset 48-4095: Reserved */
} DatabaseHeader;

#define HEADER_SIZE 48

/*============================================================================
 * Helper functions
 *============================================================================*/

/* Get offset for a page in the file */
static off_t get_page_offset(uint32_t page_num) {
    return (off_t)page_num * PAGE_SIZE;
}

/* Read exact number of bytes from file */
static int read_exact(int fd, void* buf, size_t count) {
    size_t total_read = 0;
    while (total_read < count) {
        ssize_t n = read(fd, (char*)buf + total_read, count - total_read);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;  /* EOF */
        total_read += (size_t)n;
    }
    return 0;
}

/* Write exact number of bytes to file */
static int write_exact(int fd, const void* buf, size_t count) {
    size_t total_written = 0;
    while (total_written < count) {
        ssize_t n = write(fd, (const char*)buf + total_written, count - total_written);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total_written += (size_t)n;
    }
    return 0;
}

/* Read header from file */
static int read_header(int fd, DatabaseHeader* header) {
    if (lseek(fd, 0, SEEK_SET) < 0) return -1;
    if (read_exact(fd, header, HEADER_SIZE) < 0) return -1;
    return 0;
}

/* Write header to file */
static int write_header(int fd, const DatabaseHeader* header) {
    if (lseek(fd, 0, SEEK_SET) < 0) return -1;
    if (write_exact(fd, header, HEADER_SIZE) < 0) return -1;
    return 0;
}

/* Initialize a new header */
static void init_header(DatabaseHeader* header) {
    memset(header, 0, sizeof(*header));
    header->magic = PAGER_MAGIC;
    header->version = 1;
    header->format_version = CURRENT_FORMAT_VERSION;
    header->page_count = 1;  /* Start with header page */
    header->first_free_page = 0;
    header->free_page_count = 0;
    header->schema_root = 0;
    header->wal_frame_count = 0;
    header->wal_checksum_1 = 0;
    header->wal_checksum_2 = 0;
    header->database_size = 1;
}

/*============================================================================
 * Pager lifecycle
 *============================================================================*/

Pager* pager_open(const char* path) {
    Pager* pager = calloc(1, sizeof(Pager));
    if (!pager) return NULL;

    pager->path = strdup(path);
    if (!pager->path) {
        free(pager);
        return NULL;
    }

    pager->fd = open(path, O_RDWR);
    if (pager->fd < 0) {
        /* Try read-only */
        pager->fd = open(path, O_RDONLY);
        if (pager->fd < 0) {
            free(pager->path);
            free(pager);
            return NULL;
        }
    }

    /* Get file size */
    struct stat st;
    if (fstat(pager->fd, &st) < 0) {
        close(pager->fd);
        free(pager->path);
        free(pager);
        return NULL;
    }
    pager->file_size = st.st_size;

    /* Read and validate header */
    DatabaseHeader header;
    if (read_header(pager->fd, &header) < 0) {
        close(pager->fd);
        free(pager->path);
        free(pager);
        return NULL;
    }

    if (header.magic != PAGER_MAGIC) {
        close(pager->fd);
        free(pager->path);
        free(pager);
        return NULL;
    }

    pager->page_count = header.page_count;
    pager->first_free_page = header.first_free_page;
    pager->free_page_count = header.free_page_count;
    pager->is_open = 1;

    pthread_mutex_init(&pager->mutex, NULL);

    return pager;
}

Pager* pager_create(const char* path) {
    Pager* pager = calloc(1, sizeof(Pager));
    if (!pager) return NULL;

    pager->path = strdup(path);
    if (!pager->path) {
        free(pager);
        return NULL;
    }

    /* Open with create/truncate */
    pager->fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (pager->fd < 0) {
        free(pager->path);
        free(pager);
        return NULL;
    }

    /* Initialize header */
    DatabaseHeader header;
    init_header(&header);

    if (write_header(pager->fd, &header) < 0) {
        close(pager->fd);
        free(pager->path);
        free(pager);
        return NULL;
    }

    /* Sync to ensure data is written */
    if (fsync(pager->fd) < 0) {
        close(pager->fd);
        free(pager->path);
        free(pager);
        return NULL;
    }

    pager->page_count = 1;
    pager->first_free_page = 0;
    pager->free_page_count = 0;
    pager->file_size = PAGE_SIZE;
    pager->is_open = 1;

    pthread_mutex_init(&pager->mutex, NULL);

    return pager;
}

void pager_close(Pager* pager) {
    if (!pager) return;

    if (pager->is_open) {
        /* Flush any pending writes */
        if (pager->file_size > 0) {
            fsync(pager->fd);
        }
        close(pager->fd);
    }

    pthread_mutex_destroy(&pager->mutex);

    free(pager->path);
    free(pager);
}

int pager_flush(Pager* pager) {
    if (!pager) return -1;
    return fsync(pager->fd);
}

void pager_stats(Pager* pager, PagerStats* stats) {
    if (!pager || !stats) return;

    pthread_mutex_lock(&pager->mutex);
    stats->page_count = pager->page_count;
    stats->first_free_page = pager->first_free_page;
    stats->free_page_count = pager->free_page_count;
    stats->file_size = pager->file_size;
    pthread_mutex_unlock(&pager->mutex);
}

/*============================================================================
 * Page I/O operations
 *============================================================================*/

int pager_read_page(Pager* pager, uint32_t page_num, void* buf) {
    if (!pager || !buf) return -1;
    if (page_num >= pager->page_count && page_num != 0) return -1;

    pthread_mutex_lock(&pager->mutex);

    off_t offset = get_page_offset(page_num);
    if (lseek(pager->fd, offset, SEEK_SET) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }

    if (read_exact(pager->fd, buf, PAGE_SIZE) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }

    pthread_mutex_unlock(&pager->mutex);
    return 0;
}

int pager_write_page(Pager* pager, uint32_t page_num, const void* buf) {
    if (!pager || !buf) return -1;

    pthread_mutex_lock(&pager->mutex);

    off_t offset = get_page_offset(page_num);
    if (lseek(pager->fd, offset, SEEK_SET) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }

    if (write_exact(pager->fd, buf, PAGE_SIZE) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }

    /* Update file_size if needed */
    off_t new_size = offset + PAGE_SIZE;
    if (new_size > pager->file_size) {
        pager->file_size = new_size;
    }

    pthread_mutex_unlock(&pager->mutex);
    return 0;
}

int pager_allocate_page(Pager* pager, uint32_t* page_num) {
    if (!pager || !page_num) return -1;

    pthread_mutex_lock(&pager->mutex);

    uint32_t new_page;

    /* Check freelist first */
    if (pager->first_free_page != 0) {
        /* There are free pages, reuse them */
        new_page = pager->first_free_page;

        /* Read the free page to get next free page */
        uint8_t buf[PAGE_SIZE];
        off_t offset = get_page_offset(new_page);
        if (lseek(pager->fd, offset, SEEK_SET) < 0) {
            pthread_mutex_unlock(&pager->mutex);
            return -1;
        }
        if (read_exact(pager->fd, buf, PAGE_SIZE) < 0) {
            pthread_mutex_unlock(&pager->mutex);
            return -1;
        }

        /* First 4 bytes of free page contain next free page number */
        pager->first_free_page = *(uint32_t*)buf;
        pager->free_page_count--;

        *page_num = new_page;
        pthread_mutex_unlock(&pager->mutex);
        return 0;
    }

    /* No free pages, allocate new one */
    new_page = pager->page_count;
    pager->page_count++;

    /* Update header */
    DatabaseHeader header;
    if (read_header(pager->fd, &header) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }
    header.page_count = pager->page_count;
    if (write_header(pager->fd, &header) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }

    /* Extend file if needed */
    off_t new_size = get_page_offset(new_page + 1);
    if (ftruncate(pager->fd, new_size) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }
    pager->file_size = new_size;

    *page_num = new_page;
    pthread_mutex_unlock(&pager->mutex);
    return 0;
}

int pager_free_page(Pager* pager, uint32_t page_num) {
    if (!pager) return -1;
    if (page_num == 0) return -1;  /* Cannot free header page */

    pthread_mutex_lock(&pager->mutex);

    /* Write page number of next free page at start of freed page */
    uint8_t buf[PAGE_SIZE];
    memset(buf, 0, PAGE_SIZE);
    *(uint32_t*)buf = pager->first_free_page;

    off_t offset = get_page_offset(page_num);
    if (lseek(pager->fd, offset, SEEK_SET) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }
    if (write_exact(pager->fd, buf, PAGE_SIZE) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }

    /* Update freelist head */
    pager->first_free_page = page_num;
    pager->free_page_count++;

    /* Update header */
    DatabaseHeader header;
    if (read_header(pager->fd, &header) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }
    header.first_free_page = pager->first_free_page;
    header.free_page_count = pager->free_page_count;
    if (write_header(pager->fd, &header) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }

    pthread_mutex_unlock(&pager->mutex);
    return 0;
}

int pager_sync(Pager* pager) {
    if (!pager) return -1;
    return fsync(pager->fd);
}

/*============================================================================
 * Header page operations
 *============================================================================*/

int pager_read_header(Pager* pager, void* buf) {
    return pager_read_page(pager, HEADER_PAGE_NUM, buf);
}

int pager_write_header(Pager* pager, const void* buf) {
    return pager_write_page(pager, HEADER_PAGE_NUM, buf);
}

int pager_validate_magic(Pager* pager) {
    if (!pager) return -1;

    DatabaseHeader header;
    if (read_header(pager->fd, &header) < 0) return -1;

    return (header.magic == PAGER_MAGIC) ? 0 : -1;
}

int pager_update_page_count(Pager* pager, uint32_t count) {
    if (!pager) return -1;

    pthread_mutex_lock(&pager->mutex);

    DatabaseHeader header;
    if (read_header(pager->fd, &header) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }
    header.page_count = count;
    pager->page_count = count;

    if (write_header(pager->fd, &header) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }

    pthread_mutex_unlock(&pager->mutex);
    return 0;
}

int pager_update_freelist(Pager* pager, uint32_t first_free, uint32_t count) {
    if (!pager) return -1;

    pthread_mutex_lock(&pager->mutex);

    DatabaseHeader header;
    if (read_header(pager->fd, &header) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }
    header.first_free_page = first_free;
    header.free_page_count = count;
    pager->first_free_page = first_free;
    pager->free_page_count = count;

    if (write_header(pager->fd, &header) < 0) {
        pthread_mutex_unlock(&pager->mutex);
        return -1;
    }

    pthread_mutex_unlock(&pager->mutex);
    return 0;
}

/*============================================================================
 * Utility functions
 *============================================================================*/

int pager_is_valid_page(Pager* pager, uint32_t page_num) {
    if (!pager) return 0;
    return page_num < pager->page_count || page_num == 0;
}

off_t pager_get_page_offset(uint32_t page_num) {
    return get_page_offset(page_num);
}

int pager_truncate(Pager* pager, off_t size) {
    if (!pager) return -1;
    int ret = ftruncate(pager->fd, size);
    if (ret == 0) {
        pager->file_size = size;
    }
    return ret;
}