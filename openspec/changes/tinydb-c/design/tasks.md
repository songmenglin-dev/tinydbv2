# TinyDB v2 Implementation Tasks

This file contains detailed implementation tasks derived from the design documents in this directory.

## Phase 1: Project Setup

### 1.1 Directory Structure and Build System

- [x] 1.1.1 Create top-level directory structure following `architecture.md` section 2.1
- [x] 1.1.2 Create `include/` directory with: `tinydb.h`, `config.h`, `types.h`
- [x] 1.1.3 Create `src/` subdirectories: `cli/`, `server/`, `sql/`, `storage/`, `util/`
- [x] 1.1.4 Create `tests/unit/`, `tests/integration/`, `tests/harness/` directories
- [x] 1.1.5 Create `Makefile` following `build-system.md` top-level Makefile design
- [x] 1.1.6 Implement `mini_test.h` test harness in `tests/unit/` following build-system.md
- [x] 1.1.7 Create `tests/unit/test-suite.c` test runner
- [x] 1.1.8 Create `.gitignore` for build artifacts, .db files, core dumps

### 1.2 Core Type Definitions

- [x] 1.2.1 Define PAGE_SIZE=4096, PAGE_SIZE_MASK, PAGE_SIZE_SHIFT in `include/types.h`
- [x] 1.2.2 Define MAX_PAGE_NUMBER, MAX_PAGES in `include/types.h`
- [x] 1.2.3 Define page types: PAGE_TYPE_HEADER, PAGE_TYPE_TABLE_LEAF, PAGE_TYPE_TABLE_INTERNAL, PAGE_TYPE_INDEX_LEAF, PAGE_TYPE_INDEX_INTERNAL, PAGE_TYPE_FREELIST, PAGE_TYPE_OVERFLOW
- [x] 1.2.4 Define error codes: SUCCESS=0, ERR_*, in `include/tinydb.h`
- [x] 1.2.5 Define ValueType enum (VALUE_NULL, VALUE_INTEGER, VALUE_FLOAT, VALUE_TEXT, VALUE_BLOB)
- [x] 1.2.6 Define ColumnType enum (COL_TYPE_INTEGER, COL_TYPE_FLOAT, COL_TYPE_TEXT, COL_TYPE_BLOB)
- [x] 1.2.7 Define IsolationLevel enum (ISOLATION_READ_COMMITTED, ISOLATION_SERIALIZABLE)

### 1.3 Utility Modules

- [x] 1.3.1 Implement `src/util/error.h` and `src/util/error.c` with error code definitions
- [x] 1.3.2 Implement `src/util/string.h` and `src/util/string.c` with string utilities
- [x] 1.3.3 Implement memory allocation wrappers with overflow checking
- [x] 1.3.4 Implement list/array data structures for internal use

## Phase 2: Storage Engine Core

### 2.1 Page and File Layer

- [x] 2.1.1 Implement header page structure (offset 0-4095) following architecture.md section 3.2
- [x] 2.1.2 Implement `pager_open()` - opens existing database file, reads header, validates magic
- [x] 2.1.3 Implement `pager_create()` - creates new file, writes header with magic `TNYD`
- [x] 2.1.4 Implement `pager_get_page()` - reads page into cache, handles 4096-byte alignment
- [x] 2.1.5 Implement `pager_flush_page()` - writes page to disk with fsync
- [x] 2.1.6 Implement `pager_close()` - closes file, releases resources
- [x] 2.1.7 Implement `pager_allocate_page()` - allocates new page (freelist first, then append)
- [x] 2.1.8 Implement `pager_free_page()` - adds page to freelist chain
- [x] 2.1.9 Implement header checksum calculation and validation (CRC32)
- [x] 2.1.10 Write unit tests for pager operations

### 2.2 Page Cache

- [x] 2.2.1 Implement Page struct with: data (PAGE_SIZE), id, refcount, is_dirty, is_wal fields
- [x] 2.2.2 Implement PageCache struct with hash table, LRU list, mutex, rwlock
- [x] 2.2.3 Implement `page_cache_create()` with configurable max_pages from config
- [x] 2.2.4 Implement `page_pin()` - get page and increment refcount
- [x] 2.2.5 Implement `page_unpin()` - decrement refcount
- [x] 2.2.6 Implement `page_mark_dirty()` - mark page as modified
- [x] 2.2.7 Implement LRU eviction algorithm in page cache
- [x] 2.2.8 Implement `page_cache_get()` with shared read lock
- [x] 2.2.9 Implement `page_cache_get_exclusive()` with exclusive write lock
- [x] 2.2.10 Implement `page_cache_flush()` - flush all dirty pages
- [x] 2.2.11 Implement `page_cache_stats()` for hit/miss tracking
- [x] 2.2.12 Write unit tests for page cache

### 2.3 B+Tree Implementation

