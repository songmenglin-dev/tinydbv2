# TinyDB v2 Technical Architecture

## Document Information

- **Version**: 1.0
- **Project**: TinyDB v2 - Lightweight SQL Database in C
- **Date**: 2026-05-21
- **Status**: Draft

---

## 1. System Architecture Overview

### 1.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              TinyDB System                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────┐                    ┌─────────────────────────────────────┐│
│  │  CLI Client │ ◄──── Unix Socket ───► │        Server Daemon              ││
│  │  (tinydb-cli)│                     │  ┌──────────────────────────────┐  ││
│  └─────────────┘                     │  │     Protocol Handler          │  ││
│                                      │  └──────────────┬───────────────┘  ││
│                                      │                 │                   ││
│                                      │  ┌──────────────▼───────────────┐  ││
│                                      │  │       SQL Engine              │  ││
│                                      │  │  ┌─────────┐  ┌──────────┐   │  ││
│                                      │  │  │ Parser  │  │ Executor │   │  ││
│                                      │  │  └─────────┘  └──────────┘   │  ││
│                                      │  └──────────────┬───────────────┘  ││
│                                      │                 │                   ││
│                                      │  ┌──────────────▼───────────────┐  ││
│                                      │  │       Storage Engine         │  ││
│                                      │  │  ┌─────────┐  ┌──────────┐   │  ││
│                                      │  │  │  B+Tree │  │   WAL    │   │  ││
│                                      │  │  └─────────┘  └──────────┘   │  ││
│                                      │  └──────────────┬───────────────┘  ││
│                                      │                 │                   ││
│                                      │  ┌──────────────▼───────────────┐  ││
│                                      │  │       File Manager           │  ││
│                                      │  └──────────────────────────────┘  ││
│                                      └─────────────────────────────────────┘│
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐│
│  │                        Persistent Storage                               ││
│  │  ┌─────────────────────┐  ┌─────────────────────┐  ┌─────────────────┐  ││
│  │  │   main.db           │  │   main.db-wal       │  │   main.db-shm   │  ││
│  │  │   (Database File)   │  │   (WAL File)        │  │   (Shared Mem) │  ││
│  │  └─────────────────────┘  └─────────────────────┘  └─────────────────┘  ││
│  └─────────────────────────────────────────────────────────────────────────┘│
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 Component Responsibilities

| Component | Responsibility | Public Interface |
|-----------|----------------|------------------|
| **CLI Client** | Read user input, send to server, display results | `main()` loop |
| **Protocol Handler** | Parse client requests, format responses | `handle_request()` |
| **SQL Parser** | Tokenize and parse SQL into AST | `sql_parse()`, `tokenize()` |
| **Query Executor** | Execute AST, produce results | `execute()`, `exec_select()` |
| **Storage Engine** | Page I/O, cache management | `page_read()`, `page_write()` |
| **B+Tree Module** | Index operations, tree traversal | `btree_insert()`, `btree_search()` |
| **WAL Module** | Write-ahead logging, recovery | `wal_append()`, `wal_recover()` |
| **File Manager** | File I/O, locking, buffering | `file_open()`, `file_sync()` |

### 1.3 Data Flow

```
1. Client Input Flow:
   stdin → CLI → Unix Socket

2. Server Request Flow:
   Unix Socket → Protocol Handler → SQL Parser → AST

3. Query Execution Flow:
   AST → Query Executor → Storage Engine → B+Tree → WAL → File Manager

4. Response Flow:
   Results ← Protocol Handler ← Unix Socket → CLI → stdout
```

---

## 2. File Structure

### 2.1 Complete Directory Layout

```
tinydbv2/
├── src/
│   ├── main.c                    # Server entry point
│   ├── cli.c                     # CLI client entry point
│   │
│   ├── protocol/
│   │   ├── protocol.h            # Protocol definitions
│   │   ├── protocol.c            # Protocol handler implementation
│   │   ├── message.h             # Message structures
│   │   └── message.c             # Message encode/decode
│   │
│   ├── storage/
│   │   ├── storage.h             # Storage engine interface
│   │   ├── storage.c             # Storage engine implementation
│   │   ├── page.h                # Page format definitions
│   │   ├── page.c                # Page I/O operations
│   │   ├── buffer.h              # Buffer pool header
│   │   └── buffer.c              # Buffer pool implementation
│   │
│   ├── btree/
│   │   ├── btree.h               # B+Tree interface
│   │   ├── btree.c               # B+Tree implementation
│   │   ├── btree_node.h          # Node structures
│   │   └── btree_node.c          # Node operations
│   │
│   ├── index/
│   │   ├── index.h               # Index interface
│   │   ├── index.c               # Index management
│   │   ├── index_iter.h          # Index iterator
│   │   └── index_iter.c          # Index iteration
│   │
│   ├── wal/
│   │   ├── wal.h                 # WAL interface
│   │   ├── wal.c                 # WAL implementation
│   │   ├── wal_reader.h          # WAL reader
│   │   ├── wal_reader.c          # WAL recovery
│   │   ├── wal_writer.h          # WAL writer
│   │   └── wal_writer.c          # WAL append
│   │
│   ├── parser/
│   │   ├── parser.h              # Parser interface
│   │   ├── parser.c              # Parser implementation
│   │   ├── lexer.h               # Lexer interface
│   │   ├── lexer.c               # Tokenizer
│   │   ├── token.h               # Token definitions
│   │   ├── ast.h                 # AST node types
│   │   ├── ast.c                 # AST construction
│   │   ├── expr.h                # Expression handling
│   │   └── expr.c                # Expression evaluation
│   │
│   ├── executor/
│   │   ├── executor.h            # Executor interface
│   │   ├── executor.c            # Query execution
│   │   ├── select.h              # SELECT execution
│   │   ├── select.c              # SELECT pipeline
│   │   ├── insert.h              # INSERT execution
│   │   ├── insert.c              # INSERT implementation
│   │   ├── update.h              # UPDATE execution
│   │   ├── update.c              # UPDATE implementation
│   │   ├── delete.h              # DELETE execution
│   │   ├── delete.c              # DELETE implementation
│   │   ├── create.h              # DDL execution
│   │   └── create.c              # CREATE TABLE/INDEX
│   │
│   ├── catalog/
│   │   ├── catalog.h             # Catalog interface
│   │   ├── catalog.c             # Schema management
│   │   ├── table.h               # Table metadata
│   │   ├── table.c               # Table operations
│   │   ├── column.h              # Column metadata
│   │   └── column.c              # Column operations
│   │
│   ├── tx/
│   │   ├── transaction.h          # Transaction interface
│   │   ├── transaction.c          # Transaction management
│   │   ├── lock.h                # Lock manager
│   │   └── lock.c                # Lock implementation
│   │
│   └── util/
│       ├── error.h               # Error handling
│       ├── error.c               # Error implementation
│       ├── memory.h              # Memory utilities
│       ├── memory.c              # Memory management
│       ├── hash.h                # Hash functions
│       ├── hash.c               # Hash implementation
│       └── util.h                # Common utilities
│
├── include/
│   ├── tinydb.h                  # Public API header
│   ├── config.h                  # Configuration constants
│   └── types.h                   # Core type definitions
│
├── tests/
│   ├── unit/
│   │   ├── test_lexer.c          # Lexer tests
│   │   ├── test_parser.c         # Parser tests
│   │   ├── test_btree.c          # B+Tree tests
│   │   ├── test_wal.c            # WAL tests
│   │   ├── test_storage.c        # Storage tests
│   │   └── test_executor.c       # Executor tests
│   │
│   ├── integration/
│   │   ├── test_crud.c           # CRUD integration tests
│   │   ├── test_transactions.c   # Transaction tests
│   │   ├── test_schema.c         # Schema tests
│   │   └── test_recovery.c       # Recovery tests
│   │
│   └── harness/
│       ├── harness.h             # Test harness header
│       └── harness.c             # Test runner
│
├── docs/
│   ├── architecture.md           # This document
│   ├── sql-reference.md          # SQL syntax reference
│   └── protocol.md               # Wire protocol spec
│
├── scripts/
│   ├── build.sh                  # Build script
│   ├── test.sh                   # Test runner
│   └── benchmark.sh              # Performance benchmarks
│
├── systemd/
│   └── tinydb.service            # Systemd service unit
│
├── Makefile                      # Build configuration
└── README.md                     # Project README
```

