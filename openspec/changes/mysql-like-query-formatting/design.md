## Context

TinyDB v2 currently returns query results in a minimal format (e.g., `Query OK, N row(s) returned` followed by raw `ROW:` lines). The CLI has basic box-mode output (`OUTPUT_MODE_BOX`) but it is not fully implemented and lacks professional formatting. The server uses Unix domain sockets with a simple text protocol — all formatting decisions are made on the CLI side.

## Goals / Non-Goals

**Goals:**
- MySQL-compatible `SHOW TABLES` and `DESC <table>` SQL commands
- Box-drawing table formatting with proper column alignment
- Formatted headers with column names and separator lines
- Result footer with row count and execution time
- Execution time tracking (millisecond precision)

**Non-Goals:**
- Changing the wire protocol between CLI and server
- Implementing SQL standard `INFORMATION_SCHEMA` (future)
- Color terminal support (future)
- Pager integration beyond current `less -R` support

## Decisions

### 1. Formatting is CLI-only

All table formatting happens in the CLI client, not on the server. This keeps the protocol simple and allows different UIs (CLI, web UI, tools) to format output differently. The server sends raw ROW: lines with tab-separated column values.

**Alternatives considered:**
- Server sends pre-formatted box characters → Breaks parsing for non-terminal UIs
- Server sends JSON with metadata → More protocol complexity for this project

### 2. Box format structure

```
+------------+-------------+------------+------------+
| column1    | column2     | column3    | column4    |
+------------+-------------+------------+------------+
| value1     | value2      | value3     | value4     |
+------------+-------------+------------+------------+
```

- `+` corners and junctions
- `-` horizontal separators
- `|` vertical separators
- Column width = max(longest value in column, header) + 1 padding on each side

**Alternatives considered:**
- ASCII-only without box drawing (`+-----+-----+`) — less readable for wide tables
- Simple space-padded columns without vertical bars — harder to scan visually

### 3. New SQL statement types

`SHOW TABLES` — parses as a new `AstShowTables` node type. No special grammar entry needed; it can be handled as a SELECT variant in the executor since the catalog already knows all table names.

`DESC <table>` / `DESCRIBE <table>` — parses as a new `AstDescribeTable` node. The executor retrieves column metadata from the catalog entry.

**Alternatives considered:**
- `SHOW TABLES` as `SELECT ... FROM information_schema` — requires full INFORMATION_SCHEMA implementation
- `DESC` as a meta-command (not SQL) — deviates from MySQL compatibility

### 4. Execution time measurement

Time from when the query is sent to when the first response byte is received. Use `gettimeofday()` for millisecond precision. Display in the footer: `N rows in set (X.XX sec)`.

**Alternatives considered:**
- Server-side timing and inclusion in protocol — adds complexity
- Microsecond precision — overkill for this use case

### 5. Result buffer protocol additions

For `SHOW TABLES` and `DESC`, the server response uses the same ROW: format, but with different column meanings:
- `SHOW TABLES`: Single column `Tables_in_<dbname>`
- `DESC table`: Columns `Field`, `Type`, `Null`, `Key`, `Default`, `Extra`

## Risks / Trade-offs

| Risk | Mitigation |
|------|-----------|
| Wide columns cause very long lines | Cap at terminal width with horizontal scroll hint |
| Unicode/UTF-8 column data misaligned | Assume single-byte for now; future UTF-8 awareness |
| Execution time inaccurate on slow systems | Use monotonic clock where possible |
| DESC output differs from MySQL | Accept minor differences; core info (name+type) is present |