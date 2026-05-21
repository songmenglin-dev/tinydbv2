## 1. Project Setup

- [ ] 1.1 Create directory structure: `src/` (storage/, parser/, server/, cli/, common/), `include/`, `tests/`, `Makefile`
- [ ] 1.2 Create `include/tinydb.h` with common types, constants, error codes
- [ ] 1.3 Create `Makefile` with targets: `all`, `clean`, `test`, `tinydb` (server), `tinydb-cli` (client)
- [ ] 1.4 Define error codes: SUCCESS=0, ERR_*, in `include/tinydb.h`
- [ ] 1.5 Create `src/common/result.h` for query result structure (rows, columns, error)
- [ ] 1.6 Create initial `src/server/main.c` with --help and --version
- [ ] 1.7 Create initial `src/cli/main.c` with --help and --version

## 2. Storage Engine - Core

### 2.1 Page and Header

- [ ] 2.1.1 Define PAGE_SIZE=4096, MAX_PAGES=4294967296 in `include/tinydb.h`
- [ ] 2.1.2 Create `src/storage/pager.h` with Pager struct (file handle, page cache)
- [ ] 2.1.3 Implement `pager_open(filename)` - opens file, reads header, validates magic `TNYD`
- [ ] 2.1.4 Implement `pager_create(filename)` - creates new file, writes header with magic
- [ ] 2.1.5 Implement `pager_get_page(pager, page_num)` - reads page into cache
- [ ] 2.1.6 Implement `pager_flush_page(pager, page_num)` - writes page to disk
- [ ] 2.1.7 Implement `pager_close(pager)` - closes file, frees resources
- [ ] 2.1.8 Implement `pager_allocate_page(pager)` - returns new page number (freelist or append)
- [ ] 2.1.9 Implement `pager_free_page(pager, page_num)` - adds page to freelist
- [ ] 2.1.10 Implement header checksum calculation and validation
- [ ] 2.1.11 Write tests for pager: open existing, create new, read/write pages

### 2.2 B+Tree Implementation

- [ ] 2.2.1 Create `src/storage/btree.h` with BTreeNode struct (is_leaf, num_keys, keys[], children[])
- [ ] 2.2.2 Define B_TREE_ORDER=64 (max keys per internal node), B_TREE_MIN=32 (min keys)
- [ ] 2.2.3 Implement `btree_search(root, key)` - returns page_num of leaf containing key
- [ ] 2.2.4 Implement `btree_insert(root, key, value)` - splits nodes if needed
- [ ] 2.2.5 Implement `btree_delete(root, key)` - merges nodes if needed
- [ ] 2.2.6 Implement `btree_range_scan(root, start_key, end_key)` - returns keys in range
- [ ] 2.2.7 Implement `btree_get_first(root)` and `btree_get_last(root)`
- [ ] 2.2.8 Write tests for btree: insert, search, delete, range scan

### 2.3 Table Storage

- [ ] 2.3.1 Create `src/storage/table.h` with Table struct (table_name, root_page, schema)
- [ ] 2.3.2 Create `src/storage/schema.h` with Column struct (name, type), Schema struct
- [ ] 2.3.3 Implement `table_create(table_name, schema)` - creates table, inserts into tinydb_master
- [ ] 2.3.4 Implement `table_drop(table_name)` - removes table B+tree, removes from tinydb_master
- [ ] 2.3.5 Implement `table_insert(row)` - allocates rowid, inserts into B+tree
- [ ] 2.3.6 Implement `table_update(rowid, values)` - updates row (rowid stable)
- [ ] 2.3.7 Implement `table_delete(rowid)` - removes from B+tree
- [ ] 2.3.8 Implement `table_select(conditions)` - applies conditions, returns matching rows
- [ ] 2.3.9 Implement `table_get_by_rowid(rowid)` - direct row lookup
- [ ] 2.3.10 Write tests for table operations

### 2.4 Index Storage