- [x] 2.3.1 Define B_TREE_MAX_DEPTH=16 constant
- [x] 2.3.2 Define B_TREE_ORDER (keys per internal node) based on page size
- [x] 2.3.3 Define B_TREE_MIN (minimum keys for non-root)
- [x] 2.3.4 Implement BTreeNode struct (is_leaf, num_keys, keys[], children[], page_num)
- [x] 2.3.5 Implement `btree_open()` - opens or creates B+tree file
- [x] 2.3.6 Implement `btree_close()` - closes B+tree, flushes dirty pages
- [x] 2.3.7 Implement `btree_insert()` - insert key-value pair, split nodes if needed
- [x] 2.3.8 Implement `btree_delete()` - delete key, merge nodes if needed
- [x] 2.3.9 Implement `btree_update()` - in-place update of value
- [x] 2.3.10 Implement BTreeCursor with stack-based traversal and fixed-size stack[B_TREE_MAX_DEPTH]
- [x] 2.3.11 Implement `btree_find()` - find exact key, return cursor
- [x] 2.3.12 Implement `btree_first()` - return cursor to first key
- [x] 2.3.13 Implement `btree_last()` - return cursor to last key
- [x] 2.3.14 Implement `btree_cursor_next()` - advance cursor forward
- [x] 2.3.15 Implement `btree_cursor_prev()` - move cursor backward
- [x] 2.3.16 Implement `btree_cursor_valid()` - check if cursor is at valid position
- [x] 2.3.17 Implement `btree_cursor_free()` - free cursor resources
- [x] 2.3.18 Implement `btree_get()` - read current position data
- [x] 2.3.19 Implement `btree_cursor_peek()` - read without advancing
- [x] 2.3.20 Implement BTreeRange for range scans
- [x] 2.3.21 Implement `btree_range_new()` and `btree_range_free()`
- [x] 2.3.22 Write unit tests for B+tree operations

## Phase 3: SQL Parser

### 3.1 Lexer (Tokenizer)

- [x] 3.1.1 Define TokenType enum: TOKEN_EOF, TOKEN_ERROR, TOKEN_IDENTIFIER, TOKEN_KEYWORD, TOKEN_INTEGER, TOKEN_REAL, TOKEN_STRING, TOKEN_OPERATOR, TOKEN_PUNCTUATION
- [x] 3.1.2 Implement Token struct with type, lexeme, line, column
- [x] 3.1.3 Implement `lexer_create()` - initialize lexer with SQL string
- [x] 3.1.4 Implement `lexer_destroy()` - free lexer resources
- [x] 3.1.5 Implement `lexer_next_token()` - return next token
- [x] 3.1.6 Implement keyword recognition (CREATE, TABLE, INDEX, SELECT, INSERT, UPDATE, DELETE, BEGIN, COMMIT, ROLLBACK, DROP, FROM, WHERE, AND, OR, NOT, LIKE, IN, BETWEEN, IS, NULL, ORDER, BY, ASC, DESC, LIMIT, DISTINCT, INTO, VALUES, SET)
- [x] 3.1.7 Implement string escape handling: `''` → `'` (SQL standard)
- [x] 3.1.8 Implement numeric parsing (integers and floats)
- [x] 3.1.9 Write unit tests for lexer

### 3.2 AST Nodes

- [x] 3.2.1 Define ASTNodeType enum for all statement types
- [x] 3.2.2 Implement AST_CREATE_TABLE struct: table_name, columns (vector), if_not_exists
- [x] 3.2.3 Implement AST_DROP_TABLE struct: table_name, if_exists
- [x] 3.2.4 Implement AST_CREATE_INDEX struct: index_name, table_name, column_name
- [x] 3.2.5 Implement AST_DROP_INDEX struct: index_name
- [x] 3.2.6 Implement AST_INSERT struct: table_name, columns, values (vector)
- [x] 3.2.7 Implement AST_UPDATE struct: table_name, set_pairs (vector), where_expr
- [x] 3.2.8 Implement AST_DELETE struct: table_name, where_expr
- [x] 3.2.9 Implement AST_SELECT struct: columns (vector), table_name, where_expr, order_by (vector), limit, distinct
- [x] 3.2.10 Implement AST_TRANSACTION struct: BEGIN, COMMIT, ROLLBACK types
- [x] 3.2.11 Implement Expr node with reference counting per api-design.md
- [x] 3.2.12 Implement `ast_free()` - free all AST nodes recursively

### 3.3 Parser Implementation

