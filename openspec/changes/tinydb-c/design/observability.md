# Observability

## Logging

### Log Levels

```c
typedef enum {
    LOG_ERROR = 0,
    LOG_WARN  = 1,
    LOG_INFO  = 2,
    LOG_DEBUG = 3,
} LogLevel;
```

### Log Destinations

| Destination | Enabled by | Output |
|------------|------------|--------|
| stderr | always | Human-readable with timestamps |
| syslog | `--syslog` or `TINYDB_SYSLOG=1` | Structured syslog |
| file | `--log-file <path>` | Same as stderr format |
| journald | via systemd | Structured JSON |

### Log Format

```
2026-05-21T10:23:45.123456Z ERROR  [storage/wal.c:142] WAL flush failed: input/output error
2026-05-21T10:23:45.124000 INFO   [server/protocol.c:089] Client disconnected: 192.168.1.100
```

Format: `ISO8601timestamp LEVEL [source_file:line] message`

### Log Categories

```c
// Enable specific categories at runtime
#define LOG_CAT_STORAGE    0x01
#define LOG_CAT_PARSER     0x02
#define LOG_CAT_EXECUTOR   0x04
#define LOG_CAT_SERVER     0x08
#define LOG_CAT_WAL        0x10
#define LOG_CAT_TRANSACTION 0x20

// Set via TINYDB_LOG_CATEGORIES=0x1f for storage+parser+executor+server+WAL
```

## Debug Mode

### Compile-Time Debug

```makefile
CFLAGS += -DTINYDB_DEBUG=1  # Enables assert() and extra logging
```

### Runtime Debug Flags

| Flag | Env Variable | Effect |
|------|--------------|--------|
| `TINYDB_DEBUG_HEAP` | | Enable heap allocator tracking |
| `TINYDB_DEBUG_PAGE` | | Log every page read/write |
| `TINYDB_DEBUG_WAL` | | Log every WAL entry |
| `TINYDB_DEBUG_BTREE` | | Log btree split/merge operations |
| `TINYDB_DEBUG_SQL` | | Log parsed AST before execution |

### Debug Meta-Commands

```sql
-- In CLI
EXPLAIN <sql>           -- Show query plan
EXPLAIN QUERY PLAN <sql>  -- Show detailed plan
.DEBUG <category>       -- Enable category logging
.DEBUG OFF             -- Disable debug logging
```

### Query Plan Output

```
tinydb> EXPLAIN SELECT * FROM users WHERE id > 10 ORDER BY name;
+---EXPLAIN---------------------------+
| QUERY PLAN                         |
+------------------------------------+
| SCAN users                          |
|   WHERE id > 10                    |
|   SORT BY name                     |
|   OUTPUT (id, name)                |
+------------------------------------+
```

## Statistics

### Runtime Statistics

```c
typedef struct {
    // Storage
    uint64_t page_reads;
    uint64_t page_writes;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t wal_entries_written;
    uint64_t wal_entries_replayed;
    uint64_t checkpoints;
    
    // SQL
    uint64_t queries_executed;
    uint64_t selects;
    uint64_t inserts;
    uint64_t updates;
    uint64_t deletes;
    uint64_t transactions_begun;
    uint64_t transactions_committed;
    uint64_t transactions_rolled_back;
    
    // Memory
    size_t arena_allocated;
    size_t arena_used;
    size_t peak_memory;
    
    // Timing (microseconds)
    uint64_t total_query_time_us;
    uint64_t total_txn_time_us;
} Statistics;
```

### Statistics Access

| Method | Access |
|--------|--------|
| `.stats` meta-command | Human-readable summary |
| `SELECT * FROM tinydb_master WHERE key = 'stats'` | Structured |
| `TINYDB_STAT_INTERVAL=5` env var | Auto-dump every N seconds |
| SIGUSR1 to server process | Dump stats to log |

### Stats SQL Interface

**tinydb_stats Virtual Table**

