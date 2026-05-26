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

/* WAL entry types (local - avoid pulling in full enum from header) */
#define WAL_ENTRY_BEGIN       1
#define WAL_ENTRY_COMMIT      2
#define WAL_ENTRY_ROLLBACK    3
#define WAL_ENTRY_PAGE_MODIFY 4
#define WAL_ENTRY_CHECKPOINT  5

/* WAL file header (32 bytes).
 * Layout: magic(4)+version(4)+start_offset(8)+end_offset(8)+frame_count(4)+checksum(4) = 32 bytes */
typedef struct __attribute__((packed)) {
    uint32_t magic;             /* 4 bytes, offset 0 */
    uint32_t version;           /* 4 bytes, offset 4 */
    uint64_t start_offset;      /* 8 bytes, offset 8 */
    uint64_t end_offset;        /* 8 bytes, offset 16 */
    uint32_t frame_count;       /* 4 bytes, offset 24 */
    uint32_t checksum;          /* 4 bytes, offset 28 */
} WALHeader;
_Static_assert(sizeof(WALHeader) == 32, "WALHeader must be exactly 32 bytes");

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

    /* Always create a fresh WAL file so wal_replay sees consistent format.
     * O_CREAT without O_TRUNC: creates file if missing, preserves content if present.
     * For files with content, we truncate and reinitialize. */
    wal->fd = open(wal->path, O_RDWR | O_CREAT, 0644);
    if (wal->fd < 0) {
        free(wal->path);
        free(wal->db_path);
        free(wal);
        return NULL;
    }

    /* Check file size to determine if this is a new or existing WAL */
    struct stat st;
    if (fstat(wal->fd, &st) < 0) {
        close(wal->fd);
        free(wal->path);
        free(wal->db_path);
        free(wal);
        return NULL;
    }

    int is_new_file = ((size_t)st.st_size <= sizeof(WALHeader));
    if (!is_new_file) {
        /* Existing WAL with entries - verify magic, then preserve content.
         * wal_replay needs to read these entries. */
        uint32_t magic;
        if (lseek(wal->fd, 0, SEEK_SET) < 0 ||
            read(wal->fd, &magic, sizeof(magic)) != (ssize_t)sizeof(magic)) {
            close(wal->fd);
            free(wal->path);
            free(wal->db_path);
            free(wal);
            return NULL;
        }
        if (magic != WAL_MAGIC) {
            /* Not a valid WAL - re-create */
            close(wal->fd);
            wal->fd = open(wal->path, O_RDWR | O_CREAT | O_TRUNC, 0644);
            if (wal->fd < 0) {
                free(wal->path);
                free(wal->db_path);
                free(wal);
                return NULL;
            }
            is_new_file = 1;
        } else {
            /* Valid WAL: seek to end of existing content for appending.
             * Do NOT write header - preserve existing entries. */
            if (lseek(wal->fd, 0, SEEK_END) < 0) {
                close(wal->fd);
                free(wal->path);
                free(wal->db_path);
                free(wal);
                return NULL;
            }
            wal->log_offset = (uint64_t)st.st_size;
        }
    }

    if (is_new_file) {
        /* Write fresh WAL header */
        WALHeader header;
        memset(&header, 0, sizeof(header));
        header.magic = WAL_MAGIC;
        header.version = WAL_FORMAT_VERSION;
        header.start_offset = sizeof(WALHeader);
        header.end_offset = sizeof(WALHeader);

        if (write_exact(wal->fd, &header, sizeof(header)) < 0) {
            close(wal->fd);
            unlink(wal->path);
            free(wal->path);
            free(wal->db_path);
            free(wal);
            return NULL;
        }

        wal->log_offset = (uint64_t)sizeof(header);
    }

    /* Get file size */
    if (fstat(wal->fd, &st) < 0) {
        close(wal->fd);
        free(wal->path);
        free(wal->db_path);
        free(wal);
        return NULL;
    }
    wal->file_size = (size_t)st.st_size;

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
    uint32_t checksum = wal_checksum(&header, sizeof(WALEntryHeader));
    if (payload && size > 0) {
        checksum ^= wal_checksum(payload, size);
    }
    header.checksum = checksum;

    /* Write header */
    if (write_exact(wal->fd, &header, sizeof(WALEntryHeader)) < 0) {
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
    wal->log_offset += sizeof(WALEntryHeader) + size;

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

    /* Zero checksum region so checksum covers deterministically-initialized bytes.
     * Bytes 8-31 are reserved; they will be written with checksum values below. */
    memset(frame + 8, 0, WAL_FRAME_HEADER_SIZE - 8);

    /* Copy page data BEFORE computing checksum so it is included */
    memcpy(frame + WAL_FRAME_HEADER_SIZE, page_data, PAGE_SIZE);

    /* Calculate frame checksum over the complete frame (header + page data) */
    uint32_t checksum = wal_checksum(frame, WAL_FRAME_HEADER_SIZE + PAGE_SIZE);
    *(uint32_t*)(frame + 8) = checksum;
    *(uint32_t*)(frame + 12) = checksum ^ 0xFFFFFFFF;

    pthread_mutex_lock(&wal->mutex);

    /* Write type byte prefix so wal_replay can identify PAGE_MODIFY frames.
     * This avoids the ambiguity where entry headers have tx_id overlapping type. */
    uint8_t type_byte = WAL_ENTRY_PAGE_MODIFY;
    if (write_exact(wal->fd, &type_byte, 1) < 0) {
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }

    /* Write raw frame (page_num + frame_size + checksum + page_data) */
    if (write_exact(wal->fd, frame, sizeof(frame)) < 0) {
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }

    wal->log_offset += 1 + sizeof(frame);
    wal->frame_count++;

    /* Update file size */
    struct stat st;
    if (fstat(wal->fd, &st) >= 0) {
        wal->file_size = st.st_size;
    }

    (void)tx_id;
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
    off_t offset = sizeof(WALHeader);
    uint64_t tx_id = 0;
    int in_tx = 0;
    uint64_t tx_start = 0;

    (void)tx_start;
    while (offset < (off_t)wal->file_size) {
        if (lseek(wal->fd, offset, SEEK_SET) < 0) break;

        WALEntryHeader header;
        memset(&header, 0, sizeof(header));
        if (read_exact(wal->fd, &header, sizeof(WALEntryHeader)) < 0) break;

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

    info->start_offset = sizeof(WALHeader);
    info->end_offset = wal->log_offset;
    info->frame_count = wal->frame_count;

    return info;
}

int wal_replay(WAL* wal, const char* db_path) {
    if (!wal || !db_path) return ERR_INTERNAL;

    /* Open database file directly for applying frames */
    int db_fd = open(db_path, O_RDWR);
    if (db_fd < 0) return ERR_STORAGE_IO;

    /* Seek past WAL header */
    if (lseek(wal->fd, sizeof(WALHeader), SEEK_SET) < 0) {
        close(db_fd);
        return ERR_STORAGE_IO;
    }

    /* Track highest page number seen so we can extend the db file if needed */
    uint32_t max_page_seen = 0;
    int ret = SUCCESS;
    off_t current = sizeof(WALHeader);

    while (1) {
        /* Read the entry type byte.
         * For PAGE_MODIFY (type=4), wal_write_page writes a type byte + 16-byte frame
         *   header + PAGE_SIZE bytes of page data.
         * For other types, there's a full WALEntryHeader (16 bytes). */
        uint8_t type_byte;
        if (read(wal->fd, &type_byte, 1) != 1) {
            if (errno == 0) break;  /* EOF */
            ret = ERR_STORAGE_IO;
            goto done;
        }

        /* Seek back so the full header read starts from the type byte */
        if (lseek(wal->fd, -1, SEEK_CUR) < 0) {
            ret = ERR_STORAGE_IO;
            goto done;
        }

        if (type_byte == WAL_ENTRY_PAGE_MODIFY) {
            /* PAGE_MODIFY: type byte (1) + frame header (16) + page data (4096).
             * After reading the 16-byte frame header, the file position is
             * current + 1 + 16 = current + 17, which is where page data begins.
             * The frame_data buffer places page data at offset WAL_FRAME_HEADER_SIZE (32),
             * so we must seek 16 bytes into the read buffer. */
            char frame_hdr[16];
            ssize_t h = read(wal->fd, frame_hdr, 16);
            if (h < 16) {
                ret = ERR_STORAGE_CORRUPT;
                goto done;
            }

            uint32_t page_num = *(uint32_t*)(frame_hdr + 0);
            /* frame_size from frame_hdr + 4 is PAGE_SIZE, validated by writer */

            /* File position is now current + 17 (type byte + header read).
             * Seek 16 bytes forward to place page data at buffer offset 32. */
            if (lseek(wal->fd, WAL_FRAME_HEADER_SIZE - 16, SEEK_CUR) < 0) {
                ret = ERR_STORAGE_IO;
                goto done;
            }

            /* Read page data into frame_data at offset WAL_FRAME_HEADER_SIZE. */
            char frame_data[WAL_FRAME_HEADER_SIZE + PAGE_SIZE];
            memset(frame_data, 0, WAL_FRAME_HEADER_SIZE); /* zero header region */
            ssize_t p = read(wal->fd, frame_data + WAL_FRAME_HEADER_SIZE, PAGE_SIZE);
            if ((size_t)p < PAGE_SIZE) {
                ret = ERR_STORAGE_CORRUPT;
                goto done;
            }

            /* Advance position past page data.
             * current + 1 (type) + 16 (header) + 4096 (page) = current + 4113 */
            current += 1 + 16 + PAGE_SIZE;

            /* Extend db file if needed */
            if (page_num > max_page_seen) {
                off_t needed_size = (off_t)(page_num + 1) * PAGE_SIZE;
                if (ftruncate(db_fd, needed_size) < 0) {
                    ret = ERR_STORAGE_IO;
                    goto done;
                }
                max_page_seen = page_num;
            }

            /* Apply page to database file.
             * The actual page data starts WAL_FRAME_HEADER_SIZE bytes into frame_data. */
            off_t offset = (off_t)page_num * PAGE_SIZE;
            if (lseek(db_fd, offset, SEEK_SET) < 0) {
                ret = ERR_STORAGE_IO;
                goto done;
            }
            if (write(db_fd, frame_data + WAL_FRAME_HEADER_SIZE, PAGE_SIZE) != (ssize_t)PAGE_SIZE) {
                ret = ERR_STORAGE_IO;
                goto done;
            }
        } else {
            /* Regular entry with WALEntryHeader (16 bytes):
             *   byte 0:  type
             *   byte 1:  flags
             *   byte 2-3: size (uint16_t)
             *   byte 4-7: checksum (uint32_t)
             *   byte 8-15: tx_id (uint64_t) */
            WALEntryHeader header;
            memset(&header, 0, sizeof(header));
            ssize_t n = read(wal->fd, &header, sizeof(WALEntryHeader));
            if (n < 0) {
                ret = ERR_STORAGE_IO;
                goto done;
            }
            if ((size_t)n < sizeof(WALEntryHeader)) {
                ret = ERR_STORAGE_CORRUPT;
                goto done;
            }

            current += 1 + sizeof(WALEntryHeader);

            switch (type_byte) {
                case WAL_ENTRY_BEGIN:
                case WAL_ENTRY_COMMIT:
                case WAL_ENTRY_ROLLBACK:
                case WAL_ENTRY_CHECKPOINT:
                    /* These entries have no payload - just skip header.size bytes */
                    break;

                default:
                    ret = ERR_STORAGE_CORRUPT;
                    goto done;
            }

            /* Advance past any payload */
            off_t payload_start = current + header.size;
            if (lseek(wal->fd, payload_start, SEEK_SET) < 0) {
                ret = ERR_STORAGE_IO;
                goto done;
            }
            current = payload_start;
        }
    }

done:
    fsync(db_fd);
    close(db_fd);
    return ret;
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
    if (!wal || !db_path) return ERR_INTERNAL;

    pthread_mutex_lock(&wal->mutex);

    /* Open database file to flush pages */
    int db_fd = open(db_path, O_RDWR);
    if (db_fd < 0) {
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }

    /* Write checkpoint marker first */
    WALEntryHeader ckpt_header;
    memset(&ckpt_header, 0, sizeof(ckpt_header));
    ckpt_header.type = (uint8_t)WAL_ENTRY_CHECKPOINT;
    ckpt_header.size = 0;
    ckpt_header.tx_id = wal->current_tx_id;
    ckpt_header.checksum = wal_checksum(&ckpt_header, sizeof(WALEntryHeader));

    if (write_exact(wal->fd, &ckpt_header, sizeof(WALEntryHeader)) < 0) {
        close(db_fd);
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }

    /* Sync database file to ensure all changes are persisted */
    if (fsync(db_fd) < 0) {
        close(db_fd);
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }

    /* Truncate WAL file to just after header */
    uint64_t ckpt_offset = sizeof(WALHeader);
    if (ftruncate(wal->fd, ckpt_offset) < 0) {
        close(db_fd);
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }

    /* Update WAL header */
    WALHeader header;
    memset(&header, 0, sizeof(header));
    header.magic = WAL_MAGIC;
    header.version = WAL_FORMAT_VERSION;
    header.start_offset = sizeof(WALHeader);
    header.end_offset = sizeof(WALHeader);
    header.frame_count = 0;
    header.checksum = 0;

    if (lseek(wal->fd, 0, SEEK_SET) < 0) {
        close(db_fd);
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }
    if (write_exact(wal->fd, &header, sizeof(header)) < 0) {
        close(db_fd);
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }

    /* Sync WAL to persist the truncation */
    if (fsync(wal->fd) < 0) {
        close(db_fd);
        pthread_mutex_unlock(&wal->mutex);
        return ERR_STORAGE_IO;
    }

    /* Update WAL struct state */
    wal->log_offset = sizeof(WALHeader);
    wal->frame_count = 0;
    wal->last_checkpoint = sizeof(WALHeader);

    close(db_fd);
    pthread_mutex_unlock(&wal->mutex);
    return SUCCESS;
}

int wal_needs_checkpoint(WAL* wal) {
    if (!wal) return 0;
    return (wal->file_size >= wal->checkpoint_threshold) ? 1 : 0;
}