- [x] 3.3.1 Implement `parser_create()` - initialize parser with SQL
- [x] 3.3.2 Implement `parser_destroy()` - free parser resources
- [x] 3.3.3 Implement `parser_parse()` - returns AST or error message
- [x] 3.3.4 Implement `parse_create_table()` - handle CREATE TABLE [IF NOT EXISTS]
- [x] 3.3.5 Implement `parse_drop_table()` - handle DROP TABLE [IF EXISTS]
- [x] 3.3.6 Implement `parse_create_index()` - handle CREATE INDEX
- [x] 3.3.7 Implement `parse_drop_index()` - handle DROP INDEX
- [x] 3.3.8 Implement `parse_insert()` - handle INSERT INTO ... VALUES
- [x] 3.3.9 Implement `parse_update()` - handle UPDATE ... SET ... WHERE (WHERE required)
- [x] 3.3.10 Implement `parse_delete()` - handle DELETE FROM ... WHERE (WHERE required)
- [x] 3.3.11 Implement `parse_select()` - handle SELECT with all clauses
- [x] 3.3.12 Implement `parse_transaction()` - handle BEGIN/COMMIT/ROLLBACK
- [x] 3.3.13 Write tests for parser

### 3.4 Expression Parser

- [x] 3.4.1 Implement `parse_expression()` with proper precedence (NOT > AND/OR > comparisons)
- [x] 3.4.2 Implement comparison operators: =, >, <, >=, <=, !=, <>
- [x] 3.4.3 Implement logical operators: AND, OR, NOT
- [x] 3.4.4 Implement LIKE operator with % and _ wildcards
- [x] 3.4.5 Implement IN operator: IN (val1, val2, ...)
- [x] 3.4.6 Implement BETWEEN operator: BETWEEN val1 AND val2
- [x] 3.4.7 Implement NULL check: IS [NOT] NULL
- [x] 3.4.8 Implement parentheses handling
- [x] 3.4.9 Write tests for expression parsing

## Phase 4: Query Execution Engine

### 4.1 Executor Core

- [x] 4.1.1 Implement Executor struct with storage, statistics
- [x] 4.1.2 Implement ResultSet struct with rows, columns, row_count
- [x] 4.1.3 Implement `executor_create()` - create executor context
- [x] 4.1.4 Implement `executor_destroy()` - free executor resources
- [x] 4.1.5 Implement `executor_exec()` - dispatch to appropriate handler based on AST type
- [x] 4.1.6 Implement `executor_exec_create_table()`
- [x] 4.1.7 Implement `executor_exec_drop_table()`
- [x] 4.1.8 Implement `executor_exec_create_index()`
- [x] 4.1.9 Implement `executor_exec_drop_index()`
- [x] 4.1.10 Implement `executor_exec_insert()`
- [x] 4.1.11 Implement `executor_exec_update()` - error if WHERE missing
- [x] 4.1.12 Implement `executor_exec_delete()` - error if WHERE missing
- [x] 4.1.13 Implement `executor_exec_select()`
- [x] 4.1.14 Implement `executor_exec_begin()`
- [x] 4.1.15 Implement `executor_exec_commit()`
- [x] 4.1.16 Implement `executor_exec_rollback()`
- [x] 4.1.17 Write unit tests for executor

### 4.2 Expression Evaluation

- [x] 4.2.1 Implement Value struct with type and union (int, float, text, blob)
- [x] 4.2.2 Implement `value_from_int()`, `value_from_float()`, `value_from_text()`, `value_from_null()`
- [x] 4.2.3 Implement `value_free()` - free text/blob memory
- [x] 4.2.4 Implement `value_compare()` - compare two values for ORDER BY, WHERE
- [x] 4.2.5 Implement `expr_create()` with variable arguments
- [x] 4.2.6 Implement `expr_ref()` - increment reference count
- [x] 4.2.7 Implement `expr_unref()` - decrement refcount, free if zero
- [x] 4.2.8 Implement `expr_copy()` - deep copy expression
- [x] 4.2.9 Implement `expr_eval()` - evaluate expression for a row, return Value
- [x] 4.2.10 Implement `eval_literal()` - evaluate literal values
- [x] 4.2.11 Implement `eval_column()` - resolve column references
- [x] 4.2.12 Implement `eval_binary()` - evaluate binary operations
- [x] 4.2.13 Implement `eval_unary()` - evaluate unary operations
- [x] 4.2.14 Implement `eval_like()` - pattern matching with % and _
- [x] 4.2.15 Implement `eval_in()` - IN list membership check
- [x] 4.2.16 Implement `eval_between()` - BETWEEN range check
- [x] 4.2.17 Write unit tests for expression evaluation

### 4.3 SELECT Processing

- [x] 4.3.1 Implement `select_init_query()` - setup for SELECT execution
- [x] 4.3.2 Implement `select_apply_where()` - filter rows using WHERE expression
- [x] 4.3.3 Implement `select_apply_distinct()` - remove duplicate rows
- [x] 4.3.4 Implement `select_apply_order_by()` - sort by columns, ASC/DESC
- [x] 4.3.5 Implement `select_apply_limit()` - limit result row count
- [x] 4.3.6 Implement `select_project_columns()` - select specific columns or *
- [x] 4.3.7 Implement aggregate handling (COUNT, MIN, MAX, SUM, AVG) if time permits
- [x] 4.3.8 Write tests for SELECT with all clause combinations