### 2.2 Header File Dependencies

```
include/tinydb.h
    └── src/util/error.h
    └── src/util/types.h

include/config.h
    └── (no dependencies)

include/types.h
    └── (no dependencies)

src/main.c
    └── src/protocol/protocol.h
    └── src/storage/storage.h
    └── src/executor/executor.h
    └── src/util/error.h

src/protocol/protocol.h
    └── src/util/error.h
    └── include/types.h

src/parser/parser.h
    └── src/parser/lexer.h
    └── src/parser/ast.h
    └── src/parser/token.h
    └── src/util/error.h

src/executor/executor.h
    └── src/parser/ast.h
    └── src/storage/storage.h
    └── src/catalog/catalog.h
    └── src/tx/transaction.h

src/storage/storage.h
    └── src/btree/btree.h
    └── src/wal/wal.h
    └── include/config.h

src/btree/btree.h
    └── include/types.h
    └── src/util/error.h

src/wal/wal.h
    └── include/types.h
    └── src/util/error.h
```

---

## 3. Storage Engine Architecture

### 3.1 Page Format

**Page Size**: 4096 bytes (fixed)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Database File Layout                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌──────────────┐                                                           │
│  │ Page 0       │  Header Page (required)                                   │
│  │ (4096 bytes) │  - Database header                                       │
│  └──────────────┘  - Free page list head                                   │
│         │        - Schema root pointer                                     │
│         ▼                                                                  │
│  ┌──────────────┐                                                           │
│  │ Page 1       │  First data page or internal page                        │
│  │ (4096 bytes) │                                                           │
│  └──────────────┘                                                           │
│         │                                                                   │
│         ▼                                                                   │
│  ┌──────────────┐                                                           │
│  │ Page 2       │  B+Tree leaf or internal page                            │
│  │ (4096 bytes) │                                                           │
│  └──────────────┘                                                           │
│         │                                                                   │
│         ▼                                                                   │
│  ┌──────────────┐                                                           │
│  │    ...       │                                                           │
│  └──────────────┘                                                           │
│                                                                             │
│  Page types: HEADER (0), TABLE_LEAF (13), TABLE_INTERNAL (10),             │
│              INDEX_LEAF (10), INDEX_INTERNAL (2), FREELIST (14),            │
│              OVERFLOW (15)                                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Header Page Structure

```c
// File: include/types.h

#define PAGE_SIZE 4096
#define MAX_PAGE_NUMBER 0x7FFFFFFF

// Page types stored in page header
#define PAGE_TYPE_HEADER      0x00
#define PAGE_TYPE_TABLE_LEAF  0x0D  // 13
#define PAGE_TYPE_TABLE_INTERNAL 0x0A  // 10
#define PAGE_TYPE_INDEX_LEAF  0x0A
#define PAGE_TYPE_INDEX_INTERNAL 0x02
#define PAGE_TYPE_FREELIST    0x0E  // 14
#define PAGE_TYPE_OVERFLOW    0x0F  // 15

// Header page structure (offset in bytes)
typedef struct __attribute__((packed)) {
    // Offset 0-3: Magic number "TINY" (4 bytes)
    uint32_t magic;

    // Offset 4-7: Page version (4 bytes)
    uint32_t version;

    // Offset 8-11: File format version (4 bytes)
    uint32_t format_version;

    // Offset 12-15: Reserved for future use (4 bytes)
    uint32_t reserved;

    // Offset 16-19: Page count (4 bytes)
    uint32_t page_count;

    // Offset 20-23: First free page number, 0 if none (4 bytes)
    uint32_t first_free_page;

    // Offset 24-27: Free page count (4 bytes)
    uint32_t free_page_count;

    // Offset 28-31: Schema root page number (4 bytes)
    uint32_t schema_root;

    // Offset 32-35: WAL frame count (4 bytes)
    uint32_t wal_frame_count;

    // Offset 36-39: WAL checksum part 1 (4 bytes)
    uint32_t wal_checksum_1;

    // Offset 40-43: WAL checksum part 2 (4 bytes)
    uint32_t wal_checksum_2;

    // Offset 44-47: Database size in pages (4 bytes)
    uint32_t database_size;

    // Offset 48-4095: Reserved for future use (4048 bytes)
    uint8_t reserved_space[PAGE_SIZE - 48];
} PageHeader;

// Total header size: 48 bytes
// Magic value: 0x54494E59 ('TINY' in ASCII)
#define HEADER_MAGIC 0x54494E59
#define CURRENT_FORMAT_VERSION 1
```

### 3.3 B+Tree Node Format

#### 3.3.1 Generic B+Tree Node Header

```c
// File: src/btree/btree_node.h

// B+Tree node header (16 bytes)
typedef struct __attribute__((packed)) {
    // Offset 0: Page type (1 byte)
    uint8_t page_type;

    // Offset 1: Number of cells (2 bytes)
    uint16_t cell_count;

    // Offset 3: Start of cell content area (2 bytes, offset from page start)
    uint16_t content_start;

    // Offset 5: Number of fragmented free bytes (1 byte)
    uint8_t fragmented_bytes;

    // Offset 6: Rightmost child pointer (for internal nodes only, 4 bytes)
    // For leaf nodes, this field does not exist
    uint32_t right_child;

    // Total header size: 10 bytes (leaf) or 14 bytes (internal)
} BTreeNodeHeader;

// Cell pointer array follows header
// Each pointer is 2 bytes, pointing to cell content
typedef uint16_t CellPointer;  // Offset from page start

// Cell pointer array: cell_count * 2 bytes
// Located immediately after node header
```

#### 3.3.2 Table B+Tree Leaf Node

```c
// File: src/btree/btree_node.h

// Table leaf cell structure
// Variable size, stored at cell content area

typedef struct __attribute__((packed)) {
    // Payload size (variable length encoding, 1-3 bytes)
    // For simplicity, we use fixed 4 bytes
    uint32_t payload_size;

    // Row ID (4 bytes)
    uint32_t row_id;

    // Key size (2 bytes) - size of primary key
    uint16_t key_size;

    // Key data (key_size bytes) - primary key value
    // Then remaining payload (payload_size - key_size - 6 bytes)

} TableLeafCell;

// Example leaf node layout (page 4096 bytes):
// Offset 0-9:   Node header (page_type, cell_count, content_start, fragmented)
// Offset 10-47: Cell pointer array (19 cells * 2 bytes = 38 bytes, so 19 cells)
// Offset 48-4095: Cell content area
```

#### 3.3.3 Table B+Tree Internal Node

```c
// File: src/btree/btree_node.h

// Table internal cell structure
typedef struct __attribute__((packed)) {
    // Child page number (4 bytes)
    uint32_t child_page;

    // Key size (2 bytes)
    uint16_t key_size;

    // Key data (key_size bytes)
    // First byte of key is the key prefix for search
} TableInternalCell;

// Internal node cell pointer + cell structure:
// Cell pointer array points to cell content
// Each cell: 4 (child) + 2 (key_size) + key_size (key data) = 6 + key_size bytes
```

### 3.4 Row Storage Format

```c
// File: src/storage/page.h

// Row header (6 bytes)
typedef struct __attribute__((packed)) {
    // Offset 0-1: Row size (2 bytes)
    uint16_t row_size;

    // Offset 2-3: Number of columns (2 bytes)
    uint16_t column_count;

    // Offset 4-5: NULL bitmap offset (2 bytes)
    uint16_t null_bitmap_offset;
} RowHeader;

// Row format:
// [RowHeader: 6 bytes]
// [NULL bitmap: ceil(column_count / 8) bytes]
// [Column 1 data]
// [Column 2 data]
// ...

// Column data storage by type:
// INTEGER: 8 bytes, big-endian
// REAL: 8 bytes, IEEE 754 double
// TEXT: 4 bytes length + N bytes data
// BLOB: 4 bytes length + N bytes data

// Row ID is stored separately in B+Tree key, not in row data
// Row ID: 4 bytes, assigned sequentially per table

typedef struct __attribute__((packed)) {
    // Row ID (4 bytes)
    uint32_t row_id;

    // Row data (variable)
    uint8_t data[];
} StoredRow;
```

