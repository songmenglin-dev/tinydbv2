# API Design

## Module Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         CLI Client                              │
│  - main.c, cli.c, commands.c                                     │
└──────────────┬──────────────────────────────────────────────────┘
               │ Unix Socket (/run/tinydb/tinydb.sock)
               │ Text Protocol (tab-separated, newline-terminated)
┌──────────────▼──────────────────────────────────────────────────┐
│                       Server Daemon                              │
│  - main.c, server.c, protocol.c                                 │
└──────────────┬──────────────────────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────────────────────┐
│                      SQL Engine                                  │
│  - parser.c, analyzer.c, planner.c, executor.c                  │
│  - expression.c, table.c                                         │
└──────────────┬──────────────────────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────────────────────┐
│                     Storage Engine                                │
│  - page.c, btree.c, btree_insert.c, btree_delete.c              │
│  - wal.c, checkpoint.c, arena.c                                 │
└──────────────┬──────────────────────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────────────────────┐
│                    File Layer                                     │
│  - file.c, mmap.c                                                │
└─────────────────────────────────────────────────────────────────┘
```

## Public API by Module

### SQL Parser (`src/parser.h`)

```c
typedef struct {
    Token* tokens;
    int count;
    int current;
    ASTNode* ast;
    char* error;
} Parser;

Parser* parser_create(const char* sql);
void parser_destroy(Parser* p);
ASTNode* parser_parse(Parser* p);
void parser_free_ast(ASTNode* node);
```

### SQL Analyzer (`src/analyzer.h`)

```c
typedef struct {
    Schema* schemas;
    int schema_count;
    char* error;
} Analyzer;

Analyzer* analyzer_create(Schema* schemas, int count);
void analyzer_destroy(Analyzer* a);
ASTNode* analyzer_analyze(Analyzer* a, ASTNode* node);
const char* analyzer_error(Analyzer* a);
```

### Query Executor (`src/executor.h`)

```c
typedef struct {
    Table* result;
    int rows_affected;
    char* error;
} ExecResult;

ExecResult executor_exec(Executor* e, ASTNode* stmt);
void executor_free_result(ExecResult* r);
```

### Storage Engine (`src/storage.h`)

```c
// Database lifecycle
int storage_open(Storage* s, const char* path);
void storage_close(Storage* s);

// Transaction control
int storage_begin(Storage* s);
int storage_commit(Storage* s);
int storage_rollback(Storage* s);

// DML operations
int storage_insert(Storage* s, const char* table, Row* row);
int storage_update(Storage* s, const char* table, Row* row, Expr* where);
int storage_delete(Storage* s, const char* table, Expr* where);

// Query operations
Table* storage_select(Storage* s, const char* table, Expr* where, 
                      int* order_by, int order_count, int limit, int distinct);

// Schema operations
int storage_create_table(Storage* s, Schema* schema);
int storage_drop_table(Storage* s, const char* table);
int storage_create_index(Storage* s, Index* index);
int storage_drop_index(Storage* s, const char* table, const char* index_name);

// Concurrency control
typedef enum {
    ISOLATION_READ_COMMITTED,  // Default: readers see committed data only
    ISOLATION_SERIALIZABLE     // Full isolation (not implemented in v1)
} IsolationLevel;

int storage_set_isolation(Storage* s, IsolationLevel level);
int storage_lock_shared(Storage* s);  // For read operations
int storage_lock_exclusive(Storage* s); // For write operations
int storage_unlock(Storage* s);

// Connection management (for multi-threaded server)
typedef struct {
    int id;
    Storage* storage;
    IsolationLevel isolation;
    int in_transaction;
    uint64_t txn_id;  // Transaction ID for this connection
} Connection;

Connection* storage_connect(Storage* s);
void storage_disconnect(Connection* conn);
```

### B+Tree (`src/btree.h`)

```c
#define B_TREE_MAX_DEPTH 16  // Maximum tree depth (fits in int8_t with margin)

