# Security

## File Permissions

### Permission Model

All files use strict permissions by default.

| File/Directory | Owner | Mode | Rationale |
|---------------|-------|------|-----------|
| `/var/lib/tinydb/` | `tinydb:tinydb` | `0750` | Database files |
| `/var/lib/tinydb/db` | `tinydb:tinydb` | `0640` | Database file |
| `/run/tinydb/` | `tinydb:tinydb` | `0755` | Socket + PID |
| `/run/tinydb/tinydb.sock` | `tinydb:tinydb` | `0666` | Socket for clients |
| `/run/tinydb/tinydb.pid` | `tinydb:tinydb` | `0644` | PID file |
| `/run/tinydb/tinydb.lock` | `tinydb:tinydb` | `0644` | Lock file |
| `/var/log/tinydb/` | `tinydb:tinydb` | `0755` | Logs |
| `tinydb-server` | `root:tinydb` | `0550` | Server binary |
| `tinydb-cli` | `root:root` | `0555` | CLI binary |

### Permission Enforcement

```c
// On startup, verify and set correct permissions
int security_set_permissions(const char* data_dir, const char* run_dir) {
    // Set data dir
    if (chown(data_dir, TINYDB_UID, TINYDB_GID) != 0) return -1;
    if (chmod(data_dir, 0750) != 0) return -1;
    
    // Set run dir
    if (chown(run_dir, TINYDB_UID, TINYDB_GID) != 0) return -1;
    if (chmod(run_dir, 0755) != 0) return -1;
    
    // Socket must be world-writable for CLI access
    if (chmod(socket_path, 0666) != 0) return -1;
    
    return 0;
}
```

## Input Validation

### SQL Input

All SQL text from clients is validated:

```c
// 1. Reject NUL bytes
if (memchr(sql, '\0', sql_len)) {
    return error("SQL contains invalid characters");
}

// 2. Reject control characters except newline/tab
for (int i = 0; i < sql_len; i++) {
    char c = sql[i];
    if (c < 0x20 && c != '\n' && c != '\t' && c != '\r') {
        return error("SQL contains invalid control character");
    }
}

// 3. Length limit
if (sql_len > TINYDB_MAX_SQL_LENGTH) {  // 1MB
    return error("SQL statement too long");
}

// 4. Parameterized queries prevent injection
```

### Path Validation

```c
// Validate database paths
int security_validate_path(const char* path) {
    // Must be absolute
    if (path[0] != '/') return -1;
    
    // Must not contain ..
    if (strstr(path, "..")) return -1;
    
    // Must be within allowed directories
    const char* allowed[] = {
        TINYDB_DEFAULT_DATA_DIR,
        "/tmp",
        "/run/tinydb",
        NULL
    };
    // Check prefix match
    ...
    
    return 0;
}
```

### Value Validation

```c
// Validate integer bounds
int64_t validate_integer(uint64_t value, int64_t min, int64_t max) {
    if (value > (uint64_t)max) return max;
    if (value < (uint64_t)min) return min;
    return value;
}

// Validate string length
int validate_string_length(const char* str, size_t max_len) {
    size_t len = strlen(str);
    if (len > max_len) return -1;
    return 0;
}
```

## Sandboxing (systemd)

See `configuration.md` for full systemd security directives.

Key protections:

```ini
[Service]
NoNewPrivileges=true        # Prevent privilege escalation
PrivateTmp=true             # Isolated /tmp
ProtectSystem=strict        # Read-only /usr, /boot, /etc
ProtectHome=true           # Hide user's home directories
ProtectKernelTunables=true  # Prevent kernel param modification
ProtectKernelModules=true   # Prevent module loading
ProtectKernelLogs=true      # Prevent log reading
ProtectClock=true           # Prevent time manipulation
ProtectControlGroups=true   # Prevent cgroup access
ProtectProc=invisible       # Hide processes outside pid namespace
ProcSubset=pid              # Only expose own PID
RestrictNamespaces=true     # Prevent namespace escapes
LockPersonality=true        # Prevent execution in non-standard address spaces
RestrictRealtime=true       # Prevent real-time scheduling
RestrictSUIDSGID=true       # Prevent suid/sgid execution
```

## Resource Limits

```ini
[Service]
LimitNOFILE=65536      # Max file descriptors
LimitNPROC=512         # Max processes
```

## Capabilities

Minimal capabilities for server:

```ini
# Instead of full root, use specific capabilities
AmbientCapabilities=CAP_NET_BIND_SERVICE
```

## Connection Security

### Unix Socket Permissions

- Socket at `/run/tinydb/tinydb.sock` with mode `0666`
- Clients connect as any user with read/write access
- Server validates peer PID on connection

### Connection Limits

```c
#define TINYDB_MAX_CONNECTIONS 100
#define TINYDB_MAX_CONCURRENT_QUERIES 50
```

### Query Timeout

```c
#define TINYDB_QUERY_TIMEOUT_MS 30000  // 30 second default
```

## Secrets Management

### No Hardcoded Secrets

The database stores no secrets internally. If user data requires encryption:

- Encryption is applied at the application layer
- TinyDB does not implement transparent encryption
- Master key management is out of scope

### SQL Injection Prevention

All user input in SQL strings must be escaped:

```sql
-- User provides: O'Brien
-- CLI should use parameterized queries:
PREPARE stmt AS SELECT * FROM users WHERE name = $1;
EXECUTE stmt('O''Brien');

-- Not string concatenation:
-- SELECT * FROM users WHERE name = 'O'Brien'  -- BREAKS!
```

### String Escape Sequences

```c
// Escape single quotes in SQL string literals
// SQL standard: '' (two single quotes) escapes a single quote
// Example: O'Brien -> O''Brien
char* escape_string(const char* input, size_t input_len, size_t* output_len) {
    // Each single quote becomes two single quotes
    // Worst case: all characters are single quotes -> output is 2x input
    char* out = malloc(input_len * 2 + 1);
    if (!out) return NULL;
    
    size_t j = 0;
    for (size_t i = 0; i < input_len; i++) {
        if (input[i] == '\'') {
            out[j++] = '\'';  // First quote of the pair
            out[j++] = '\'';  // Second quote of the pair
        } else {
            out[j++] = input[i];
        }
    }
    out[j] = '\0';
    *output_len = j;
    return out;
}
```

**Important:** The backslash (`\`) is NOT used for escaping in SQL. The SQL standard uses `''` to represent a literal single quote inside a string. The parser and executor must understand this convention.

## Security Audit Checklist

Before any release:

- [ ] All file operations use correct permissions
- [ ] No hardcoded paths outside of config
- [ ] No hardcoded credentials or keys
- [ ] All SQL input validated (NUL bytes, control chars, length)
- [ ] Path traversal prevention (no `..` in paths)
- [ ] Buffer bounds checked on all string operations
- [ ] Connection limits enforced
- [ ] Query timeout enforced
- [ ] systemd security directives applied
- [ ] Resource limits set
- [ ] Log rotation configured
- [ ] Log files not world-readable
- [ ] PID file removed on exit
- [ ] Lock file released on exit
- [ ] SIGTERM exits cleanly (no cleanup skipped)