### 3.5 Index B-Tree Format

```c
// File: src/index/index.h

// Index entry structure
typedef struct __attribute__((packed)) {
    // Index key size (2 bytes)
    uint16_t key_size;

    // Index key data (key_size bytes)
    uint8_t key_data[];

    // Row ID (4 bytes) - the value associated with the key
    uint32_t row_id;
} IndexEntry;

// Index leaf cell format:
// [key_size: 2 bytes]
// [key_data: key_size bytes]
// [row_id: 4 bytes]
```

### 3.6 Page Cache (Buffer Pool)

```c
// File: src/storage/buffer.h

#define BUFFER_POOL_SIZE 256  // Number of cached pages

typedef struct {
    uint32_t page_number;
    uint8_t data[PAGE_SIZE];
    bool is_dirty;
    bool is_pinned;
    int pin_count;
    LRUEntry* lru_next;
    LRUEntry* lru_prev;
} BufferFrame;

typedef struct {
    BufferFrame frames[BUFFER_POOL_SIZE];
    HashMap* page_map;  // page_number -> frame index
    LRUList* lru_head;
    LRUList* lru_tail;
    int hit_count;
    int miss_count;
} BufferPool;
```

---

## 4. WAL Architecture

### 4.1 WAL File Format

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           WAL File Format                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                        WAL Header (32 bytes)                          │ │
│  │  ┌──────────────────────────────────────────────────────────────────┐ │ │
│  │  │  Magic: 0x377F0682 (4 bytes)                                    │ │ │
│  │  │  Format version: 1 (4 bytes)                                    │ │ │
│  │  │  Page size: 4096 (4 bytes)                                      │ │ │
│  │  │  Sequence number (8 bytes)                                      │ │ │
│  │  │  Salt 1 for checksum (4 bytes)                                  │ │ │
│  │  │  Salt 2 for checksum (4 bytes)                                  │ │ │
│  │  │  Checksum of header (4 bytes, Salz1 + Salt2)                     │ │ │
│  │  └──────────────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│         │                                                                    │
│         ▼                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                        Frame 1 (4100 bytes)                           │ │
│  │  ┌─────────────────────┬──────────────────────────────────────────┐  │ │
│  │  │  Frame Header (4B)  │           Page Data (4096 bytes)          │  │ │
│  │  │  - Page number      │                                          │  │ │
│  │  │  - Commit size      │                                          │  │ │
│  │  └─────────────────────┴──────────────────────────────────────────┘  │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│         │                                                                    │
│         ▼                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                        Frame 2 (4100 bytes)                           │ │
│  │  └──────────────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│         │                                                                    │
│         ▼                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                        ...                                            │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 WAL Header Structure

```c
// File: src/wal/wal.h

// WAL header (32 bytes)
typedef struct __attribute__((packed)) {
    // Offset 0-3: Magic number 0x377F0682 (4 bytes)
    uint32_t magic;

    // Offset 4-7: Format version, currently 1 (4 bytes)
    uint32_t format_version;

    // Offset 8-11: Database page size (4 bytes), must equal PAGE_SIZE
    uint32_t page_size;

    // Offset 12-19: WAL sequence number (8 bytes)
    uint64_t sequence_number;

    // Offset 20-23: Salt value 1 for checksum (4 bytes)
    uint32_t salt_1;

    // Offset 24-27: Salt value 2 for checksum (4 bytes)
    uint32_t salt_2;

    // Offset 28-31: Checksum of first 28 bytes (4 bytes)
    uint32_t checksum;
} WALHeader;

#define WAL_MAGIC 0x377F0682
#define WAL_FORMAT_VERSION 1
```

### 4.3 WAL Frame Structure

```c
// File: src/wal/wal_writer.h

// WAL frame header (4 bytes)
typedef struct __attribute__((packed)) {
    // Offset 0-3: Page number in database (4 bytes)
    uint32_t page_number;
} WALFrameHeader;

// Frame format: 4 bytes header + 4096 bytes page data = 4100 bytes total

// For committed frames, the frame also includes:
// Salt values used for this frame's checksum
```

### 4.4 WAL Operations

```c
// File: src/wal/wal_writer.h

typedef struct {
    int fd;                      // WAL file descriptor
    uint64_t sequence_number;    // Current sequence number
    uint32_t frame_count;       // Number of frames in WAL
    uint32_t salt_1;            // Salt for checksums
    uint32_t salt_2;            // Salt for checksums
    off_t file_size;             // Current file size
    bool is_committing;          // Currently committing transaction
} WALWriter;

// WAL append sequence:
// 1. Write page to WAL (wal_writer_append)
// 2. Update is_committing flag
// 3. Sync WAL to disk (wal_writer_commit)

typedef struct {
    int fd;                      // WAL file descriptor
    uint64_t sequence_number;    // Sequence number from header
    uint32_t frame_count;        // Number of frames
    WALHeader header;            // Parsed header
    uint32_t current_frame;      // Current frame position for iteration
} WALReader;

// WAL recovery sequence:
// 1. Open WAL file (wal_reader_open)
// 2. Read and validate header (wal_reader_read_header)
// 3. Iterate frames, collect changes (wal_reader_iterate_frames)
// 4. Apply all changes to main database (wal_recovery_apply)
// 5. Optionally checkpoint (wal_recovery_checkpoint)
```

### 4.5 WAL Recovery Algorithm

```c
// File: src/wal/wal_reader.c

// Recovery algorithm (pseudocode):
void wal_recover(Database* db) {
    // Step 1: Open WAL file, read header
    WALReader* reader = wal_reader_open(db->wal_path);
    if (!reader) return;  // No WAL, nothing to recover

    // Step 2: Read all frames into memory
    FrameList* frames = frame_list_create();
    while (wal_reader_has_next(reader)) {
        WALFrame* frame = wal_reader_next(reader);
        frame_list_append(frames, frame);
    }

    // Step 3: Group frames by page number (last frame wins)
    HashMap* latest = hash_map_create();
    for each frame in frames (in order) {
        uint32_t page_no = frame->page_number;
        hash_map_set(latest, page_no, frame);
    }

    // Step 4: Apply frames to database in order
    for each (page_no, frame) in latest (in order) {
        // For each unique page, the latest frame is the authoritative version
        // Write to main database at page_no
        storage_write_page(db->storage, page_no, frame->data);
    }

    // Step 5: Truncate WAL file (recovery complete)
    wal_reader_close(reader);
    truncate_wal_file();

    // Step 6: Optionally checkpoint (if WAL is large)
    if (frame_count > CHECKPOINT_THRESHOLD) {
        storage_checkpoint(db->storage);
    }
}
```

### 4.6 Checkpoint Strategy

```c
// File: src/wal/wal.h

// Checkpoint triggers:
// 1. WAL frame count exceeds threshold (default: 1000 frames)
// 2. Explicit CHECKPOINT command from client
// 3. Database shutdown (automatic)

// Checkpoint process:
// 1. Acquire checkpoint lock
// 2. Sync all WAL frames to disk
// 3. Write all dirty pages from buffer pool to main database
// 4. Sync main database to disk
// 5. Truncate WAL file
// 6. Update header to reflect clean state
// 7. Release checkpoint lock

#define WAL_CHECKPOINT_THRESHOLD 1000
#define WAL_AUTOCHECKPOINT_ENABLED true
```

---

## 5. SQL Parser Architecture

### 5.1 Token Types