### 4.4 Transaction Execution

- [x] 4.4.1 Implement Transaction struct with state (INIT, ACTIVE, COMMITTED, ROLLED_BACK), changes list
- [x] 4.4.2 Implement `txn_begin()` - start transaction, set state to ACTIVE
- [x] 4.4.3 Implement `txn_commit()` - commit transaction, clear changes list
- [x] 4.4.4 Implement `txn_rollback()` - rollback transaction, revert changes
- [x] 4.4.5 Track page modifications during transaction for rollback
- [x] 4.4.6 Implement `txn_savepoint()` and `txn_rollback_to_savepoint()` if needed
- [x] 4.4.7 Write unit tests for transactions

## Phase 5: WAL (Write-Ahead Logging)

### 5.1 WAL Structure

- [x] 5.1.1 Define WAL magic: 0x544C4442 ("TLDW"), WAL version: 1
- [x] 5.1.2 Implement WAL entry types enum: WAL_ENTRY_BEGIN, WAL_ENTRY_COMMIT, WAL_ENTRY_ROLLBACK, WAL_ENTRY_PAGE_MODIFY, WAL_ENTRY_CHECKPOINT
- [x] 5.1.3 Implement WALEntryHeader struct (8 bytes): type, flags, size, checksum
- [x] 5.1.4 Implement WALEntry struct with header and variable-size payload
- [x] 5.1.5 Implement WAL struct with fd, shm_fd, log_offset, last_checkpoint, frame_count, mutex
- [x] 5.1.6 Implement `wal_open()` - opens or creates WAL file with header
- [x] 5.1.7 Implement `wal_close()` - closes WAL file, releases resources
- [x] 5.1.8 Implement `wal_append()` - append entry to WAL with mutex lock
- [x] 5.1.9 Implement `wal_flush()` - fsync WAL to disk
- [x] 5.1.10 Write unit tests for WAL structure

### 5.2 WAL Recovery

- [x] 5.2.1 Implement `wal_recovery_info()` - scan WAL, return WALRecoveryInfo
- [x] 5.2.2 Identify committed transactions (BEGIN at offset, COMMIT at later offset)
- [x] 5.2.3 Identify uncommitted transactions (BEGIN but no COMMIT before EOF)
- [x] 5.2.4 Implement `wal_recover()` - replay committed transactions, discard uncommitted
- [x] 5.2.5 Implement `wal_replay()` - apply page modifications in offset order
- [x] 5.2.6 Implement `wal_truncate()` - remove applied frames after checkpoint
- [x] 5.2.7 Integrate WAL recovery into `storage_open()` - call on database open
- [x] 5.2.8 Write unit tests for WAL recovery

### 5.3 Checkpoint

- [x] 5.3.1 Implement `wal_checkpoint()` - periodic checkpoint when threshold reached
- [x] 5.3.2 Implement `wal_checkpoint_full()` - force checkpoint regardless of threshold
- [x] 5.3.3 Implement checkpoint process: write CHECKPOINT marker, flush WAL, copy pages to main db, truncate WAL
- [x] 5.3.4 Implement automatic checkpoint trigger (default 64MB threshold from configuration.md)
- [x] 5.3.5 Implement manual checkpoint via admin command
- [x] 5.3.6 Write unit tests for checkpoint

### 5.4 WAL Integration

- [x] 5.4.1 Modify `storage_insert()` to write WAL entry before applying
- [x] 5.4.2 Modify `storage_update()` to write WAL entry before applying
- [x] 5.4.3 Modify `storage_delete()` to write WAL entry before applying
- [x] 5.4.4 Flush WAL to disk on COMMIT before returning success
- [x] 5.4.5 Discard WAL entries on ROLLBACK (already applied changes are reverted)
- [x] 5.4.6 Write integration tests for WAL with transactions

## Phase 6: Catalog and Schema

### 6.1 tinydb_master System Table

- [x] 6.1.1 Define tinydb_master schema: type TEXT, name TEXT, tbl_name TEXT, sql TEXT
- [x] 6.1.2 Initialize tinydb_master on first database creation (in `pager_create()`)
- [x] 6.1.3 Implement `catalog_insert()` - add entry to tinydb_master
- [x] 6.1.4 Implement `catalog_delete()` - remove entry from tinydb_master
- [x] 6.1.5 Implement `catalog_lookup_type_name()` - get entry by type and name
- [x] 6.1.6 Implement `catalog_get_tables()` - list all tables
- [x] 6.1.7 Implement `catalog_get_indexes()` - list indexes for table
- [x] 6.1.8 Write unit tests for catalog operations

### 6.2 Schema Management

