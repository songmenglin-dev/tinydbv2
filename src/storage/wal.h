#ifndef TINYDB_WAL_H
#define TINYDB_WAL_H

#include "../../include/tinydb.h"
#include <pthread.h>
#include <stdint.h>

/*============================================================================
 * WAL Constants
 *============================================================================*/

/* WAL magic number ("TLDW" - Tiny Log Data Write) */
#ifndef WAL_MAGIC
#define WAL_MAGIC 0x544C4442
#endif
#define WAL_FORMAT_VERSION 1

/* WAL entry types */
typedef enum {
    WAL_ENTRY_BEGIN = 1,
    WAL_ENTRY_COMMIT = 2,
    WAL_ENTRY_ROLLBACK = 3,
    WAL_ENTRY_PAGE_MODIFY = 4,
    WAL_ENTRY_CHECKPOINT = 5
} WALEntryType;

/* WAL frame header size */
#define WAL_FRAME_HEADER_SIZE 32

/* WAL entry header size (sizeof packed WALEntryHeader) */
#define WAL_ENTRY_HEADER_SIZE 16

/* Maximum WAL frame size (page data + header) */
#define WAL_MAX_FRAME_SIZE (PAGE_SIZE + WAL_FRAME_HEADER_SIZE)

/* Default auto-checkpoint threshold (64MB) */
#define WAL_AUTOCHECKPOINT_THRESHOLD (64 * 1024 * 1024)

/*============================================================================
 * WAL Entry Header (12 bytes)
 *============================================================================*/

typedef struct __attribute__((packed)) {
    uint8_t type;           /* WAL entry type */
    uint8_t flags;          /* Reserved flags */
    uint16_t size;          /* Entry size (excluding header) */
    uint32_t checksum;      /* Entry checksum */
    uint64_t tx_id;         /* Transaction ID */
} WALEntryHeader;

/* WAL entry with allocated payload buffer */
typedef struct WALEntry {
    WALEntryHeader header;
    uint8_t payload[1];     /* Variable size payload */
} WALEntry;

/*============================================================================
 * WAL Frame (for recovery)
 *============================================================================*/

typedef struct WALFrame {
    uint32_t page_num;          /* Page number */
    uint32_t frame_size;        /* Frame data size */
    uint8_t page_data[PAGE_SIZE]; /* Page data */
} WALFrame;

/*============================================================================
 * WAL Recovery Information
 *============================================================================*/

typedef struct {
    uint64_t start_offset;      /* Start offset of first entry */
    uint64_t end_offset;        /* End offset (last entry + 1) */
    uint64_t last_tx_id;        /* Last transaction ID seen */
    uint32_t frame_count;       /* Total frames in WAL */
    int has_checkpoint;        /* Contains a checkpoint */
    uint64_t checkpoint_offset; /* Offset of checkpoint if present */
} WALRecoveryInfo;

/*============================================================================
 * WAL Structure
 *============================================================================*/

typedef struct WAL {
    /* File descriptors */
    int fd;                     /* WAL file descriptor */
    int shm_fd;                 /* Shared memory file (for future use) */
    char* path;                 /* WAL file path */
    char* db_path;              /* Database file path */

    /* WAL state */
    uint64_t log_offset;        /* Current write offset */
    uint64_t last_checkpoint;   /* Offset of last checkpoint */
    uint32_t frame_count;       /* Total frames written */
    uint64_t current_tx_id;    /* Current transaction ID */
    size_t file_size;           /* Current WAL file size */

    /* Checkpoint threshold */
    size_t checkpoint_threshold; /* Auto-checkpoint trigger */

    /* Thread safety */
    pthread_mutex_t mutex;

    /* WAL is open */
    int is_open;
} WAL;

/*============================================================================
 * WAL Lifecycle
 *============================================================================*/

/* Open or create WAL file */
WAL* wal_open(const char* db_path);

/* Close WAL file */
void wal_close(WAL* wal);

/* Flush WAL to disk */
int wal_flush(WAL* wal);

/*============================================================================
 * WAL Entry Operations
 *============================================================================*/

/* Append an entry to WAL */
int wal_append(WAL* wal, WALEntryType type, uint64_t tx_id,
               const void* payload, uint32_t size);

/* Write transaction boundaries */
int wal_begin_tx(WAL* wal);
int wal_commit_tx(WAL* wal);
int wal_rollback_tx(WAL* wal);

/* Write page modification entry */
int wal_write_page(WAL* wal, uint64_t tx_id, uint32_t page_num, const void* page_data);

/*============================================================================
 * WAL Recovery
 *============================================================================*/

/* Get WAL recovery information by scanning */
WALRecoveryInfo* wal_recovery_info(WAL* wal);

/* Perform recovery from WAL */
int wal_recover(WAL* wal, const char* db_path);

/* Replay WAL entries */
int wal_replay(WAL* wal, const char* db_path);

/* Truncate WAL after checkpoint */
int wal_truncate(WAL* wal, uint64_t checkpoint_offset);

/* Free recovery info */
void wal_recovery_info_free(WALRecoveryInfo* info);

/*============================================================================
 * Checkpoint Operations
 *============================================================================*/

/* Write checkpoint marker */
int wal_checkpoint(WAL* wal);

/* Perform full checkpoint (blocking) */
int wal_checkpoint_full(WAL* wal, const char* db_path);

/* Check if checkpoint is needed */
int wal_needs_checkpoint(WAL* wal);

/*============================================================================
 * Utility Functions
 *============================================================================*/

/* Calculate checksum for WAL entry */
uint32_t wal_checksum(const void* data, size_t len);

/* Verify WAL file magic */
int wal_verify_magic(int fd);

/* Get WAL file path from database path */
char* wal_get_path(const char* db_path);

#endif /* TINYDB_WAL_H */