```c
// File: src/parser/token.h

typedef enum {
    // Literals
    TOKEN_INTEGER,          // 123, 0x1F
    TOKEN_FLOAT,            // 3.14, 1e-5
    TOKEN_STRING,           // 'hello', "world"
    TOKEN_BLOB,             // x'0ABC1234', X'deadbeef'
    TOKEN_IDENTIFIER,       // table_name, column1
    TOKEN_PARAMETER,        // ? (parameter placeholder)

    // Operators
    TOKEN_PLUS,             // +
    TOKEN_MINUS,           // -
    TOKEN_STAR,            // *
    TOKEN_SLASH,           // /
    TOKEN_PERCENT,         // %
    TOKEN_EQ,               // =
    TOKEN_NEQ,              // != or <>
    TOKEN_LT,               // <
    TOKEN_GT,               // >
    TOKEN_LTE,              // <=
    TOKEN_GTE,              // >=
    TOKEN_DOUBLE_EQ,        // ==
    TOKEN_BITAND,           // &
    TOKEN_BITOR,            // |
    TOKEN_BITNOT,           // ~
    TOKEN_BITXOR,           // ^

    // Keywords
    TOKEN_SELECT,
    TOKEN_FROM,
    TOKEN_WHERE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_IN,
    TOKEN_LIKE,
    TOKEN_BETWEEN,
    TOKEN_IS,
    TOKEN_NULL,

    TOKEN_INSERT,
    TOKEN_INTO,
    TOKEN_VALUES,
    TOKEN_UPDATE,
    TOKEN_SET,

    TOKEN_DELETE,
    TOKEN_CREATE,
    TOKEN_DROP,
    TOKEN_TABLE,
    TOKEN_INDEX,
    TOKEN_IF,
    TOKEN_EXISTS,

    TOKEN_BEGIN,
    TOKEN_COMMIT,
    TOKEN_ROLLBACK,
    TOKEN_TRANSACTION,

    TOKEN_ORDER,
    TOKEN_BY,
    TOKEN_ASC,
    TOKEN_DESC,
    TOKEN_LIMIT,
    TOKEN_OFFSET,
    TOKEN_DISTINCT,
    TOKEN_AS,
    TOKEN_ON,
    TOKEN_PRIMARY,
    TOKEN_KEY,
    TOKEN_UNIQUE,
    TOKEN_DEFAULT,
    TOKEN_AUTOINCREMENT,

    // Data types
    TOKEN_INTEGER_KW,       // INTEGER keyword
    TOKEN_REAL,             // REAL keyword
    TOKEN_TEXT,             // TEXT keyword
    TOKEN_BLOB_KW,          // BLOB keyword

    // Special tokens
    TOKEN_LPAREN,           // (
    TOKEN_RPAREN,           // )
    TOKEN_COMMA,             // ,
    TOKEN_DOT,               // .
    TOKEN_SEMICOLON,        // ;
    TOKEN_COLON,            // :
    TOKEN_QUESTION,         // ?
    TOKEN_PLACEHOLDER,      // $1, $2, etc.

    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    union {
        int64_t integer_value;
        double float_value;
        struct {
            char* value;
            size_t length;
        } string_value;
    } value;
    char* text;              // Original text of token
    size_t text_length;
    int line;
    int column;
} Token;
```

### 5.2 AST Node Hierarchy

```c
// File: src/parser/ast.h

// Base AST node
typedef struct AstNode {
    AstNodeType type;
    int ref_count;          // For memory management
} AstNode;

// Statement types
typedef enum {
    AST_STMT_SELECT,
    AST_STMT_INSERT,
    AST_STMT_UPDATE,
    AST_STMT_DELETE,
    AST_STMT_CREATE_TABLE,
    AST_STMT_DROP_TABLE,
    AST_STMT_CREATE_INDEX,
    AST_STMT_DROP_INDEX,
    AST_STMT_BEGIN,
    AST_STMT_COMMIT,
    AST_STMT_ROLLBACK
} AstNodeType;

// SELECT statement
typedef struct {
    AstNode base;
    // Column list
    ColumnList* columns;      // NULL for *, list otherwise
    bool is_distinct;
    // FROM clause
    char* table_name;
    char* alias;
    // WHERE clause
    Expression* where;
    // ORDER BY
    OrderByList* order_by;
    // LIMIT
    Expression* limit;
    // OFFSET
    Expression* offset;
} SelectStatement;

// INSERT statement
typedef struct {
    AstNode base;
    char* table_name;
    ColumnList* columns;      // NULL if not specified
    ValueList* values;        // List of value lists
} InsertStatement;

// UPDATE statement
typedef struct {
    AstNode base;
    char* table_name;
    SetClause* set_clauses;
    Expression* where;
} UpdateStatement;

// DELETE statement
typedef struct {
    AstNode base;
    char* table_name;
    Expression* where;
} DeleteStatement;

// CREATE TABLE statement
typedef struct {
    AstNode base;
    char* table_name;
    bool if_not_exists;
    ColumnDefinition* columns;  // Linked list
} CreateTableStatement;

// CREATE INDEX statement
typedef struct {
    AstNode base;
    char* index_name;
    char* table_name;
    char* column_name;
    bool unique;
} CreateIndexStatement;

// Column list
typedef struct ColumnList {
    char* name;
    char* alias;
    struct ColumnList* next;
} ColumnList;

// Column definition (for CREATE TABLE)
typedef struct ColumnDefinition {
    char* name;
    ColumnType type;
    bool not_null;
    bool primary_key;
    bool autoincrement;
    Expression* default_value;
    struct ColumnDefinition* next;
} ColumnDefinition;

typedef enum {
    COL_TYPE_INTEGER,
    COL_TYPE_REAL,
    COL_TYPE_TEXT,
    COL_TYPE_BLOB
} ColumnType;

// Value list (for INSERT)
typedef struct ValueList {
    ExpressionList* values;
    struct ValueList* next;
} ValueList;

// ORDER BY list
typedef struct OrderByList {
    char* column_name;
    bool descending;
    struct OrderByList* next;
} OrderByList;
```

### 5.3 Expression AST

```c
// File: src/parser/expr.h

typedef enum {
    EXPR_LITERAL_INTEGER,
    EXPR_LITERAL_FLOAT,
    EXPR_LITERAL_STRING,
    EXPR_LITERAL_BLOB,
    EXPR_LITERAL_NULL,

    EXPR_COLUMN,            // Column reference
    EXPR_PARAMETER,         // ? or $n

    EXPR_BINARY_OP,         // a + b, a = b, etc.
    EXPR_UNARY_OP,          // -a, NOT a

    EXPR_LIKE,              // LIKE pattern matching
    EXPR_IN,                // IN (list) or IN (subquery)
    EXPR_BETWEEN,           // BETWEEN a AND b

    EXPR_CASE,              // CASE ... WHEN ... END
    EXPR_CAST               // CAST(expr AS type)
} ExpressionType;

// Expression node
typedef struct Expression {
    ExpressionType type;
    union {
        int64_t literal_integer;
        double literal_float;
        struct {
            char* value;
            size_t length;
        } literal_string;
        struct {
            uint8_t* data;
            size_t length;
        } literal_blob;

        struct {
            char* table_name;
            char* column_name;
        } column_ref;

        int parameter_index;

        struct {
            BinaryOp op;
            struct Expression* left;
            struct Expression* right;
        } binary;

        struct {
            UnaryOp op;
            struct Expression* operand;
        } unary;

        struct {
            struct Expression* value;
            struct Expression* min;
            struct Expression* max;
        } between;

        struct {
            struct Expression* value;
            struct ExpressionList* list;
        } in;
    } value;
} Expression;

// Binary operators
typedef enum {
    BINOP_PLUS,
    BINOP_MINUS,
    BINOP_STAR,
    BINOP_SLASH,
    BINOP_MOD,
    BINOP_EQ,
    BINOP_NEQ,
    BINOP_LT,
    BINOP_GT,
    BINOP_LTE,
    BINOP_GTE,
    BINOP_AND,
    BINOP_OR,
    BINOP_BITAND,
    BINOP_BITOR,
    BINOP_BITXOR,
    BINOP_LIKE,
    BINOP_GLOB
} BinaryOp;

// Unary operators
typedef enum {
    UNOP_NOT,
    UNOP_MINUS,
    UNOP_BITNOT
} UnaryOp;

// Expression list for IN lists, VALUES, etc.
typedef struct ExpressionList {
    Expression* expr;
    struct ExpressionList* next;
} ExpressionList;
```

### 5.4 Expression Precedence

```c
// File: src/parser/parser.c

// Operator precedence (lowest to highest, as in SQLite)
static const int PRECEDENCE[] = {
    0,  // LOWEST
    1,  // COMMA
    2,  // ID, AS, ON
    3,  // OR
    4,  // AND
    5,  // NOT
    6,  // IN, LIKE, BETWEEN, GLOB
    7,  // <, <=, >, >=, =, !=, <>
    8,  // |
    9,  // &
    10, // <<, >>
    11, // +, -
    12, // *, /, %
    13, // UNARY (+, -, ~)
    14, // . (column qualifier)
    15  // HIGHEST
};

// Precedence climbing parser for expressions:
// parse_expression(min_prec):
//   left = parse_primary()
//   while (next token has higher precedence):
//       op = next token
//       advance
//       right = parse_expression(op.precedence + (op is left-associative ? 0 : 1))
//       left = binary_node(op, left, right)
//   return left
```