- [x] 6.2.1 Implement Schema struct with table_name, columns, col_count, root_page, auto_inc
- [x] 6.2.2 Implement Column struct with name, type, not_null, primary_key, auto_increment, default_value
- [x] 6.2.3 Implement `schema_create()` - create schema from column definitions
- [x] 6.2.4 Implement `schema_destroy()` - free schema resources
- [x] 6.2.5 Implement `schema_serialize()` - serialize schema to bytes for storage
- [x] 6.2.6 Implement `schema_deserialize()` - deserialize schema from bytes
- [x] 6.2.7 Implement `schema_column_index()` - find column index by name

### 6.3 Table and Index Storage

- [x] 6.3.1 Implement `table_create()` - creates B+tree for table, inserts into tinydb_master
- [x] 6.3.2 Implement `table_drop()` - removes table B+tree, removes from tinydb_master
- [x] 6.3.3 Implement `table_insert()` - allocates rowid, inserts into B+tree, updates indexes
- [x] 6.3.4 Implement `table_update()` - updates row (rowid stable), updates indexes
- [x] 6.3.5 Implement `table_delete()` - removes from B+tree, removes from indexes
- [x] 6.3.6 Implement `table_select()` - scans table, applies WHERE, returns matching rows
- [x] 6.3.7 Implement `table_get_by_rowid()` - direct row lookup
- [x] 6.3.8 Implement `index_create()` - creates B-tree index, inserts into tinydb_master
- [x] 6.3.9 Implement `index_drop()` - removes index B-tree, removes from tinydb_master
- [x] 6.3.10 Implement `index_insert()` - adds index entry
- [x] 6.3.11 Implement `index_delete()` - removes index entry
- [x] 6.3.12 Implement `index_lookup()` - returns rowids matching key
- [x] 6.3.13 Implement `index_range_lookup()` - returns rowids in range
- [x] 6.3.14 Auto-update indexes on INSERT/UPDATE/DELETE
- [x] 6.3.15 Write unit tests for table and index operations

## Phase 7: Server Daemon

### 7.1 Socket Server

- [x] 7.1.1 Create `src/server/server.h`
- [x] 7.1.2 Implement `server_init()` - initialize server context
- [x] 7.1.3 Implement `server_bind_and_listen()` - create Unix socket at path, listen
- [x] 7.1.4 Implement `server_accept_client()` - accept connection, set non-blocking
- [x] 7.1.5 Implement `server_handle_client()` - read query, execute, respond, close
- [x] 7.1.6 Implement `server_run_loop()` - main accept loop
- [x] 7.1.7 Create runtime directory `/run/tinydb/` with proper permissions
- [x] 7.1.8 Write unit tests for server socket

### 7.2 Protocol Handler

- [x] 7.2.1 Implement Protocol struct with client_fd, recv_buf, recv_len
- [x] 7.2.2 Implement `protocol_accept()` - accept on server socket, return Protocol
- [x] 7.2.3 Implement `protocol_read()` - read SQL line until newline
- [x] 7.2.4 Implement `protocol_write_result()` - write tab-separated results
- [x] 7.2.5 Implement `protocol_write_error()` - write "ERROR: message"
- [x] 7.2.6 Implement `protocol_write_ok()` - write "OK n" for rows affected
- [x] 7.2.7 Implement `protocol_close()` - close client connection
- [x] 7.2.8 Write tests for protocol

### 7.3 Lifecycle Management

- [x] 7.3.1 Write PID to `/run/tinydb/tinydb.pid`
- [x] 7.3.2 Acquire lock file `/run/tinydb/tinydb.lock` with exclusive lock
- [x] 7.3.3 Handle SIGTERM gracefully - complete query, close database, remove PID
- [x] 7.3.4 Handle SIGINT (Ctrl+C) - same cleanup as SIGTERM
- [x] 7.3.5 Check for existing lock on startup - fail if already running
- [x] 7.3.6 Implement `server_cleanup()` - remove PID file, release lock, close database
- [x] 7.3.7 Write tests for lifecycle

### 7.4 Main Entry Point

- [x] 7.4.1 Parse arguments: --help, --version, --db-path, --socket-path, --log-level
- [x] 7.4.2 Load configuration from environment and defaults
- [x] 7.4.3 Initialize database (create if not exists)
- [x] 7.4.4 Initialize server socket
- [x] 7.4.5 Run server loop
- [x] 7.4.6 Handle errors and cleanup properly
- [x] 7.4.7 Implement `--version` output: "TinyDB v2.0.0"

## Phase 8: CLI Client

### 8.1 CLI Core