// Cursor for iteration and point lookups
typedef struct {
    BTree* tree;
    uint32_t root_page;
    
    // Current position
    uint32_t page;     // Current page number
    int cell;          // Cell index within page (-1 for end)
    
    // Stack for traversal from root to leaf
    int depth;         // Current depth (0 = at root page)
    struct {
        uint32_t page;
        int cell;
    } stack[B_TREE_MAX_DEPTH];
    
    // Iteration state
    int is_forward;     // 1 for forward, 0 for backward
} BTreeCursor;

BTree* btree_open(const char* path, uint32_t magic);
int btree_close(BTree* bt);

// Transaction-aware operations
int btree_insert(BTree* bt, uint64_t key, const void* value, uint32_t len);
int btree_delete(BTree* bt, uint64_t key);
int btree_update(BTree* bt, uint64_t key, const void* value, uint32_t len); // In-place update

// Lookup and iteration
BTreeCursor* btree_find(BTree* bt, uint64_t key); // Find exact key
BTreeCursor* btree_first(BTree* bt);               // First key in order
BTreeCursor* btree_last(BTree* bt);                // Last key in order
void btree_cursor_next(BTreeCursor* c);
void btree_cursor_prev(BTreeCursor* c);  // Added for bidirectional iteration
int btree_cursor_valid(BTreeCursor* c);
void btree_cursor_free(BTreeCursor* c);

// Access current position
int btree_get(BTreeCursor* c, uint64_t* key, void* buf, uint32_t* len);
int btree_cursor_peek(BTreeCursor* c, uint64_t* key, void* buf, uint32_t* len);

// Range scan
typedef struct {
    BTreeCursor* start;
    BTreeCursor* end;
    // Internal: stores range bounds
} BTreeRange;

BTreeRange* btree_range_new(BTree* bt, uint64_t start_key, uint64_t end_key);
void btree_range_free(BTreeRange* r);
```

### Write-Ahead Log (`src/wal.h`)

```c
// WAL entry types
typedef enum {
    WAL_ENTRY_BEGIN,      // Transaction start
    WAL_ENTRY_COMMIT,    // Transaction commit
    WAL_ENTRY_ROLLBACK,  // Transaction rollback
    WAL_ENTRY_PAGE_MODIFY, // Page modification
    WAL_ENTRY_CHECKPOINT // Checkpoint marker
} WALEntryType;

// WAL entry header (8 bytes)
typedef struct {
    uint8_t type;        // WALEntryType
    uint8_t flags;       // Reserved flags
    uint16_t size;      // Payload size (excluding header)
    uint32_t checksum;   // CRC32 of entry
} WALEntryHeader;

// Variable-size WAL entry
typedef struct {
    WALEntryHeader header;
    void* payload;       // Type-specific data
} WALEntry;

// WAL handle
typedef struct {
    int fd;              // WAL file descriptor
    int shm_fd;          // Shared memory file descriptor
    uint64_t log_offset; // Current write position
    uint64_t last_checkpoint; // Offset of last checkpoint
    uint32_t frame_count; // Number of frames in WAL
    // Internal sync state
    pthread_mutex_t mutex; // Serialize WAL writes
} WAL;

WAL* wal_open(const char* db_path);
void wal_close(WAL* w);

// Write operations
int wal_append(WAL* w, WALEntry* entry);
int wal_flush(WAL* w);

// Checkpoint operations
int wal_checkpoint(WAL* w, uint64_t threshold_bytes);
int wal_checkpoint_full(WAL* w); // Force checkpoint regardless of threshold

// Recovery operations
typedef struct {
    uint64_t committed_offset; // Offset after last committed txn
    uint64_t wal_end_offset;   // End of WAL
    int needs_recovery;
    int has_uncommitted;
} WALRecoveryInfo;

WALRecoveryInfo wal_recovery_info(WAL* w);
int wal_recover(WAL* w, WALRecoveryInfo* info);
int wal_replay(WAL* w, void (*apply_page)(uint32_t page_num, void* data));
```

**WAL Recovery Protocol:**

On database open:
1. Check if `-wal` file exists and has content
2. If WAL is empty or only has CHECKPOINT marker: no recovery needed
3. If WAL has content:
   a. Scan WAL sequentially building transaction boundary list
   b. Identify committed transactions: entries between BEGIN and COMMIT
   c. Identify uncommitted transactions: BEGIN with no following COMMIT before EOF
   d. For committed transactions: mark pages as needing apply
   e. For uncommitted transactions: discard (no changes applied)
4. Apply page modifications in offset order (replay in order)
5. Truncate WAL to CHECKPOINT marker after successful recovery

**WAL File Format:**
```
[8-byte header]
  magic: 4 bytes (0x544C4442 "TLDW")
  version: 2 bytes (1)
  frame_count: 2 bytes