- [ ] 2.4.1 Create `src/storage/index.h` with Index struct (index_name, table_name, column_name, root_page)
- [ ] 2.4.2 Implement `index_create(index_name, table_name, column_name)` - creates B-tree index
- [ ] 2.4.3 Implement `index_drop(index_name)` - removes index B-tree, removes from tinydb_master
- [ ] 2.4.4 Implement `index_insert(index, key, rowid)` - adds index entry
- [ ] 2.4.5 Implement `index_delete(index, key, rowid)` - removes index entry
- [ ] 2.4.6 Implement `index_lookup(index, key)` - returns rowids matching key
- [ ] 2.4.7 Implement `index_range_lookup(index, start, end)` - returns rowids in range
- [ ] 2.4.8 Auto-update indexes on INSERT/UPDATE/DELETE
- [ ] 2.4.9 Write tests for index operations

### 2.5 tinydb_master System Table

- [ ] 2.5.1 Define tinydb_master schema: type TEXT, name TEXT, tbl_name TEXT, sql TEXT
- [ ] 2.5.2 Initialize tinydb_master on first database creation
- [ ] 2.5.3 Implement `catalog_insert(type, name, tbl_name, sql)` - add entry to tinydb_master
- [ ] 2.5.4 Implement `catalog_delete(type, name)` - remove entry from tinydb_master
- [ ] 2.5.5 Implement `catalog_lookup_type_name(type, name)` - get entry by type and name
- [ ] 2.5.6 Implement `catalog_get_tables()` - list all tables
- [ ] 2.5.7 Implement `catalog_get_indexes(table_name)` - list indexes for table
- [ ] 2.5.8 Write tests for catalog operations

## 3. SQL Parser

### 3.1 Lexer (Tokenizer)

- [ ] 3.1.1 Create `src/parser/lexer.h` with TokenType enum and Token struct
- [ ] 3.1.2 Define tokens: KEYWORD, IDENTIFIER, INTEGER, REAL, STRING, OPERATOR, PUNCTUATION
- [ ] 3.1.3 Implement `lexer_init(sql)` - initialize lexer with SQL string
- [ ] 3.1.4 Implement `lexer_next_token()` - return next token
- [ ] 3.1.5 Implement string escape handling: `''` → `'`
- [ ] 3.1.6 Implement keyword recognition: CREATE, TABLE, INDEX, SELECT, INSERT, UPDATE, DELETE, etc.
- [ ] 3.1.7 Write tests for lexer: keywords, identifiers, numbers, strings with escapes

### 3.2 AST Nodes

- [ ] 3.2.1 Create `src/parser/ast.h` with all AST node types
- [ ] 3.2.2 Define AST_CREATE_TABLE: table_name, columns (name+type), if_not_exists
- [ ] 3.2.3 Define AST_DROP_TABLE: table_name, if_exists
- [ ] 3.2.4 Define AST_CREATE_INDEX: index_name, table_name, column_name
- [ ] 3.2.5 Define AST_DROP_INDEX: index_name
- [ ] 3.2.6 Define AST_INSERT: table_name, values[]
- [ ] 3.2.7 Define AST_UPDATE: table_name, set_pairs[], where_expr
- [ ] 3.2.8 Define AST_DELETE: table_name, where_expr
- [ ] 3.2.9 Define AST_SELECT: columns[], table_name, where_expr, order_by, limit, distinct
- [ ] 3.2.10 Define AST_EXPR: type (literal, column, binary, unary, function), value/left/right
- [ ] 3.2.11 Define AST_TRANSACTION: BEGIN, COMMIT, ROLLBACK
- [ ] 3.2.12 Implement `ast_free(ast)` - free all AST nodes

### 3.3 Parser Implementation