- [x] 8.1.1 Create `src/cli/cli.h`
- [x] 8.1.2 Implement `cli_parse_args()` - parse -c, -f, -s, -n, -t, -V, -q, --help, --version
- [x] 8.1.3 Implement `cli_connect()` - connect to server socket
- [x] 8.1.4 Implement `cli_disconnect()` - close connection
- [x] 8.1.5 Implement `cli_interactive_mode()` - read-eval-print loop
- [x] 8.1.6 Implement `cli_single_query()` - execute single query, print result, exit
- [x] 8.1.7 Implement `cli_batch_mode()` - read SQL from file, execute, exit
- [x] 8.1.8 Multi-line input (continue on incomplete statement, detect ';')
- [x] 8.1.9 SQL history (up/down arrows, Ctrl+R search, ~/.tinydb_history)
- [x] 8.1.10 Write tests for CLI core

### 8.2 Meta Commands

- [x] 8.2.1 Implement `.help` - show available commands
- [x] 8.2.2 Implement `.quit` / `.exit` - exit CLI
- [x] 8.2.3 Implement `.tables` - query tinydb_master for type='table'
- [x] 8.2.4 Implement `.schema [table]` - query tinydb_master for CREATE SQL
- [x] 8.2.5 Implement `.indexes [table]` - list indexes for table
- [x] 8.2.6 Implement `.plan <sql>` - show EXPLAIN QUERY PLAN
- [x] 8.2.7 Implement `.timer on/off` - enable/disable query timing
- [x] 8.2.8 Implement `.mode <box|csv|line|list>` - output mode
- [x] 8.2.9 Implement `.headers on/off` - show/hide column headers
- [x] 8.2.10 Implement `.null <string>` - string for NULL display
- [x] 8.2.11 Implement `.pager <cmd>` - set pager command
- [x] 8.2.12 Implement `.shell <cmd>` - execute shell command
- [x] 8.2.13 Implement `.read <file>` - read and execute SQL from file
- [x] 8.2.14 Implement `.trace on/off` - enable server trace
- [x] 8.2.15 Implement `.stats` - show statistics
- [x] 8.2.16 Implement `cli_is_meta_command()` - detect dot commands
- [x] 8.2.17 Write tests for meta commands

### 8.3 Result Display

- [x] 8.3.1 Implement Box mode output with aligned columns and borders
- [x] 8.3.2 Implement CSV mode output
- [x] 8.3.3 Implement Line mode output (column = value)
- [x] 8.3.4 Implement List mode output (tab-separated)
- [x] 8.3.5 Implement `display_result_set()` - route to appropriate formatter
- [x] 8.3.6 Implement `display_column_headers()` - print column names
- [x] 8.3.7 Implement `display_row_count()` - print "n rows affected"
- [x] 8.3.8 Implement `display_error()` - print "ERROR: message" (red if terminal supports)
- [x] 8.3.9 ANSI color support for errors and syntax highlighting
- [x] 8.3.10 Write tests for result display

### 8.4 Auto-Complete and Editing

- [x] 8.4.1 SQL keyword auto-complete (SELECT, INSERT, etc.)
- [x] 8.4.2 Table name auto-complete (after FROM, INTO)
- [x] 8.4.3 Column name auto-complete (after WHERE, SET)
- [x] 8.4.4 Emacs-style editing (Ctrl+A, Ctrl+E, Ctrl+K, Ctrl+Y)
- [x] 8.4.5 Ctrl+C to cancel current input
- [x] 8.4.6 Syntax highlighting (keywords cyan, strings green, numbers magenta)

### 8.5 Interactive Features

- [x] 8.5.1 Startup banner: "TinyDB v2.0.0\nType \".help\" for help."
- [x] 8.5.2 Prompt formatting: "tinydb> ", continuation "   ...> ", transaction "tinydb tx> "
- [x] 8.5.3 Pager integration for large result sets
- [x] 8.5.4 Connection error handling: "ERROR: Cannot connect to server"
- [x] 8.5.5 Quiet mode for batch use (no banner, minimal output)

## Phase 9: Configuration System

### 9.1 Compile-Time Constants

- [x] 9.1.1 Define defaults in `include/config.h` following configuration.md
- [x] 9.1.2 Define TINYDB_DEFAULT_SOCKET_PATH, TINYDB_DEFAULT_DATA_DIR, etc.
- [x] 9.1.3 Define PAGE_SIZE, TINYDB_CACHE_PAGES, TINYDB_WAL_SYNC_MODE
- [x] 9.1.4 Define TINYDB_CHECKPOINT_BYTES threshold
- [x] 9.1.5 Define TINYDB_MAX_SQL_LENGTH (1MB)

### 9.2 Environment Variables

- [x] 9.2.1 Implement `config_load()` function to read environment variables
- [x] 9.2.2 Parse TINYDB_SOCKET, TINYDB_DATA_DIR, TINYDB_DB_NAME, etc.
- [x] 9.2.3 Parse TINYDB_CACHE_SIZE, TINYDB_WAL_SYNC, TINYDB_CHECKPOINT_THRESHOLD
- [x] 9.2.4 Parse TINYDB_LOG_LEVEL, TINYDB_READONLY, TINYDB_TRACE_SQL
- [x] 9.2.5 Implement TINYDB_HISTORY, TINYDB_PAGER for CLI
- [x] 9.2.6 Implement TINYDB_STAT_INTERVAL for periodic stats dump