[Zero or more frames]
  frame_header:
    page_num: 4 bytes
    commit_offset: 4 bytes (offset of COMMIT for this txn, 0 if uncommitted)
  page_data: 4096 bytes

[4-byte trailer]
  checksum of entire file
```

**Checkpoint Process:**
1. Write CHECKPOINT marker to WAL with current offset
2. Call `wal_flush()` to sync WAL to disk
3. For each modified page in frame:
   a. Write page to main database file at page_num * PAGE_SIZE
   b. Mark page clean in page cache
4. Truncate WAL file (remove applied frames)
5. Update header with new frame_count=0
6. Flush main database file

**Checkpoint Trigger:**
- Automatic when WAL exceeds threshold (default 64MB from configuration.md)
- Manual via `wal_checkpoint_full()` for admin operations
- Before database close (optional, if dirty pages exist)

### Page Cache (`src/page.h`)

```c
typedef struct {
    void* data;           // Page data (PAGE_SIZE bytes)
    uint32_t id;          // Page number
    int refcount;         // Reference count for pin/unpin
    int is_dirty;         // 1 if modified since last flush
    int is_wal;           // 1 if page is from WAL (not yet applied to main db)
} Page;

// Page reference management
Page* page_pin(PageCache* cache, uint32_t page_id);  // Get and pin
void page_unpin(Page* p);  // Release pin
void page_mark_dirty(Page* p);

// Page cache with LRU eviction and thread-safe access
typedef struct {
    Page** pages;         // Hash table: page_id -> Page*
    int count;            // Current page count
    int max_cache_size;   // Max pages (e.g., 128 from config)
    int hit_count;        // For statistics
    int miss_count;
    // LRU list for eviction
    struct Page* lru_head;
    struct Page* lru_tail;
    // Lock for thread safety
    pthread_mutex_t mutex;
    pthread_rwlock_t rwlock;  // Read-write lock for page access
} PageCache;

PageCache* page_cache_create(int max_pages);
void page_cache_destroy(PageCache* cache);

// Cache operations (thread-safe)
Page* page_cache_get(PageCache* cache, uint32_t id);  // Shared read
Page* page_cache_get_exclusive(PageCache* cache, uint32_t id);  // Exclusive for writes
int page_cache_put(PageCache* cache, Page* p);  // Release after pin
int page_cache_evict(PageCache* cache, uint32_t page_id);  // Force evict
void page_cache_flush(PageCache* cache);  // Flush all dirty pages
void page_cache_flush_page(PageCache* cache, uint32_t page_id);

// Statistics
int page_cache_stats(PageCache* cache, int* hits, int* misses, int* size);
```

## Internal Module APIs

### Expression Evaluator (`src/expression.h`)

```c
// SQL Value types (same as ColumnType in schema)
typedef enum {
    VALUE_NULL = 0,
    VALUE_INTEGER = 1,
    VALUE_FLOAT = 2,
    VALUE_TEXT = 3,
    VALUE_BLOB = 4
} ValueType;

// Generic SQL value container
typedef struct {
    ValueType type;
    union {
        int64_t as_int;
        double as_float;
        struct { char* str; size_t len; } as_text;
        struct { void* blob; size_t len; } as_blob;
    };
} Value;

// Expression types
typedef enum {
    EXPR_LITERAL_INT,
    EXPR_LITERAL_FLOAT,
    EXPR_LITERAL_STRING,
    EXPR_LITERAL_NULL,
    EXPR_COLUMN,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_FUNC,
    EXPR_CASE,
    EXPR_SUBQUERY,
    EXPR_IN,
    EXPR_BETWEEN,
    EXPR_LIKE
} ExprType;