### 5.5 Parser State Machine

```c
// File: src/parser/parser.h

typedef struct {
    // Tokenizer state
    Lexer* lexer;
    Token current_token;
    Token peek_token;

    // Source position for error reporting
    const char* source;
    size_t source_length;

    // Error state
    Error* error;
    bool has_error;

    // Memory management
    MemoryArena* arena;      // For AST allocation
} Parser;

// Parser functions
Parser* parser_create(const char* sql, size_t length);
void parser_destroy(Parser* parser);
Statement* parser_parse_statement(Parser* parser);
Expression* parser_parse_expression(Parser* parser);
void parser_free_ast(Parser* parser, AstNode* node);

// Entry point
Statement* sql_parse(const char* sql, size_t length) {
    Parser* parser = parser_create(sql, length);
    Statement* stmt = parser_parse_statement(parser);
    if (parser->has_error) {
        // Handle parse error
        return NULL;
    }
    parser_destroy(parser);
    return stmt;
}
```

---

## 6. Query Execution Architecture

### 6.1 Executor Overview

```c
// File: src/executor/executor.h

typedef struct {
    Storage* storage;
    Catalog* catalog;
    Transaction* transaction;
    MemoryArena* arena;
    bool eof;
    int changes;              // Number of rows modified
} Executor;

// Executor lifecycle
Executor* executor_create(Storage* storage, Catalog* catalog, Transaction* tx);
void executor_destroy(Executor* exec);
Result* executor_execute(Executor* exec, Statement* stmt);
void executor_reset(Executor* exec);

// Result structure
typedef struct {
    bool success;
    bool has_rows;            // True if result contains rows (SELECT)
    ColumnInfo* columns;      // Column metadata
    size_t column_count;
    RowBuffer* rows;          // Result rows
    size_t row_count;
    int64_t changes;          // Rows affected (INSERT/UPDATE/DELETE)
    Error* error;
} Result;

typedef struct {
    char* name;
    ColumnType type;
    size_t size;
} ColumnInfo;
```

### 6.2 SELECT Execution Pipeline

```c
// File: src/executor/select.h

// SELECT execution stages:
// 1. Open cursor on table/index
// 2. Apply WHERE filter
// 3. Sort if ORDER BY
// 4. Apply LIMIT/OFFSET
// 5. Format output columns

typedef struct {
    Executor* exec;
    Cursor* cursor;           // Table scan cursor
    Expression* where;        // WHERE clause
    ColumnList* columns;      // Output columns
    OrderByList* order_by;    // Sort specification
    Expression* limit;
    Expression* offset;
    bool distinct;
    bool is_open;
} SelectCursor;

// Execution function
Result* exec_select(Executor* exec, SelectStatement* stmt) {
    // 1. Get table metadata
    Table* table = catalog_get_table(exec->catalog, stmt->table_name);
    if (!table) return error_result("Table not found");

    // 2. Open table cursor
    Cursor* cursor = storage_cursor_open(exec->storage, table);
    defer(storage_cursor_close(cursor));

    // 3. Build select context
    SelectContext ctx = {
        .exec = exec,
        .table = table,
        .cursor = cursor,
        .where = stmt->where,
        .columns = stmt->columns,
        .order_by = stmt->order_by,
        .limit = stmt->limit,
        .offset = stmt->offset,
        .distinct = stmt->is_distinct,
    };

    // 4. Execute scan with filters
    Result* result = select_execute(&ctx);

    return result;
}

// Cursor for table scan
typedef struct {
    uint32_t page_num;
    uint32_t cell_index;
    bool at_end;
    uint8_t* current_row;
} Cursor;
```

### 6.3 WHERE Clause Evaluation

```c
// File: src/parser/expr.c

// Expression evaluation for WHERE clauses
bool eval_expression(Expression* expr, RowView* row, EvalContext* ctx) {
    switch (expr->type) {
        case EXPR_LITERAL_INTEGER:
            return expr->value.literal_integer;

        case EXPR_LITERAL_FLOAT:
            return expr->value.literal_float != 0.0;

        case EXPR_LITERAL_STRING:
            return expr->value.literal_string.length > 0;

        case EXPR_LITERAL_NULL:
            return false;

        case EXPR_COLUMN: {
            Value val = row_get_value(row, expr->value.column_ref.column_name);
            return value_is_truthy(&val);
        }

        case EXPR_BINARY_OP:
            return eval_binary_op(expr, row, ctx);

        case EXPR_UNARY_OP:
            return eval_unary_op(expr, row, ctx);

        case EXPR_LIKE:
            return eval_like(expr, row, ctx);

        case EXPR_IN:
            return eval_in(expr, row, ctx);

        case EXPR_BETWEEN:
            return eval_between(expr, row, ctx);

        default:
            return false;
    }
}

// Binary operator evaluation
bool eval_binary_op(Expression* expr, RowView* row, EvalContext* ctx) {
    Value left = eval_value(expr->value.binary.left, row, ctx);
    Value right = eval_value(expr->value.binary.right, row, ctx);

    switch (expr->value.binary.op) {
        case BINOP_EQ:
            return value_equal(&left, &right);
        case BINOP_NEQ:
            return !value_equal(&left, &right);
        case BINOP_LT:
            return value_compare(&left, &right) < 0;
        case BINOP_GT:
            return value_compare(&left, &right) > 0;
        case BINOP_LTE:
            return value_compare(&left, &right) <= 0;
        case BINOP_GTE:
            return value_compare(&left, &right) >= 0;
        case BINOP_AND:
            return value_is_truthy(&left) && value_is_truthy(&right);
        case BINOP_OR:
            return value_is_truthy(&left) || value_is_truthy(&right);
        case BINOP_PLUS:
            return value_add(&left, &right);
        // ... etc
        default:
            return false;
    }
}
```

### 6.4 INSERT/UPDATE/DELETE Execution

```c
// File: src/executor/insert.h

Result* exec_insert(Executor* exec, InsertStatement* stmt) {
    Table* table = catalog_get_table(exec->catalog, stmt->table_name);
    if (!table) return error_result("Table not found");

    // Validate column count
    size_t expected_cols = stmt->columns ? stmt->columns->count : table->column_count;
    if (expected_cols != table->column_count) {
        return error_result("Column count mismatch");
    }

    int64_t rows_inserted = 0;

    // Process each VALUES row
    ValueList* values = stmt->values;
    while (values) {
        // Convert expressions to values
        ValueList* parsed_values = eval_value_list(values->values, exec->arena);

        // Insert row
        storage_insert_row(exec->storage, table, parsed_values);
        rows_inserted++;

        values = values->next;
    }

    // Return success
    return success_result(0, rows_inserted);
}

// File: src/executor/update.h

Result* exec_update(Executor* exec, UpdateStatement* stmt) {
    Table* table = catalog_get_table(exec->catalog, stmt->table_name);
    if (!table) return error_result("Table not found");

    // WHERE is required for UPDATE
    if (!stmt->where) {
        return error_result("UPDATE requires WHERE clause");
    }

    // Open cursor for scanning
    Cursor* cursor = storage_cursor_open(exec->storage, table);
    defer(storage_cursor_close(cursor));

    int64_t rows_updated = 0;

    // Scan table
    while (storage_cursor_next(cursor)) {
        RowView* row = cursor_get_row(cursor);

        // Evaluate WHERE
        if (!eval_expression(stmt->where, row, NULL)) {
            continue;
        }

        // Apply SET clauses
        for (SetClause* clause = stmt->set_clauses; clause; clause = clause->next) {
            Value new_value = eval_expression(clause->value, row, NULL);
            row_set_value(row, clause->column_name, &new_value);
        }

        // Write updated row
        storage_update_row(exec->storage, cursor);
        rows_updated++;
    }

    return success_result(0, rows_updated);
}

// File: src/executor/delete.h

Result* exec_delete(Executor* exec, DeleteStatement* stmt) {
    Table* table = catalog_get_table(exec->catalog, stmt->table_name);
    if (!table) return error_result("Table not found");

    // WHERE is required for DELETE
    if (!stmt->where) {
        return error_result("DELETE requires WHERE clause");
    }

    Cursor* cursor = storage_cursor_open(exec->storage, table);
    defer(storage_cursor_close(cursor));

    int64_t rows_deleted = 0;

    while (storage_cursor_next(cursor)) {
        RowView* row = cursor_get_row(cursor);

        if (!eval_expression(stmt->where, row, NULL)) {
            continue;
        }

        storage_delete_row(exec->storage, cursor);
        rows_deleted++;
    }

    return success_result(0, rows_deleted);
}
```