### 9.3 Runtime Validation

- [x] 9.3.1 Implement `config_validate()` to check values are reasonable
- [x] 9.3.2 Validate cache_pages range (4-4096)
- [x] 9.3.3 Validate wal_sync_mode (0-2)
- [x] 9.3.4 Validate checkpoint_threshold minimum (1MB)
- [x] 9.3.5 Validate data directory is accessible

## Phase 10: Observability

### 10.1 Logging System

- [x] 10.1.1 Define LogLevel enum: LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG
- [x] 10.1.2 Implement log_init() with destination (stderr, syslog, file, journald)
- [x] 10.1.3 Implement log_write() with ISO8601timestamp LEVEL [file:line] format
- [x] 10.1.4 Implement log category filtering via LOG_CAT_* masks
- [x] 10.1.5 Implement log rotation for file destination
- [x] 10.1.6 Implement log_set_level() runtime level change
- [x] 10.1.7 Write tests for logging

### 10.2 Statistics System

- [x] 10.2.1 Implement Statistics struct with all counters per observability.md
- [x] 10.2.2 Implement stats_init(), stats_destroy()
- [x] 10.2.3 Implement stats_increment() for counter increments
- [x] 10.2.4 Implement stats_gauge_set() for gauge values
- [x] 10.2.5 Implement stats_dump() for human-readable output
- [x] 10.2.6 Implement stats_reset() for counter reset
- [x] 10.2.7 Implement SIGUSR1 handler to dump stats to log

### 10.3 tinydb_stats Virtual Table

- [x] 10.3.1 Implement special case handling in executor for "tinydb_stats" table
- [x] 10.3.2 Return computed values from Statistics struct
- [x] 10.3.3 Implement "tinydb_table_stats" with per-table statistics
- [x] 10.3.4 Write tests for statistics virtual tables

### 10.4 Health Checks

- [x] 10.4.1 Implement startup verification sequence
- [x] 10.4.2 Check socket directory writable
- [x] 10.4.3 Check data directory exists and writable
- [x] 10.4.4 Check lock file can be acquired
- [x] 10.4.5 Check database file can be opened
- [x] 10.4.6 Implement .ping meta-command for runtime health
- [x] 10.4.7 Implement .lock meta-command for lock status

### 10.5 Crash Handling

- [x] 10.5.1 Implement signal handlers for SIGSEGV, SIGABRT
- [x] 10.5.2 Catch and log current query context before crash
- [x] 10.5.3 Flush WAL before exit
- [x] 10.5.4 Print stack trace in debug builds

## Phase 11: Security

### 11.1 File Permissions

- [x] 11.1.1 Implement `security_set_permissions()` to set ownership and modes
- [x] 11.1.2 Set data directory /var/lib/tinydb to 0750
- [x] 11.1.3 Set run directory /run/tinydb to 0755
- [x] 11.1.4 Set socket to 0666 (world-writable for clients)
- [x] 11.1.5 Write tests for permission setting

### 11.2 Input Validation

- [x] 11.2.1 Reject NUL bytes in SQL input
- [x] 11.2.2 Reject control characters except newline/tab/cr
- [x] 11.2.3 Enforce SQL length limit (1MB)
- [x] 11.2.4 Implement `security_validate_path()` - absolute paths, no ..
- [x] 11.2.5 Implement `validate_integer()` bounds checking
- [x] 11.2.6 Implement `validate_string_length()` max length check
- [x] 11.2.7 Write tests for input validation

### 11.3 SQL Injection Prevention