- [ ] 3.3.1 Create `src/parser/parser.h` with Parser struct
- [ ] 3.3.2 Implement `parser_init(sql)` - initialize parser with SQL
- [ ] 3.3.3 Implement `parser_parse()` - returns AST or error
- [ ] 3.3.4 Implement `parse_create_table()` - handle CREATE TABLE [IF NOT EXISTS]
- [ ] 3.3.5 Implement `parse_drop_table()` - handle DROP TABLE [IF EXISTS]
- [ ] 3.3.6 Implement `parse_create_index()` - handle CREATE INDEX
- [ ] 3.3.7 Implement `parse_drop_index()` - handle DROP INDEX
- [ ] 3.3.8 Implement `parse_insert()` - handle INSERT INTO ... VALUES
- [ ] 3.3.9 Implement `parse_update()` - handle UPDATE ... SET ... WHERE
- [ ] 3.3.10 Implement `parse_delete()` - handle DELETE FROM ... WHERE
- [ ] 3.3.11 Implement `parse_select()` - handle SELECT ... FROM ... WHERE ... ORDER BY ... LIMIT
- [ ] 3.3.12 Implement `parse_transaction()` - handle BEGIN/COMMIT/ROLLBACK
- [ ] 3.3.13 Write tests for parser: each statement type, error cases

### 3.4 Expression Parser (WHERE conditions)

- [ ] 3.4.1 Implement `parse_expression()` - parse binary expressions with precedence
- [ ] 3.4.2 Implement `parse_comparison()` - =, >, <, >=, <=, !=, <>
- [ ] 3.4.3 Implement `parse_logical()` - AND, OR, NOT
- [ ] 3.4.4 Implement `parse_like()` - LIKE operator with % and _ wildcards
- [ ] 3.4.5 Implement `parse_in()` - IN (val1, val2, ...)
- [ ] 3.4.6 Implement `parse_between()` - BETWEEN val1 AND val2
- [ ] 3.4.7 Implement `parse_null_check()` - IS [NOT] NULL
- [ ] 3.4.8 Write tests for expressions: all operators, precedence, parentheses

## 4. Query Execution Engine

### 4.1 Executor Core

- [ ] 4.1.1 Create `src/executor/executor.h` with Executor struct
- [ ] 4.1.2 Create `src/executor/result.h` with Row struct, ResultSet struct
- [ ] 4.1.3 Implement `executor_init()` - create executor context
- [ ] 4.1.4 Implement `executor_execute(ast)` - dispatch to appropriate handler
- [ ] 4.1.5 Implement `executor_execute_create_table(ast)`
- [ ] 4.1.6 Implement `executor_execute_drop_table(ast)`
- [ ] 4.1.7 Implement `executor_execute_create_index(ast)`
- [ ] 4.1.8 Implement `executor_execute_drop_index(ast)`
- [ ] 4.1.9 Implement `executor_execute_insert(ast)`
- [ ] 4.1.10 Implement `executor_execute_update(ast)` - requires WHERE
- [ ] 4.1.11 Implement `executor_execute_delete(ast)` - requires WHERE
- [ ] 4.1.12 Implement `executor_execute_select(ast)`
- [ ] 4.1.13 Write tests for executor

### 4.2 WHERE Evaluation

- [ ] 4.2.1 Create `src/executor/eval.h`
- [ ] 4.2.2 Implement `eval_expression(expr, row)` - evaluate expression for a row
- [ ] 4.2.3 Implement `eval_comparison(op, left, right)` - comparison evaluation
- [ ] 4.2.4 Implement `eval_logical(op, left, right)` - AND, OR, NOT
- [ ] 4.2.5 Implement `eval_like(pattern, value)` - pattern matching
- [ ] 4.2.6 Implement `eval_in(values, target)` - IN check
- [ ] 4.2.7 Implement `eval_between(val, start, end)` - BETWEEN check
- [ ] 4.2.8 Write tests for WHERE evaluation

### 4.3 SELECT Processing

- [ ] 4.3.1 Implement `select_init_query()` - setup for SELECT
- [ ] 4.3.2 Implement `select_apply_where()` - filter rows by WHERE
- [ ] 4.3.3 Implement `select_apply_distinct()` - remove duplicate rows
- [ ] 4.3.4 Implement `select_apply_order_by()` - sort rows by ORDER BY column and direction
- [ ] 4.3.5 Implement `select_apply_limit()` - limit result to n rows
- [ ] 4.3.6 Implement `select_project_columns()` - select specific columns
- [ ] 4.3.7 Write tests for SELECT with various combinations

### 4.4 Transaction Execution

