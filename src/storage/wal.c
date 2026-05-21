#define _POSIX_C_SOURCE 200809L

#include "wal.h"
#include "pager.h"
#include "page_cache.h"
#include "../util/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/*============================================================================
 * Constants
 *============================================================================*/

/* WAL file header (32 bytes) */
typedef struct __attribute__((packed)) {
    uint32_t magic;             /* WAL_MAGIC */
    uint32_t version;           /* WAL_FORMAT_VERSION */
    uint64_t start_offset;      /* Start of entries */
    uint64_t end_offset;        /* End of entries */
    uint32_t frame_count;       /* Total frames */
    uint32_t checksum_1;        /* Header checksum part 1 */
    uint32_t checksum_2;        /* Header checksum part 2 */
    uint8_t reserved[4];        /* Reserved */
} WALHeader;

#define WAL_HEADER_SIZE 32

/*============================================================================
 * Helper Functions
 *============================================================================*/

/* Read exact bytes from file */
static int read_exact(int fd, void* buf, size_t count) {
    size_t total_read = 0;
    while (total_read < count) {
        ssize_t n = read(fd, (char*)buf + total_read, count - total_read);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        total_read += (size_t)n;
    }
    return 0;
}

/* Write exact bytes to file */
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

/* Calculate simple checksum for WAL data */
uint32_t wal_checksum(const void* data, size_t len) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum = (sum << 5) + sum + bytes[i];
    }
    return sum;
}

/* Get WAL file path from database path */
char* wal_get_path(const char* db_path) {
    if (!db_path) return NULL;

    size_t db_len = strlen(db_path);
    char* wal_path = malloc(db_len + 5); /* .wal + null */
    if (!wal_path) return NULL;

    strcpy(wal_path, db_path);
    strcat(wal_path, "-wal");
    return wal_path;
}

/* Verify WAL magic number */
int wal_verify_magic(int fd) {
    WALHeader header;
    if (lseek(fd, 0, SEEK_SET) < 0) return -1;
    if (read_exact(fd, &header, sizeof(header)) < 0) return -1;
    return (header.magic == WAL_MAGIC) ? 0 : -1;
}

/*============================================================================
 * WAL Lifecycle
 *============================================================================*/

