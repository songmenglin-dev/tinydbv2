# CLI/UX Design

## Command Structure

```
tinydb [OPTIONS] [SQL]

OPTIONS:
  -h, --help             Show help
  -v, --version          Show version
  -s, --socket <path>    Connect to socket (default /run/tinydb/tinydb.sock)
  -f, --file <file>      Execute SQL from file
  -c, --command <sql>    Execute single SQL command
  -n, --no-pager         Disable output pager
  -t, --stats            Show query statistics
  -V, --verbose          Verbose output
```

## Interactive Modes

### 1. Interactive Query Mode (default)

```
$ tinydb
TinyDB v2.0.0
Type ".help" for help.

tinydb> CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT);
tinydb> INSERT INTO users VALUES (1, 'Alice');
tinydb> SELECT * FROM users;
id      name
1       Alice

tinydb> .quit
```

### 2. File Input Mode

```
$ tinydb -f script.sql
```

### 3. Single Command Mode

```
$ tinydb -c "SELECT * FROM users WHERE id = 1"
```

## Meta-Commands (dot commands)

Meta-commands are processed locally by the CLI, not sent to the server.

| Command | Description |
|---------|-------------|
| `.help` | Show this help |
| `.quit` / `.exit` | Exit the CLI |
| `.tables` | List all tables |
| `.schema <table>` | Show CREATE TABLE for a table |
| `.indexes <table>` | List indexes on a table |
| `.plan <sql>` | Show query plan (EXPLAIN) |
| `.timer on/off` | Enable/disable query timing |
| `.mode <mode>` | Output mode: `box`, `csv`, `line`, `list` |
| `.headers on/off` | Show/hide column headers |
| `.null <string>` | String to display for NULL |
| `.pager <cmd>` | Set pager command (default: `less -R`) |
| `.shell <cmd>` | Execute shell command |
| `.read <file>` | Read and execute SQL from file |
| `.trace on/off` | Enable server trace |

## Output Modes

### Box Mode (default)

```
+----+----------+
| id | name     |
+----+----------+
| 1  | Alice    |
| 2  | Bob      |
+----+----------+
```

### CSV Mode

```
id,name
1,Alice
2,Bob
```

### Line Mode

```
id = 1
name = Alice

id = 2
name = Bob
```

### List Mode (tab-separated)

```
id      name
1       Alice
2       Bob
```

## Query Timing

When `.timer on`:

```
tinydb> SELECT * FROM users;
id      name
1       Alice
(1 row)

Time: 0.0012s (12ms)
```

## Error Display

```
tinydb> SELECT * FROM nonexistent;
ERROR: no such table: nonexistent
```

Errors are displayed in red (if terminal supports colors) with the error prefix.

## SQL History

- History stored in `~/.tinydb_history`
- Navigation: Up/Down arrows
- Search: Ctrl+R
- History persisted across sessions
- History limit: 10,000 entries

## Interactive Features

### Auto-complete

- SQL keywords (SELECT, INSERT, etc.)
- Table names (after FROM or INTO)
- Column names (after WHERE or SET)
- File paths (after `.read` or `-f`)

### Syntax Highlighting

Keywords: bold cyan
Strings: green
Numbers: magenta
Identifiers: default
Errors: underlined red

### Input Editing

- Emacs-style editing (Ctrl+A, Ctrl+E, Ctrl+K, Ctrl+Y)
- Multi-line SQL (semicolon + newline completes, or blank line)
- Ctrl+C to cancel current input

## Pager Integration

- Results use pager when exceeding terminal height
- Configurable via `.pager` command
- Disable with `-n` flag or `.pager cat`

## Startup Sequence

```
1. Parse CLI arguments
2. Connect to socket
3. Send version query (optional handshake)
4. Enter interactive loop or execute command
```

## Prompt Format

Default: `tinydb> `
Multiline: `   ...> ` (continuation)
Transaction: `tinydb tx> `
Error state: `tinydb !> `

## Quiet Mode

When `-q` or commands piped via stdin:

- No banner
- No `.help` shown on startup
- Minimal error formatting
- Exit code reflects success/failure

```
$ echo "SELECT 1" | tinydb
1
$ echo $?
0
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Query error |
| 2 | Connection error |
| 3 | CLI internal error |
| 130 | SIGINT (Ctrl+C) |