- [ ] 4.4.1 Create `src/executor/transaction.h` with Transaction struct (state, changes)
- [ ] 4.4.2 Implement `txn_begin()` - start transaction, set state to ACTIVE
- [ ] 4.4.3 Implement `txn_commit()` - commit transaction
- [ ] 4.4.4 Implement `txn_rollback()` - rollback transaction
- [ ] 4.4.5 Track changes during transaction for rollback
- [ ] 4.4.6 Write tests for transactions

## 5. WAL (Write-Ahead Logging)

### 5.1 WAL Structure

- [ ] 5.1.1 Define WAL_MAGIC = 0x57414C00, WAL_VERSION = 1
- [ ] 5.1.2 Create `src/storage/wal.h` with WAL struct
- [ ] 5.1.3 WAL header: magic, version, page_count, file_size
- [ ] 5.1.4 WAL record types: BEGIN, UPDATE, INSERT, DELETE, COMMIT, ROLLBACK
- [ ] 5.1.5 Implement `wal_open(db_filename)` - opens or creates WAL file
- [ ] 5.1.6 Implement `wal_close()` - closes WAL file
- [ ] 5.1.7 Implement `wal_append_record(type, data, size)` - appends record to WAL
- [ ] 5.1.8 Write tests for WAL structure

### 5.2 WAL Recovery

- [ ] 5.2.1 Implement `wal_recover()` - scan WAL, replay committed transactions
- [ ] 5.2.2 Identify committed vs uncommitted transactions in WAL
- [ ] 5.2.3 Apply committed changes to database
- [ ] 5.2.4 Discard uncommitted changes
- [ ] 5.2.5 Implement `wal_checkpoint()` - checkpoint WAL, truncate if clean
- [ ] 5.2.6 Call checkpoint when WAL reaches 1000 pages
- [ ] 5.2.7 Write tests for WAL recovery

### 5.3 WAL Integration

- [ ] 5.3.1 Modify INSERT to write WAL record before applying
- [ ] 5.3.2 Modify UPDATE to write WAL record before applying
- [ ] 5.3.3 Modify DELETE to write WAL record before applying
- [ ] 5.3.4 Flush WAL to disk on COMMIT before returning success
- [ ] 5.3.5 Integrate WAL recovery into pager_init()

## 6. Server Daemon

### 6.1 Socket Server

- [ ] 6.1.1 Create `src/server/server.h`
- [ ] 6.1.2 Implement `server_init(socket_path)` - create Unix socket
- [ ] 6.1.3 Implement `server_bind_and_listen(socket_path)` - bind and listen
- [ ] 6.1.4 Implement `server_accept_client()` - accept connection
- [ ] 6.1.5 Implement `server_handle_client(client_fd)` - read query, execute, respond
- [ ] 6.1.6 Implement `server_run_loop()` - main accept loop
- [ ] 6.1.7 Write tests for server socket operations

### 6.2 Protocol Handler

- [ ] 6.2.1 Implement `protocol_read_query(client_fd)` - read SQL line from client
- [ ] 6.2.2 Implement `protocol_write_result(client_fd, result)` - write tab-separated results
- [ ] 6.2.3 Implement `protocol_write_error(client_fd, error)` - write ERROR: message
- [ ] 6.2.4 Implement `protocol_write_ok(client_fd, rows_affected)` - write OK n
- [ ] 6.2.5 Close client connection after each query
- [ ] 6.2.6 Write tests for protocol

### 6.3 Lifecycle Management

- [ ] 6.3.1 Implement `server_create_runtime_dir()` - create /run/tinydb/
- [ ] 6.3.2 Implement `server_write_pid()` - write PID to /run/tinydb/tinydb.pid
- [ ] 6.3.3 Implement `server_check_lock()` - check /run/tinydb/tinydb.lock
- [ ] 6.3.4 Implement `server_acquire_lock()` - acquire exclusive lock
- [ ] 6.3.5 Implement `server_handle_signal(signum)` - handle SIGTERM gracefully
- [ ] 6.3.6 Implement `server_cleanup()` - remove PID file, close database
- [ ] 6.3.7 Write tests for lifecycle

### 6.4 Main Entry Point

