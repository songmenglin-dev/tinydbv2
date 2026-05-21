## Context

We are building a lightweight SQL database in C from scratch. The goal is a SQLite-alternative that runs as a client-server daemon rather than embedded. Target deployment: Linux systems with systemd. Users interact via CLI client that connects to the server over a Unix domain socket.

Current state: Empty project. No existing code.

Constraints:
- Pure C, no external dependencies beyond libc
- POSIX-compliant (Linux primary target)
- Single-file database (one file per database)
- No network protocol initially - Unix socket only

## Goals / Non-Goals

**Goals:**
- ACID transactions with WAL (Write-Ahead Logging)
- Page-based storage with B+tree indexes
- SQL parser supporting comprehensive DDL, DML, and SELECT
- Interactive CLI client with meta-commands
- systemd service unit
- Persistent storage survive restarts
- Schema storage via tinydb_master system table

**Non-Goals:**
- Network TCP support (Unix socket only)
- JOINs, subqueries, complex aggregations
- Multiple databases in single server (single database per instance)
- Foreign keys, triggers, views
- Client library (just CLI is sufficient)

## SQL Surface Area

### Supported Statements

**DDL:**
- `CREATE TABLE table_name (col1 TYPE, col2 TYPE, ...)`
- `CREATE TABLE IF NOT EXISTS table_name (...)`
- `DROP TABLE table_name`
- `DROP TABLE IF EXISTS table_name`
- `CREATE INDEX idx_name ON table_name(column)`
- `DROP INDEX idx_name`

**DML:**
- `INSERT INTO table_name VALUES (val1, val2, ...)`
- `UPDATE table_name SET col1 = val1 [, col2 = val2 ...] WHERE condition`
- `DELETE FROM table_name WHERE condition`

**SELECT:**
- `SELECT columns FROM table_name [WHERE condition] [ORDER BY col [ASC|DESC]] [LIMIT n] [DISTINCT]`

**Transactions:**
- `BEGIN`
- `COMMIT`
- `ROLLBACK`

### WHERE Conditions

Supported operators:
- Comparison: `=`, `>`, `<`, `>=`, `<=`, `!=`, `<>`
- Logical: `AND`, `OR`, `NOT`
- Special: `LIKE`, `IN (...)`, `BETWEEN val1 AND val2`, `IS NULL`, `IS NOT NULL`

### Data Types

- `INTEGER` (64-bit signed)
- `REAL` (64-bit floating point)
- `TEXT` (UTF-8 string, SQL-standard escaping with `''`)
- `BLOB` (binary data)

## Decisions

### 1. Storage Format
**Decision:** Single database file with page-based structure (4KB pages), header page + data pages + freelist

**Rationale:** SQLite-proven approach. Simple to implement, reliable, and allows future expansion.

### 2. Schema Storage (tinydb_master)
**Decision:** Schema stored in special system table `tinydb_master` with columns: `type`, `name`, `tbl_name`, `sql`

**Rationale:** SQLite-compatible approach. Schema is queryable via SELECT, consistent with SQL expectations.

**Structure:**
```
tinydb_master columns:
  - type: TEXT ('table' or 'index')
  - name: TEXT (table/index name)
  - tbl_name: TEXT (parent table name for indexes)
  - sql: TEXT (CREATE statement)
```

### 3. Index Structure
**Decision:** B+tree for table data, B-tree for indexes

**Rationale:** B+tree better for range scans (ORDER BY). Standard approach used by most DBs.

### 4. Transaction Model
**Decision:** Write-Ahead Logging (WAL) with checkpointing

**Rationale:** WAL provides good write performance, crash recovery, and reader-writer concurrency.

### 5. SQL Parser Approach
**Decision:** Recursive descent parser, hand-written

**Rationale:** Sufficient for our SQL surface, no external dependencies, full control.

### 6. Client-Server Protocol
**Decision:** Simple request-response over Unix socket, text-based SQL with tabular results

**Rationale:** Keeps protocol simple. CLI sends SQL string, server returns tab-separated results or error.

**Protocol:**
- Client sends: SQL string ending with newline
- Server responds: Tab-separated values per row, newline to end, "OK n" for affected rows, "ERROR: message" for errors

### 7. Concurrency Model
**Decision:** Single-threaded server, serial query processing, fcntl locking for file access

**Rationale:** Simpler implementation. File locking provides safety for basic concurrency.

### 8. UPDATE/DELETE Safety
**Decision:** WHERE clause REQUIRED for UPDATE and DELETE statements

**Rationale:** Prevents accidental full-table modifications. Returns error if WHERE omitted.

## Risks / Trade-offs

- [Risk] Data corruption on crash during write → [Mitigation] WAL ensures atomic writes; recovery on startup
- [Risk] Large datasets won't fit in memory → [Mitigation] B+tree provides efficient disk access; LIMIT prevents unbounded queries
- [Risk] Parser bugs cause unexpected behavior → [Mitigation] Comprehensive test suite
- [Risk] Single-threaded limits throughput → [Mitigation] Sufficient for target use case (lightweight workloads)

## Open Questions

1. ~~What SQL data types to support?~~ → INTEGER, REAL, TEXT, BLOB
2. ~~How to handle schema?~~ → tinydb_master system table
3. Max database size limit? (2^32 pages = 16TB with 4KB pages, practical limit ~1TB)
4. WAL checkpoint frequency? (default: when WAL reaches 1000 pages)