---

## 7. Client-Server Protocol

### 7.1 Protocol Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     Client-Server Protocol Flow                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   Client                          Server                                    │
│     │                               │                                        │
│     │──── Connect Request ─────────>│                                        │
│     │                               │                                        │
│     │<─── Connection ACK ────────────│                                        │
│     │                               │                                        │
│     │──── SQL Request ─────────────>│                                        │
│     │      (text SQL + metadata)    │                                        │
│     │                               │                                        │
│     │      [Execute Query]         │                                        │
│     │      [Access Storage]        │                                        │
│     │      [Return Results]         │                                        │
│     │                               │                                        │
│     │<─── Response Header ────────────│                                        │
│     │      (status, columns, rows)  │                                        │
│     │                               │                                        │
│     │<─── Row Data (N times) ────────│                                        │
│     │      (tab-separated values)   │                                        │
│     │                               │                                        │
│     │<─── Response End ──────────────│                                        │
│     │      (OK n / ERROR: msg)       │                                        │
│     │                               │                                        │
│     │──── Disconnect ───────────────>│                                        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 7.2 Request Format

```c
// File: src/protocol/message.h

// Request message types
typedef enum {
    REQUEST_EXECUTE,           // Execute SQL statement
    REQUEST_PING,               // Health check
    REQUEST_SHUTDOWN,           // Server shutdown
    REQUEST_BEGIN,              // Explicit transaction start
    REQUEST_COMMIT,             // Commit transaction
    REQUEST_ROLLBACK            // Rollback transaction
} RequestType;

// Request header (8 bytes)
typedef struct __attribute__((packed)) {
    // Offset 0: Request type (1 byte)
    RequestType type;

    // Offset 1: Flags (1 byte)
    // Bit 0: sync - wait for completion
    // Bit 1: read_only - read-only transaction
    uint8_t flags;

    // Offset 2-3: Request body length (2 bytes)
    uint16_t body_length;

    // Offset 4-7: Request ID for correlation (4 bytes)
    uint32_t request_id;
} RequestHeader;

// Execute request body (variable length):
// [SQL length: 4 bytes]
// [SQL text: n bytes]
// [Parameter count: 2 bytes, optional]
// [Parameters: n bytes, optional]

typedef struct __attribute__((packed)) {
    uint32_t sql_length;
    char sql_data[];           // Variable length
    // Followed by parameter data if present
} ExecuteRequestBody;
```

### 7.3 Response Format

```c
// File: src/protocol/message.h

// Response message types
typedef enum {
    RESPONSE_OK,               // Success with changes count
    RESPONSE_ROWS,             // Success with row data
    RESPONSE_ERROR,            // Error occurred
    RESPONSE_BUSY,             // Database locked
    RESPONSE_SHUTDOWN_ACK      // Shutdown acknowledgment
} ResponseType;

// Response header (12 bytes)
typedef struct __attribute__((packed)) {
    // Offset 0: Response type (1 byte)
    ResponseType type;

    // Offset 1: Flags (1 byte)
    // Bit 0: has_error - error message follows
    uint8_t flags;

    // Offset 2-3: Column count (2 bytes) - for ROWS response
    uint16_t column_count;

    // Offset 4-7: Row count (4 bytes) - for ROWS response
    uint32_t row_count;

    // Offset 8-11: Changes count (4 bytes) - for OK response
    uint32_t changes;
} ResponseHeader;

// Row data format (tab-separated):
// Column1\tColumn2\tColumn3\n
// Values are escaped:
//   - Tab replaced with \t
//   - Newline replaced with \n
//   - Backslash replaced with \\
//   - NULL represented as empty field

// Error response body:
typedef struct __attribute__((packed)) {
    uint32_t error_code;
    uint16_t error_message_length;
    char error_message[];       // Variable length
} ErrorResponseBody;
```

### 7.4 Protocol Constants

```c
// File: src/protocol/protocol.h

#define PROTOCOL_VERSION 1

// Socket paths
#define DEFAULT_SOCKET_PATH "/run/tinydb/tinydb.sock"
#define ALTERNATE_SOCKET_PATH "/var/run/tinydb/tinydb.sock"

// Protocol limits
#define MAX_SQL_LENGTH 1024 * 1024  // 1MB
#define MAX_ROWS_IN_RESPONSE 10000
#define MAX_COLUMN_NAME_LENGTH 64
#define MAX_ERROR_MESSAGE_LENGTH 256

// Timeout values (in seconds)
#define DEFAULT_TIMEOUT 30
#define QUERY_TIMEOUT 300
#define LOCK_TIMEOUT 10

// Protocol magic bytes for validation
#define CLIENT_MAGIC 0x54494E59  // 'TINY'
#define SERVER_MAGIC 0x54494E44  // 'TIND'

// Escaped value markers
#define ESCAPE_CHAR '\\'
#define TAB_ESCAPE 't'
#define NEWLINE_ESCAPE 'n'
#define NULL_MARKER '\x00'
```

### 7.5 Error Codes

```c
// File: src/util/error.h

typedef enum {
    ERR_NONE = 0,

    // Parse errors (1xxx)
    ERR_PARSE_SYNTAX = 1000,
    ERR_PARSE_UNEXPECTED_TOKEN,
    ERR_PARSE_UNTERMINATED_STRING,
    ERR_PARSE_INVALID_NUMBER,
    ERR_PARSE_INVALID_IDENTIFIER,

    // Execution errors (2xxx)
    ERR_EXEC_TABLE_NOT_FOUND = 2000,
    ERR_EXEC_COLUMN_NOT_FOUND,
    ERR_EXEC_DUPLICATE_COLUMN,
    ERR_EXEC_TYPE_MISMATCH,
    ERR_EXEC_NOT_NULL_VIOLATION,
    ERR_EXEC_CONSTRAINT_VIOLATION,
    ERR_EXEC_FOREIGN_KEY_VIOLATION,
    ERR_EXEC_UNIQUE_VIOLATION,

    // Transaction errors (3xxx)
    ERR_TX_ALREADY_ACTIVE = 3000,
    ERR_TX_NOT_ACTIVE,
    ERR_TX_LOCK_TIMEOUT,
    ERR_TX_DEADLOCK,
    ERR_TX_ROLLBACK,

    // Storage errors (4xxx)
    ERR_STORAGE_IO = 4000,
    ERR_STORAGE_CORRUPT,
    ERR_STORAGE_FULL,
    ERR_STORAGE_PERMISSION,
    ERR_STORAGE_NOT_FOUND,

    // Protocol errors (5xxx)
    ERR_PROTO_INVALID_REQUEST = 5000,
    ERR_PROTO_CONNECTION_CLOSED,
    ERR_PROTO_TIMEOUT,

    // Internal errors (9xxx)
    ERR_INTERNAL = 9000,
    ERR_OUT_OF_MEMORY,
    ERR_ASSERTION_FAILED
} ErrorCode;

// Error structure
typedef struct {
    ErrorCode code;
    char message[256];
    char context[256];          // Additional context
    const char* file;          // Source file
    int line;                  // Source line
} Error;

// Error creation macros
#define ERROR_CREATE(code, msg) \
    (Error) { .code = code, .message = msg, .file = __FILE__, .line = __LINE__ }

#define ERROR_CREATE_FMT(code, fmt, ...) \
    (Error) { .code = code, .message = "", .file = __FILE__, .line = __LINE__, \
               .context = "" }
```

---

## 8. Error Handling Strategy

### 8.1 Error Propagation Model