- [ ] 6.4.1 Parse --help, --version, --db-path arguments
- [ ] 6.4.2 Initialize database (create if not exists)
- [ ] 6.4.3 Initialize server socket
- [ ] 6.4.4 Run server loop
- [ ] 6.4.5 Handle errors and cleanup properly

## 7. CLI Client

### 7.1 CLI Core

- [ ] 7.1.1 Create `src/cli/cli.h`
- [ ] 7.1.2 Implement `cli_interactive_mode()` - read-eval-print loop
- [ ] 7.1.3 Implement `cli_single_query(sql)` - execute single query, print result
- [ ] 7.1.4 Implement `cli_parse_args(argc, argv)` - parse -c, --help, --version
- [ ] 7.1.5 Implement prompt printing "tinydb> "
- [ ] 7.1.6 Implement multi-line input (continue on incomplete statement)
- [ ] 7.1.7 Write tests for CLI core

### 7.2 Meta Commands

- [ ] 7.2.1 Implement `.tables` - query tinydb_master for type='table'
- [ ] 7.2.2 Implement `.schema [table]` - query tinydb_master for CREATE SQL
- [ ] 7.2.3 Implement `.quit` and `.exit` - exit CLI
- [ ] 7.2.4 Implement `.help` - show available meta commands
- [ ] 7.2.5 Implement `cli_is_meta_command(input)` - detect meta commands
- [ ] 7.2.6 Write tests for meta commands

### 7.3 Result Display

- [ ] 7.3.1 Implement `display_result_set(result_set)` - print tabular results
- [ ] 7.3.2 Implement `display_column_headers(columns)` - print column names
- [ ] 7.3.3 Implement `display_separator(columns)` - print aligned separator
- [ ] 7.3.4 Implement `display_row(row, columns)` - print single row aligned
- [ ] 7.3.5 Implement `display_row_count(count, operation)` - print "n rows affected"
- [ ] 7.3.6 Implement `display_error(message)` - print "ERROR: message"
- [ ] 7.3.7 Write tests for result display

### 7.4 Socket Client

- [ ] 7.4.1 Implement `client_connect(socket_path)` - connect to server
- [ ] 7.4.2 Implement `client_send_query(query)` - send SQL to server
- [ ] 7.4.3 Implement `client_read_result()` - read result from server
- [ ] 7.4.4 Implement `client_disconnect()` - close connection
- [ ] 7.4.5 Handle connection errors gracefully
- [ ] 7.4.6 Write tests for socket client

## 8. Systemd Integration

- [ ] 8.1 Create `systemd/tinydb.service` unit file
- [ ] 8.2 Create `systemd/tinydb.socket` unit file (for socket activation)
- [ ] 8.3 Create `systemd/tinydb-tmpfiles.conf` for /run/tinydb/
- [ ] 8.4 Document installation steps in README

## 9. Testing

- [ ] 9.1 Write unit tests for pager (see 2.1.11)
- [ ] 9.2 Write unit tests for btree (see 2.2.8)
- [ ] 9.3 Write unit tests for table operations (see 2.3.10)
- [ ] 9.4 Write unit tests for index operations (see 2.4.9)
- [ ] 9.5 Write unit tests for catalog operations (see 2.5.8)
- [ ] 9.6 Write unit tests for lexer (see 3.1.7)
- [ ] 9.7 Write unit tests for parser (see 3.3.13)
- [ ] 9.8 Write unit tests for expressions (see 3.4.8)
- [ ] 9.9 Write unit tests for executor (see 4.1.13)
- [ ] 9.10 Write unit tests for SELECT (see 4.3.7)
- [ ] 9.11 Write unit tests for transactions (see 4.4.5)
- [ ] 9.12 Write unit tests for WAL (see 5.1.8)
- [ ] 9.13 Write unit tests for WAL recovery (see 5.2.7)
- [ ] 9.14 Write integration tests: server + CLI

## 10. Documentation

- [ ] 10.1 Write README with build and usage instructions
- [ ] 10.2 Write man page for tinydb(1) server
- [ ] 10.3 Write man page for tinydb-cli(1) client
- [ ] 10.4 Verify all specs requirements are met
- [ ] 10.5 Memory leak checking with valgrind