- [x] 11.3.1 Implement `escape_string()` per SQL standard ('' not \')
- [x] 11.3.2 Implement parameterized query support if time permits
- [x] 11.3.3 Document injection prevention in code comments
- [x] 11.3.4 Write tests for string escaping

### 11.4 Connection Security

- [x] 11.4.1 Implement connection limits (TINYDB_MAX_CONNECTIONS)
- [x] 11.4.2 Implement concurrent query limits (TINYDB_MAX_CONCURRENT_QUERIES)
- [x] 11.4.3 Implement query timeout (TINYDB_QUERY_TIMEOUT_MS)
- [x] 11.4.4 Validate peer PID on Unix socket connections

## Phase 12: Systemd Integration

### 12.1 Service Unit

- [x] 12.1.1 Create `systemd/tinydb.service` with configuration.md directives
- [x] 12.1.2 Configure Type=notify, User=tinydb, Group=tinydb
- [x] 12.1.3 Configure RuntimeDirectory, StateDirectory, LogsDirectory
- [x] 12.1.4 Configure all security sandbox directives (NoNewPrivileges, ProtectSystem, etc.)
- [x] 12.1.5 Configure resource limits (LimitNOFILE, LimitNPROC)
- [x] 12.1.6 Configure restart policy (Restart=on-failure, RestartSec=5s)
- [x] 12.1.7 Configure environment variables (TINYDB_LOG_LEVEL, etc.)

### 12.2 Socket Unit (Optional)

- [x] 12.2.1 Create `systemd/tinydb.socket` for socket activation
- [x] 12.2.2 Configure ListenStream=/run/tinydb/tinydb.sock
- [x] 12.2.3 Document socket activation benefits

### 12.3 tmpfiles.d

- [x] 12.3.1 Create `systemd/tinydb-tmpfiles.conf` for /run/tinydb/
- [x] 12.3.2 Define D! /run/tinydb 0755 tinydb tinydb -
- [x] 12.3.3 Document installation steps

## Phase 13: Testing

### 13.1 Unit Tests

- [x] 13.1.1 Write unit tests for pager (see 2.1.10)
- [x] 13.1.2 Write unit tests for page cache (see 2.2.12)
- [x] 13.1.3 Write unit tests for btree (see 2.3.22)
- [x] 13.1.4 Write unit tests for lexer (see 3.1.9)
- [x] 13.1.5 Write unit tests for parser (see 3.3.13)
- [x] 13.1.6 Write unit tests for expressions (see 4.2.17)
- [x] 13.1.7 Write unit tests for executor (see 4.1.17)
- [x] 13.1.8 Write unit tests for SELECT (see 4.3.8)
- [x] 13.1.9 Write unit tests for transactions (see 4.4.7)
- [x] 13.1.10 Write unit tests for WAL (see 5.1.10)
- [x] 13.1.11 Write unit tests for WAL recovery (see 5.2.8)
- [x] 13.1.12 Write unit tests for catalog (see 6.1.8)
- [x] 13.1.13 Write unit tests for table/index operations (see 6.3.15)
- [x] 13.1.14 Write unit tests for server (see 7.1.7)
- [x] 13.1.15 Write unit tests for protocol (see 7.2.8)
- [x] 13.1.16 Write unit tests for CLI (see 8.1.9)
- [x] 13.1.17 Write unit tests for meta commands (see 8.2.17)
- [x] 13.1.18 Write unit tests for logging (see 10.1.7)
- [x] 13.1.19 Write unit tests for security (see 11.2.7, 11.3.4)

### 13.2 Integration Tests

- [x] 13.2.1 Write integration test: server + CLI basic operations
- [x] 13.2.2 Write integration test: CREATE TABLE, INSERT, SELECT, UPDATE, DELETE
- [x] 13.2.3 Write integration test: transaction BEGIN/COMMIT/ROLLBACK
- [x] 13.2.4 Write integration test: CREATE INDEX, index usage
- [x] 13.2.5 Write integration test: crash recovery (kill server during transaction)
- [x] 13.2.6 Write integration test: concurrent connections (if multi-threaded)
- [x] 13.2.7 Write integration test: disk full scenarios
- [x] 13.2.8 Write integration test: WAL checkpoint trigger

### 13.3 Test Infrastructure

- [x] 13.3.1 Implement test database creation/teardown helpers
- [x] 13.3.2 Implement random data generators for stress testing
- [x] 13.3.3 Implement test isolation (each test gets fresh database)
- [x] 13.3.4 Implement valgrind suppressions for known patterns
- [x] 13.3.5 Generate coverage reports with gcov

## Phase 14: Documentation

### 14.1 README

- [x] 14.1.1 Write README with build instructions (make, make test, make install)
- [x] 14.1.2 Document command line options for server and CLI
- [x] 14.1.3 Document SQL syntax supported
- [x] 14.1.4 Document configuration via environment variables
- [x] 14.1.5 Document systemd installation steps

### 14.2 Man Pages

- [x] 14.2.1 Write man page for tinydb-server(1)
- [x] 14.2.2 Write man page for tinydb-cli(1)
- [x] 14.2.3 Write man page for tinydb.conf(5) if config file used

### 14.3 SQL Reference

- [x] 14.3.1 Document CREATE TABLE syntax with column types and constraints
- [x] 14.3.2 Document CREATE INDEX syntax
- [x] 14.3.3 Document SELECT syntax with all clauses
- [x] 14.3.4 Document INSERT, UPDATE, DELETE syntax
- [x] 14.3.5 Document transaction statements (BEGIN, COMMIT, ROLLBACK)
- [x] 14.3.6 Document string escape conventions ('' not \')

### 14.4 Architecture Documentation

- [x] 14.4.1 Verify all design document requirements are implemented
- [x] 14.4.2 Update architecture.md with actual file structure if changed
- [x] 14.4.3 Document any deviations from design