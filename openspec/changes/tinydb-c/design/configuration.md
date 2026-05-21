# Configuration

## Configuration Sources (in priority order)

1. **Compile-time constants** — hardcoded project defaults
2. **Environment variables** — runtime overrides for deployment
3. **systemd unit file** — formal deployment parameters

## Compile-Time Constants

```c
// src/util/config.h

#define TINYDB_DEFAULT_SOCKET_PATH  "/run/tinydb/tinydb.sock"
#define TINYDB_DEFAULT_DATA_DIR      "/var/lib/tinydb"
#define TINYDB_DEFAULT_DB_NAME      "db"
#define TINYDB_DEFAULT_PID_FILE     "/run/tinydb/tinydb.pid"
#define TINYDB_DEFAULT_LOCK_FILE    "/run/tinydb/tinydb.lock"

#define TINYDB_PAGE_SIZE            4096
#define TINYDB_CACHE_PAGES         128
#define TINYDB_WAL_SYNC_MODE       1       // 0=nosync, 1=fsync, 2=fdatasync
#define TINYDB_CHECKPOINT_BYTES    (64 * 1024 * 1024)  // 64MB checkpoint threshold
#define TINYDB_MAX_EXPR_DEPTH      32

#define TINYDB_SOCKET_BACKLOG      64
#define TINYDB_RECV_BUF_SIZE       8192

#define TINYDB_TXN_WRITE_AHEAD_LOG 1
```

## Environment Variables

### Server Environment Variables

| Variable | Type | Default | Description |
|----------|------|---------|-------------|
| `TINYDB_SOCKET` | path | `/run/tinydb/tinydb.sock` | Unix socket path |
| `TINYDB_DATA_DIR` | path | `/var/lib/tinydb` | Database directory |
| `TINYDB_DB_NAME` | string | `db` | Database file name |
| `TINYDB_PID_FILE` | path | `/run/tinydb/tinydb.pid` | PID file path |
| `TINYDB_LOCK_FILE` | path | `/run/tinydb/tinydb.lock` | Lock file path |
| `TINYDB_CACHE_SIZE` | int | `128` | Page cache size (pages) |
| `TINYDB_WAL_SYNC` | int | `1` | WAL sync mode (0=none, 1=fsync, 2=fdatasync) |
| `TINYDB_CHECKPOINT_THRESHOLD` | int | `67108864` | Checkpoint trigger bytes |
| `TINYDB_LOG_LEVEL` | string | `info` | Log level (error/warn/info/debug) |
| `TINYDB_READONLY` | int | `0` | Open database read-only |
| `TINYDB_TRACE_SQL` | int | `0` | Log all SQL statements |
| `TINYDB_STAT_INTERVAL` | int | `0` | Stats dump interval in seconds (0=disabled) |

### CLI Client Environment Variables

| Variable | Type | Default | Description |
|----------|------|---------|-------------|
| `TINYDB_SOCKET` | path | `/run/tinydb/tinydb.sock` | Server socket path |
| `TINYDB_HISTORY` | path | `~/.tinydb_history` | SQL history file |
| `TINYDB_PAGER` | string | `less -R` | Output pager |
| `TINYDB_PROMPT` | string | `tinydb> ` | CLI prompt format |

## Config Loading