// Expression node with reference counting
typedef struct Expr {
    ExprType type;
    int refcount;
    union {
        int64_t as_int;                                       // EXPR_LITERAL_INT
        double as_float;                                      // EXPR_LITERAL_FLOAT
        struct { char* str; size_t len; } as_string;         // EXPR_LITERAL_STRING
        struct { int col_index; char* col_name; } as_column; // EXPR_COLUMN
        struct { Expr* left; TokenType op; Expr* right; } as_binary; // EXPR_BINARY
        struct { TokenType op; Expr* operand; } as_unary;     // EXPR_UNARY
        struct { char* name; Expr** args; int arg_count; } as_func;  // EXPR_FUNC
        struct { Expr* cond; Expr* then; Expr* else_; } as_case;     // EXPR_CASE
        struct { ASTNode* query; } as_subquery;              // EXPR_SUBQUERY
        struct { Expr* value; Expr** list; int count; int not; } as_in;     // EXPR_IN
        struct { Expr* value; Expr* low; Expr* high; int not; } as_between; // EXPR_BETWEEN
        struct { Expr* str; Expr* pattern; int escape; int not; } as_like; // EXPR_LIKE
    };
} Expr;

Expr* expr_create(ExprType type, ...);
void expr_ref(Expr* e);    // Increment refcount
void expr_unref(Expr* e);  // Decrement refcount, free if zero
Expr* expr_copy(Expr* e);

// Evaluate expression, returning Value (caller must free text/blob memory)
Value expr_eval(Expr* e, Row* row, EvalCtx* ctx);

// Helper functions for Value
Value value_from_int(int64_t v);
Value value_from_float(double v);
Value value_from_text(const char* str, size_t len);
Value value_from_null(void);
void value_free(Value* v);  // Free text/blob if allocated
int value_compare(Value* a, Value* b);  // For ORDER BY, WHERE
```

### Schema Definition (`src/schema.h`)

```c
typedef enum {
    COL_TYPE_INTEGER,
    COL_TYPE_FLOAT,
    COL_TYPE_TEXT,
    COL_TYPE_BLOB
} ColumnType;

typedef struct {
    char* name;
    ColumnType type;
    int not_null;
    int primary_key;
    int auto_increment;
    char* default_value;
} Column;

typedef struct {
    char* table_name;
    Column* columns;
    int col_count;
    uint32_t root_page;
    int auto_inc;
} Schema;

Schema* schema_create(const char* name, Column* cols, int count);
void schema_destroy(Schema* s);
int schema_serialize(Schema* s, void* buf, uint32_t buf_size);
Schema* schema_deserialize(const void* buf);
int schema_column_index(Schema* s, const char* name);
```

## Server Protocol API (`src/protocol.h`)

```c
typedef struct {
    int client_fd;
    char recv_buf[8192];
    int recv_len;
    // response state
} Protocol;

Protocol* protocol_accept(int server_fd);
int protocol_read(Protocol* p, char** out_sql);
int protocol_write_result(Protocol* p, Result* r);
int protocol_write_error(Protocol* p, const char* msg);
void protocol_close(Protocol* p);
```

## Key Design Decisions

1. **Reference counting on AST nodes**: Parser and analyzer use refcounts to manage memory. Callers must call `expr_destroy()` or `parser_free_ast()` when done.

2. **Opaque storage handle**: `Storage*` is the main handle passed to the executor. All allocation happens within the storage engine module.

3. **No internal globals**: Every module accepts explicit context. WAL is attached to `Storage`, not a global variable.

4. **Error handling via output parameters**: Functions return 0 on success, negative on error. Extended error context is returned via `char** error` out-parameters where callers need it.

5. **BTreeCursor is forward-only**: No backward iteration to keep cursor state minimal. "Rewind" requires a new cursor.

6. **Page refs are borrowed**: `page_cache_get()` returns a borrowed reference. Callers must `page_cache_put()` to release or keep.

7. **WAL entries are variadic**: `WALEntry` contains type-tagged unions to handle INSERT/UPDATE/DELETE/CREATE/DROP with per-type payload layout.