WAL* wal_open(const char* db_path) {
    if (!db_path) return NULL;

    WAL* wal = calloc(1, sizeof(WAL));
    if (!wal) return NULL;

    wal->db_path = strdup(db_path);
    if (!wal->db_path) {
        free(wal);
        return NULL;
    }

    wal->path = wal_get_path(db_path);
    if (!wal->path) {
        free(wal->db_path);
        free(wal);
        return NULL;
    }

    /* Try to open existing WAL, or create new one */
    wal->fd = open(wal->path, O_RDWR);
    if (wal->fd < 0) {
        /* Create new WAL */
        wal->fd = open(wal->path, O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (wal->fd < 0) {
            free(wal->path);
            free(wal->db_path);
            free(wal);
            return NULL;
        }

        /* Write WAL header */
        WALHeader header;
        memset(&header, 0, sizeof(header));
        header.magic = WAL_MAGIC;
        header.version = WAL_FORMAT_VERSION;
        header.start_offset = WAL_HEADER_SIZE;
        header.end_offset = WAL_HEADER_SIZE;

        if (write_exact(wal->fd, &header, sizeof(header)) < 0) {
            close(wal->fd);
            unlink(wal->path);
            free(wal->path);
            free(wal->db_path);
            free(wal);
            return NULL;
        }

        wal->log_offset = WAL_HEADER_SIZE;
    } else {
        /* Read existing WAL header */
        WALHeader header;
        if (read_exact(wal->fd, &header, sizeof(header)) < 0) {
            close(wal->fd);
            free(wal->path);
            free(wal->db_path);
            free(wal);
            return NULL;
        }

        if (header.magic != WAL_MAGIC) {
            close(wal->fd);
            free(wal->path);
            free(wal->db_path);
            free(wal);
            return NULL;
        }

        wal->log_offset = header.end_offset;
        wal->frame_count = header.frame_count;
    }

    /* Get file size */
    struct stat st;
    if (fstat(wal->fd, &st) < 0) {
        close(wal->fd);
        free(wal->path);
        free(wal->db_path);
        free(wal);
        return NULL;
    }
    wal->file_size = st.st_size;

    /* Initialize checkpoint threshold */
    wal->checkpoint_threshold = WAL_AUTOCHECKPOINT_THRESHOLD;

    pthread_mutex_init(&wal->mutex, NULL);
    wal->is_open = 1;

    return wal;
}

void wal_close(WAL* wal) {
    if (!wal) return;

    if (wal->is_open && wal->fd >= 0) {
        /* Flush WAL before closing */
        fsync(wal->fd);
        close(wal->fd);
    }

    pthread_mutex_destroy(&wal->mutex);

    free(wal->path);
    free(wal->db_path);
    free(wal);
}

int wal_flush(WAL* wal) {
    if (!wal) return ERR_INTERNAL;
    if (wal->fd < 0) return ERR_STORAGE_IO;

    if (fsync(wal->fd) < 0) {
        return ERR_STORAGE_IO;
    }
    return SUCCESS;
}

/*============================================================================
 * WAL Entry Operations
 *============================================================================*/

/* Append an entry to WAL */
int wal_append(WAL* wal, WALEntryType type, uint64_t tx_id,
               const void* payload, uint32_t size) {
    if (!wal || !wal->is_open) return ERR_INTERNAL;

    pthread_mutex_lock(&wal->mutex);

    /* Build entry header */
    WALEntryHeader header;
    memset(&header, 0, sizeof(header));
    header.type = (uint8_t)type;
    header.flags = 0;
    header.size = size;
    header.tx_id = tx_id;

    /* Calculate checksum over header + payload */
    uint32_t checksum = wal_checksum(&header, sizeof(header));
    if (payload && size > 0) {
        checksum ^= wal_checksum(payload, size);
    }
    header.checksum = checksum;

    /* Write header */
    if (write_exact(wal->fd, &header, sizeof(header)) < 0) {
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }

    /* Write payload if present */
    if (payload && size > 0) {
        if (write_exact(wal->fd, payload, size) < 0) {
            pthread_mutex_unlock(&wal->mutex);
            return ERR_STORAGE_IO;
        }
    }

    /* Update offset */
    wal->log_offset += sizeof(header) + size;

    /* Update file size */
    struct stat st;
    if (fstat(wal->fd, &st) >= 0) {
        wal->file_size = st.st_size;
    }

    pthread_mutex_unlock(&wal->mutex);
    return SUCCESS;
}

int wal_begin_tx(WAL* wal) {
    if (!wal) return ERR_INTERNAL;
    wal->current_tx_id++;
    return wal_append(wal, WAL_ENTRY_BEGIN, wal->current_tx_id, NULL, 0);
}

int wal_commit_tx(WAL* wal) {
    if (!wal) return ERR_INTERNAL;
    return wal_append(wal, WAL_ENTRY_COMMIT, wal->current_tx_id, NULL, 0);
}

int wal_rollback_tx(WAL* wal) {
    if (!wal) return ERR_INTERNAL;
    return wal_append(wal, WAL_ENTRY_ROLLBACK, wal->current_tx_id, NULL, 0);
}

int wal_write_page(WAL* wal, uint64_t tx_id, uint32_t page_num, const void* page_data) {
    if (!wal || !page_data) return ERR_INTERNAL;

    /* Frame header + page data */
    char frame[WAL_FRAME_HEADER_SIZE + PAGE_SIZE];

    /* Frame header: page number and size */
    *(uint32_t*)(frame + 0) = page_num;
    *(uint32_t*)(frame + 4) = PAGE_SIZE;

    (void)tx_id;
    /* Copy page data */
    memcpy(frame + WAL_FRAME_HEADER_SIZE, page_data, PAGE_SIZE);

    /* Calculate frame checksum */
    uint32_t checksum = wal_checksum(frame, WAL_FRAME_HEADER_SIZE + PAGE_SIZE);
    *(uint32_t*)(frame + 8) = checksum;
    *(uint32_t*)(frame + 12) = checksum ^ 0xFFFFFFFF;

    pthread_mutex_lock(&wal->mutex);

    /* Write frame */
    if (write_exact(wal->fd, frame, sizeof(frame)) < 0) {
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }

    wal->log_offset += sizeof(frame);
    wal->frame_count++;

    /* Update file size */
    struct stat st;
    if (fstat(wal->fd, &st) >= 0) {
        wal->file_size = st.st_size;
    }

    pthread_mutex_unlock(&wal->mutex);
    return SUCCESS;
}

/*============================================================================
 * WAL Recovery
 *============================================================================*/

WALRecoveryInfo* wal_recovery_info(WAL* wal) {
    if (!wal) return NULL;

    WALRecoveryInfo* info = calloc(1, sizeof(WALRecoveryInfo));
    if (!info) return NULL;

    /* Scan WAL file to find committed transactions */
    off_t offset = WAL_HEADER_SIZE;
    uint64_t tx_id = 0;
    int in_tx = 0;
    uint64_t tx_start = 0;

    (void)tx_start;
    while (offset < (off_t)wal->file_size) {
        if (lseek(wal->fd, offset, SEEK_SET) < 0) break;

        WALEntryHeader header;
        if (read_exact(wal->fd, &header, sizeof(header)) < 0) break;

        switch (header.type) {
            case WAL_ENTRY_BEGIN:
                tx_id = header.tx_id;
                tx_start = offset;
                in_tx = 1;
                break;

            case WAL_ENTRY_COMMIT:
                if (in_tx && header.tx_id == tx_id) {
                    /* Commit found - this transaction committed */
                    info->last_tx_id = header.tx_id;
                    in_tx = 0;
                }
                break;

            case WAL_ENTRY_ROLLBACK:
                in_tx = 0;
                break;

            case WAL_ENTRY_PAGE_MODIFY:
                info->frame_count++;
                break;

            case WAL_ENTRY_CHECKPOINT:
                info->has_checkpoint = 1;
                info->checkpoint_offset = offset;
                break;
        }

        offset += sizeof(header) + header.size;
    }

    info->start_offset = WAL_HEADER_SIZE;
    info->end_offset = wal->log_offset;
    info->frame_count = wal->frame_count;

    return info;
}

int wal_replay(WAL* wal, const char* db_path) {
    (void)wal;
    (void)db_path;
    /* TODO: Implement WAL replay - read frames and apply to database */
    return SUCCESS;
}

int wal_truncate(WAL* wal, uint64_t checkpoint_offset) {
    if (!wal) return ERR_INTERNAL;

    pthread_mutex_lock(&wal->mutex);

    /* Truncate WAL to checkpoint offset */
    if (ftruncate(wal->fd, checkpoint_offset) < 0) {
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }

    wal->log_offset = checkpoint_offset;

    /* Update header */
    WALHeader header;
    if (lseek(wal->fd, 0, SEEK_SET) < 0) {
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }
    if (read_exact(wal->fd, &header, sizeof(header)) < 0) {
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }

    header.end_offset = checkpoint_offset;
    header.frame_count = 0;

    if (lseek(wal->fd, 0, SEEK_SET) < 0) {
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }
    if (write_exact(wal->fd, &header, sizeof(header)) < 0) {
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }

    fsync(wal->fd);

    pthread_mutex_unlock(&wal->mutex);
    return SUCCESS;
}

void wal_recovery_info_free(WALRecoveryInfo* info) {
    if (info) free(info);
}

/*============================================================================
 * Checkpoint Operations
 *============================================================================*/

int wal_checkpoint(WAL* wal) {
    if (!wal) return ERR_INTERNAL;
    return wal_append(wal, WAL_ENTRY_CHECKPOINT, wal->current_tx_id, NULL, 0);
}

int wal_checkpoint_full(WAL* wal, const char* db_path) {
    (void)wal;
    (void)db_path;
    /* TODO: Implement full checkpoint - flush all frames and truncate WAL */
    return SUCCESS;
}

int wal_needs_checkpoint(WAL* wal) {
    if (!wal) return 0;
    return (wal->file_size >= wal->checkpoint_threshold) ? 1 : 0;
}