```c
// src/util/config.c
typedef struct {
    const char* socket_path;
    const char* data_dir;
    const char* db_name;
    const char* pid_file;
    const char* lock_file;
    int cache_pages;
    int wal_sync_mode;
    uint64_t checkpoint_threshold;
    LogLevel log_level;
    int readonly;
    int trace_sql;
    int stat_interval;
} Config;

Config config_load(void) {
    Config cfg = {
        .socket_path    = getenv("TINYDB_SOCKET") ?: TINYDB_DEFAULT_SOCKET_PATH,
        .data_dir       = getenv("TINYDB_DATA_DIR") ?: TINYDB_DEFAULT_DATA_DIR,
        .db_name        = getenv("TINYDB_DB_NAME") ?: TINYDB_DEFAULT_DB_NAME,
        .pid_file       = getenv("TINYDB_PID_FILE") ?: TINYDB_DEFAULT_PID_FILE,
        .lock_file      = getenv("TINYDB_LOCK_FILE") ?: TINYDB_DEFAULT_LOCK_FILE,
        .cache_pages    = atoi(getenv("TINYDB_CACHE_SIZE") ?: "128"),
        .wal_sync_mode  = atoi(getenv("TINYDB_WAL_SYNC") ?: "1"),
        .checkpoint_threshold = atoll(getenv("TINYDB_CHECKPOINT_THRESHOLD") ?: "67108864"),
        .log_level      = parse_log_level(getenv("TINYDB_LOG_LEVEL") ?: "info"),
        .readonly       = atoi(getenv("TINYDB_READONLY") ?: "0"),
        .trace_sql      = atoi(getenv("TINYDB_TRACE_SQL") ?: "0"),
        .stat_interval  = atoi(getenv("TINYDB_STAT_INTERVAL") ?: "0"),
    };
    return cfg;
}
```

## systemd Service Configuration

```ini
# systemd/tinydb.service
[Unit]
Description=TinyDB v2 Embedded Database
After=network.target
Documentation=man:tinydb(1)

[Service]
Type=notify
ExecStart=/usr/local/bin/tinydb-server
User=tinydb
Group=tinydb
RuntimeDirectory=tinydb
RuntimeDirectoryMode=0755
StateDirectory=tinydb
StateDirectoryMode=0750
LogsDirectory=tinydb
LogsDirectoryMode=0755
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ProtectKernelTunables=true
ProtectKernelModules=true
ProtectKernelLogs=true
ProtectClock=true
ProtectControlGroups=true
ProtectProc=invisible
ProcSubset=pid
RestrictNamespaces=true
LockPersonality=true
RestrictRealtime=true
RestrictSUIDSGID=true

# Allow read on /var/lib/tinydb
ReadOnlyPaths=/
ReadWritePaths=/var/lib/tinydb
ReadWritePaths=/run/tinydb

# Allow binding to /run/tinydb/tinydb.sock
BindPaths=/run/tinydb

# Allow kill signal
KillMode=mixed
SendSIGKILL=yes

# Restart policy
Restart=on-failure
RestartSec=5s
TimeoutStopSec=30s

# Resource limits
LimitNOFILE=65536
LimitNPROC=512

# Environment
Environment=TINYDB_LOG_LEVEL=info
Environment=TINYDB_TRACE_SQL=0

[Install]
WantedBy=multi-user.target
```

## Runtime Configuration Validation

```c
// Validate config at startup
int config_validate(Config* cfg) {
    if (cfg->cache_pages < 4 || cfg->cache_pages > 4096) {
        fprintf(stderr, "TINYDB_CACHE_SIZE must be 4-4096\n");
        return -1;
    }
    if (cfg->wal_sync_mode > 2) {
        fprintf(stderr, "TINYDB_WAL_SYNC must be 0, 1, or 2\n");
        return -1;
    }
    if (cfg->checkpoint_threshold < (1024 * 1024)) {
        fprintf(stderr, "TINYDB_CHECKPOINT_THRESHOLD minimum 1MB\n");
        return -1;
    }
    if (access(cfg->data_dir, R_OK | W_OK) != 0) {
        fprintf(stderr, "Data directory %s not accessible\n", cfg->data_dir);
        return -1;
    }
    return 0;
}
```

## Compile-Time Feature Flags

Optional features controlled at compile time via `-D`:

```makefile
# Enable/disable features
CFLAGS += -DTINYDB_DEBUG=0
CFLAGS += -DTINYDB_SANITIZE=0
CFLAGS += -DTINYDB_DISABLE_WAL=0
```

| Flag | Default | Description |
|------|---------|-------------|
| `TINYDB_DEBUG` | 0 | Debug mode (extra asserts, logging) |
| `TINYDB_SANITIZE` | 0 | Enable ASAN/MSAN |
| `TINYDB_DISABLE_WAL` | 0 | Disable WAL (for testing only) |