```c
// File: src/util/error.h

// All functions that can fail return Error or Result
typedef Error* (*ErrorableFunction)(void);

// Result type for query execution
typedef struct {
    bool success;
    union {
        struct {
            ColumnInfo* columns;
            size_t column_count;
            RowBuffer* rows;
            size_t row_count;
        } data;
        struct {
            int64_t changes;
        } modification;
    } value;
    Error* error;              // Non-NULL if !success
} QueryResult;

// Error chain for nested operations
typedef struct ErrorChain {
    Error* error;
    struct ErrorChain* parent;
} ErrorChain;

// Example error propagation:
Error* storage_read_page(Storage* storage, uint32_t page_num, Page* page) {
    Error* err = file_read_at(storage->fd, page_num * PAGE_SIZE,
                              page->data, PAGE_SIZE);
    if (err) {
        return error_wrap(err, "Failed to read page %u", page_num);
    }
    return NULL;
}

// Error wrapping helper
Error* error_wrap(Error* cause, const char* fmt, ...) {
    Error* wrapped = error_create(cause->code, "");
    snprintf(wrapped->context, sizeof(wrapped->context), fmt, ...);
    wrapped->cause = cause;
    return wrapped;
}
```

### 8.2 Panic and Recovery

```c
// File: src/util/error.h

// Panic for unrecoverable errors (assertions, corruption)
typedef enum {
    PANIC_NONE,
    PANIC_IO_ERROR,
    PANIC_CORRUPTION,
    PANIC_OUT_OF_MEMORY,
    PANIC_ASSERTION
} PanicType;

typedef struct {
    PanicType type;
    const char* message;
    const char* file;
    int line;
} PanicInfo;

// Global panic state
extern volatile PanicInfo g_panic;

void panic(const char* fmt, ...) __attribute__((noreturn));
void panic_at(const char* file, int line, const char* fmt, ...) __attribute__((noreturn));

// Panic recovery in server
void handle_panic(Executor* exec) {
    if (g_panic.type != PANIC_NONE) {
        // Log panic
        log_error("PANIC: %s at %s:%d", g_panic.message, g_panic.file, g_panic.line);

        // Rollback active transaction
        if (exec->transaction && exec->transaction->is_active) {
            transaction_rollback(exec->transaction);
        }

        // Reset panic state
        g_panic.type = PANIC_NONE;

        // Return error to client
        send_error_response("Internal server error");
    }
}

// Assertion macro
#define ASSERT(cond, msg) \
    ((cond) ? (void)0 : \
     panic_at(__FILE__, __LINE__, "Assertion failed: %s", msg))
```

### 8.3 Transaction Error Handling

```c
// File: src/tx/transaction.h

typedef enum {
    TX_STATE_IDLE,
    TX_STATE_ACTIVE,
    TX_STATE_COMMITTING,
    TX_STATE_ROLLBACK,
    TX_STATE_FINALIZED
} TransactionState;

typedef struct {
    uint64_t id;              // Transaction ID
    TransactionState state;
    uint64_t start_time;
    uint64_t commit_time;
    uint64_t undo_log_size;    // Undo log entries
    uint32_t lock_count;
    bool is_read_only;
} Transaction;

// Transaction error handling
Error* transaction_begin(TransactionManager* mgr, Transaction** tx) {
    if (mgr->active_tx) {
        return ERROR_CREATE(ERR_TX_ALREADY_ACTIVE,
                           "Transaction already active");
    }

    Transaction* new_tx = memory_alloc(sizeof(Transaction));
    new_tx->id = mgr->next_tx_id++;
    new_tx->state = TX_STATE_ACTIVE;
    new_tx->start_time = time_now();
    new_tx->lock_count = 0;
    new_tx->is_read_only = false;

    mgr->active_tx = new_tx;
    *tx = new_tx;

    return NULL;
}

Error* transaction_commit(Transaction* tx) {
    if (tx->state != TX_STATE_ACTIVE) {
        return ERROR_CREATE(ERR_TX_NOT_ACTIVE, "Transaction not active");
    }

    tx->state = TX_STATE_COMMITTING;

    // Write WAL commit marker
    Error* err = wal_commit(tx->id);
    if (err) {
        tx->state = TX_STATE_ROLLBACK;
        return error_wrap(err, "WAL commit failed");
    }

    tx->state = TX_STATE_FINALIZED;
    tx->commit_time = time_now();

    return NULL;
}

Error* transaction_rollback(Transaction* tx) {
    if (tx->state == TX_STATE_IDLE) {
        return NULL;  // Nothing to rollback
    }

    tx->state = TX_STATE_ROLLBACK;

    // Apply undo log
    Error* err = apply_undo_log(tx->undo_log);
    if (err) {
        return error_wrap(err, "Rollback failed");
    }

    tx->state = TX_STATE_FINALIZED;

    return NULL;
}
```

---

## 9. Memory Management

### 9.1 Memory Arena

```c
// File: src/util/memory.h

// Arena-based allocation for AST and transient data
typedef struct MemoryArena {
    uint8_t* base;            // Base of memory region
    size_t size;              // Total arena size
    size_t used;              // Currently used bytes
    size_t high_water;        // Peak usage
    struct MemoryArena* parent;
} MemoryArena;

// Arena creation
MemoryArena* arena_create(size_t initial_size) {
    MemoryArena* arena = malloc(sizeof(MemoryArena));
    arena->base = malloc(initial_size);
    arena->size = initial_size;
    arena->used = 0;
    arena->high_water = 0;
    arena->parent = NULL;
    return arena;
}

// Arena allocation
void* arena_alloc(MemoryArena* arena, size_t size, size_t align) {
    // Align current position
    size_t aligned = (arena->used + align - 1) & ~(align - 1);

    // Check for overflow
    if (aligned + size > arena->size) {
        return NULL;  // Out of memory
    }

    void* ptr = arena->base + aligned;
    arena->used = aligned + size;

    if (arena->used > arena->high_water) {
        arena->high_water = arena->used;
    }

    return ptr;
}

// Arena reset (for query reuse)
void arena_reset(MemoryArena* arena) {
    arena->used = 0;
}

// Arena destruction
void arena_destroy(MemoryArena* arena) {
    if (arena->parent) {
        // Child arena, mark as freed
        arena->used = 0;
    } else {
        // Root arena, free memory
        free(arena->base);
        free(arena);
    }
}
```

### 9.2 Reference Counting for AST Nodes

```c
// File: src/parser/ast.h

// Reference counting for AST nodes
void ast_retain(AstNode* node) {
    node->ref_count++;
}

void ast_release(AstNode* node) {
    if (node == NULL) return;

    node->ref_count--;
    if (node->ref_count <= 0) {
        // Recursively free children
        switch (node->type) {
            case AST_STMT_SELECT:
                ast_release((AstNode*)((SelectStatement*)node)->where);
                break;
            case AST_STMT_INSERT:
                // ... free children
                break;
            // ... other types
        }
        free(node);
    }
}
```

### 9.3 Memory Cleanup Strategy

```c
// File: src/executor/executor.c

// Per-query memory management
Result* executor_execute(Executor* exec, Statement* stmt) {
    // Create temporary arena for this query
    MemoryArena* query_arena = arena_create(64 * 1024);  // 64KB initial

    Result* result = execute_internal(exec, stmt, query_arena);

    // Clean up query arena
    arena_destroy(query_arena);

    return result;
}

// Statement cleanup
void statement_destroy(Statement* stmt) {
    switch (stmt->type) {
        case STMT_SELECT:
            destroy_select((SelectStatement*)stmt);
            break;
        case STMT_INSERT:
            destroy_insert((InsertStatement*)stmt);
            break;
        // ... other types
    }
}
```

---

## 10. Testing Strategy

### 10.1 Test Structure

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          Test Pyramid                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                           ┌─────────────┐                                   │
│                           │   E2E Tests │                                   │
│                           │   (~5 tests) │                                  │
│                           └─────────────┘                                   │
│                                  │                                          │
│                           ┌─────────────┐                                   │
│                           │ Integration │                                  │
│                           │   Tests     │                                  │
│                           │ (~50 tests) │                                  │
│                           └─────────────┘                                   │
│                                  │                                          │
│                           ┌─────────────┐                                   │
│                           │  Unit Tests │                                  │
│                           │ (~200 tests)│                                  │
│                           └─────────────┘                                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 10.2 Test Framework (No External Dependencies)