```sql
-- Global statistics table (read-only, populated from memory)
CREATE VIRTUAL TABLE tinydb_stats (
    metric_name TEXT PRIMARY KEY,
    metric_value INTEGER,
    metric_type TEXT  -- 'counter', 'gauge', 'timing'
);

-- Per-table statistics table (also virtual, computed on demand)
CREATE VIRTUAL TABLE tinydb_table_stats (
    table_name TEXT PRIMARY KEY,
    row_count INTEGER,
    page_count INTEGER,
    index_count INTEGER,
    total_size_bytes INTEGER
);
```

**Default Statistics:**

| metric_name | metric_type | Description |
|------------|-------------|-------------|
| page_reads | counter | Total pages read from disk |
| page_writes | counter | Total pages written to disk |
| cache_hits | counter | Page cache hits |
| cache_misses | counter | Page cache misses |
| wal_entries_written | counter | WAL entries appended |
| wal_entries_replayed | counter | WAL entries replayed on recovery |
| checkpoints | counter | Number of checkpoints performed |
| queries_executed | counter | Total queries run |
| selects | counter | SELECT statements |
| inserts | counter | INSERT statements |
| updates | counter | UPDATE statements |
| deletes | counter | DELETE statements |
| transactions_begun | counter | BEGIN statements |
| transactions_committed | counter | COMMIT statements |
| transactions_rolled_back | counter | ROLLBACK statements |
| total_query_time_us | timing | Total time spent in queries (microseconds) |
| total_txn_time_us | timing | Total time spent in transactions |
| arena_allocated | gauge | Memory allocated from arena |
| arena_used | gauge | Memory used from arena |
| peak_memory | gauge | Peak memory usage |
| wal_size_bytes | gauge | Current WAL file size |

**Implementation Note:** These are virtual tables implemented as read-only system views. The executor recognizes `tinydb_stats` and `tinydb_table_stats` as special cases, returning computed values from the runtime Statistics struct instead of querying regular tables.

### Prometheus Export (optional)

```
# If compiled with TINYDB_PROMETHEUS=1
GET /metrics  -- Exposes Prometheus-format metrics on a local port
```

## Tracing

### SQL Tracing

```bash
$ TINYDB_TRACE_SQL=1 tinydb-server
2026-05-21T10:00:00.001000 INFO [sql/executor.c:042] TRACE: SELECT id, name FROM users WHERE id > 5
```

### Request Tracing

Each client connection gets a trace ID:

```
2026-05-21T10:00:00.001000 INFO [server/protocol.c:020] [conn-001] Connected
2026-05-21T10:00:00.001500 INFO [server/protocol.c:042] [conn-001] Executing: SELECT * FROM users
2026-05-21T10:00:00.002000 INFO [sql/executor.c:042] [conn-001] OK: returned 10 rows in 2.5ms
2026-05-21T10:00:00.002500 INFO [server/protocol.c:089] [conn-001] Disconnected
```

## Health Checks

### Startup Verification

On server start, verify:

- Socket directory is writable
- Data directory exists and is writable
- Lock file can be acquired
- Database file can be opened
- WAL can be replayed

### Runtime Health

```bash
# Ping the server
$ tinydb -c ".ping"
PONG: OK (connected, 1 query executed)

# Check lock status
$ tinydb -c ".lock"
Lock file: /run/tinydb/tinydb.lock
Status: held
PID: 12345
```

## Panic / Crash Handling

### Assert Failures

```c
// When TINYDB_DEBUG=1 and assert() fails
Assertion failed: (page->refcount > 0) [storage/page.c:284]
Aborted (core dumped)
```

### Server Crash

- SIGSEGV/SIGABRT caught by signal handler
- Current query logged with trace ID
- WAL flushed before exit
- Stack trace printed if debug build

## Trace Output File

```bash
# Direct trace to file
$ tinydb-server --trace /var/log/tinydb/trace.log

# Rotating trace (10MB per file, 5 files)
$ tinydb-server --trace-rotate /var/log/tinydb/trace.log:10m:5
```

## Log Rotation

via `logrotate`:

```text
# /etc/logrotate.d/tinydb
/var/log/tinydb/*.log {
    rotate 7
    daily
    compress
    delaycompress
    notifempty
    sharedscripts
    postrotate
        killall -HUP tinydb-server 2>/dev/null || true
    endscript
}
```