```c
// File: tests/harness/harness.h

// Simple test framework for pure C
#define TEST_SUITE(name) \
    void test_suite_##name(TestContext* ctx); \
    void register_suite_##name(TestRegistry* reg) { \
        test_registry_add(reg, #name, test_suite_##name); \
    } \
    void test_suite_##name(TestContext* ctx)

#define TEST(name) \
    void test_##name(TestContext* ctx); \
    static void __attribute__((constructor)) register_test_##name() { \
        current_test = #name; \
        test_registry_add_test(#name, test_##name); \
    } \
    void test_##name(TestContext* ctx)

// Test context
typedef struct {
    MemoryArena* arena;
    Storage* storage;
    Catalog* catalog;
    int passed;
    int failed;
    char* current_test;
} TestContext;

// Test assertions
#define ASSERT_TRUE(cond) \
    test_assert(ctx, cond, "Expected true", #cond, __FILE__, __LINE__)

#define ASSERT_FALSE(cond) \
    test_assert(ctx, !(cond), "Expected false", #cond, __FILE__, __LINE__)

#define ASSERT_EQ(a, b) \
    test_assert_eq(ctx, a, b, #a " == " #b, __FILE__, __LINE__)

#define ASSERT_STREQ(a, b) \
    test_assert_str(ctx, a, b, __FILE__, __LINE__)

#define ASSERT_NULL(ptr) \
    test_assert(ctx, ptr == NULL, "Expected NULL", #ptr, __FILE__, __LINE__)

#define ASSERT_NOT_NULL(ptr) \
    test_assert(ctx, ptr != NULL, "Expected non-NULL", #ptr, __FILE__, __LINE__)
```

### 10.3 Unit Test Categories

```c
// File: tests/unit/test_lexer.c

// Lexer tests
TEST_SUITE(lexer) {
    TEST(test_integer_tokens) {
        TokenizeResult result = tokenize("123 0xFF -456");
        ASSERT_EQ(result.count, 4);  // 123, 0xFF, -456, EOF
    }

    TEST(test_string_tokens) {
        TokenizeResult result = tokenize("'hello' \"world\"");
        ASSERT_EQ(result.tokens[0].type, TOKEN_STRING);
        ASSERT_STREQ(result.tokens[0].value.string, "hello");
    }

    TEST(test_keyword_tokens) {
        TokenizeResult result = tokenize("SELECT * FROM t WHERE x = 1");
        ASSERT_EQ(result.tokens[0].type, TOKEN_SELECT);
        ASSERT_EQ(result.tokens[1].type, TOKEN_STAR);
        ASSERT_EQ(result.tokens[2].type, TOKEN_FROM);
    }

    TEST(test_operator_tokens) {
        TokenizeResult result = tokenize("= <> != < > <= >=");
        ASSERT_EQ(result.tokens[0].type, TOKEN_EQ);
        ASSERT_EQ(result.tokens[1].type, TOKEN_NEQ);
        // ...
    }
}

// File: tests/unit/test_parser.c

// Parser tests
TEST_SUITE(parser) {
    TEST(test_select_parse) {
        Statement* stmt = sql_parse("SELECT * FROM users WHERE id = 1");
        ASSERT_NOT_NULL(stmt);
        ASSERT_EQ(stmt->type, AST_STMT_SELECT);
    }

    TEST(test_insert_parse) {
        Statement* stmt = sql_parse("INSERT INTO t VALUES (1, 'hello')");
        ASSERT_NOT_NULL(stmt);
        ASSERT_EQ(stmt->type, AST_STMT_INSERT);
    }
}

// File: tests/unit/test_btree.c

// B+Tree tests
TEST_SUITE(btree) {
    TEST(test_insert_and_search) {
        BTree* tree = btree_create(storage);
        btree_insert(tree, "key1", row_data_1);
        btree_insert(tree, "key2", row_data_2);

        Result* r = btree_search(tree, "key1");
        ASSERT_NOT_NULL(r);
        ASSERT_TRUE(value_equals(r->value, row_data_1));
    }

    TEST(test_range_query) {
        BTree* tree = btree_create(storage);
        for (int i = 0; i < 100; i++) {
            char key[16];
            snprintf(key, sizeof(key), "key%03d", i);
            btree_insert(tree, key, value_for(i));
        }

        Results* results = btree_range(tree, "key050", "key060");
        ASSERT_EQ(results->count, 11);  // 50 to 60 inclusive
    }

    TEST(test_delete) {
        // ...
    }
}
```

### 10.4 Integration Tests

```c
// File: tests/integration/test_crud.c

TEST_SUITE(crud_integration) {
    TEST(test_insert_and_select) {
        TestContext ctx = test_context_create();

        // Create table
        execute(&ctx, "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");

        // Insert rows
        execute(&ctx, "INSERT INTO users VALUES (1, 'Alice')");
        execute(&ctx, "INSERT INTO users VALUES (2, 'Bob')");

        // Verify with SELECT
        Result* result = execute(&ctx, "SELECT * FROM users WHERE id = 1");
        ASSERT_EQ(result->row_count, 1);
        ASSERT_STREQ(result->rows[0].name, "Alice");

        test_context_destroy(ctx);
    }

    TEST(test_update_with_where) {
        // ...
    }

    TEST(test_delete_with_where) {
        // ...
    }

    TEST(test_transaction_commit) {
        // ...
    }

    TEST(test_transaction_rollback) {
        // ...
    }
}
```

### 10.5 Test Coverage Targets

```c
// File: Makefile

# Coverage targets
COVERAGE_TARGET = 80%

# Test categories and targets
UNIT_TESTS = tests/unit/test_lexer tests/unit/test_parser \
             tests/unit/test_btree tests/unit/test_wal \
             tests/unit/test_storage tests/unit/test_executor

INTEGRATION_TESTS = tests/integration/test_crud \
                    tests/integration/test_transactions \
                    tests/integration/test_schema \
                    tests/integration/test_recovery

E2E_TESTS = tests/e2e/test_cli_full

# Coverage report generation
coverage: clean
    gcovr --html-details coverage.html
    gcovr --sonarqube coverage.xml

# Minimum coverage check
check-coverage:
    @COVERAGE=$$(gcovr --text-summary | grep " coverage:" | awk '{print $$1}' | tr -d '%')
    @if [ $$COVERAGE -lt 80 ]; then \
        echo "Coverage $$COVERAGE% is below target 80%"; \
        exit 1; \
    fi
```

### 10.6 Test Execution

```bash
#!/bin/bash
# scripts/test.sh

set -e

echo "=== Running Unit Tests ==="
for test in tests/unit/test_*; do
    echo "Running $test"
    $test
done

echo "=== Running Integration Tests ==="
for test in tests/integration/test_*; do
    echo "Running $test"
    $test
done

echo "=== Running E2E Tests ==="
./tests/e2e/test_cli_full

echo "=== All Tests Passed ==="
```

---

## Appendix A: Type Definitions

```c
// File: include/types.h

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Fixed-width integer types
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Boolean type
typedef int8_t   bool8;

// Size type
typedef size_t   usize;

// Page number type (up to 2^31 pages)
typedef uint32_t pagenum_t;

// Row ID type
typedef uint32_t rowid_t;

// Common size constants
#define PAGE_SIZE 4096
#define MAX_ROW_SIZE (PAGE_SIZE - 100)  // Leave room for cell overhead
#define MAX_COLUMNS 64
#define MAX_TABLE_NAME 64
#define MAX_COLUMN_NAME 64
#define MAX_INDEX_NAME 64
```

## Appendix B: File Header Magic Numbers

| File | Magic | ASCII |
|------|-------|-------|
| Database | 0x54494E59 | 'TINY' |
| WAL | 0x377F0682 | (binary) |

## Appendix C: Error Code Ranges

| Range | Category |
|-------|----------|
| 1000-1999 | Parse errors |
| 2000-2999 | Execution errors |
| 3000-3999 | Transaction errors |
| 4000-4999 | Storage errors |
| 5000-5999 | Protocol errors |
| 9000-9999 | Internal errors |

---

