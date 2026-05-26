## Why

TinyDB v2 currently returns query results in a minimal text format that lacks the readability and information density of MySQL's CLI client. Users cannot quickly see available tables, understand table structures, or parse column-aligned data from wide result sets. Improving output formatting will make the database significantly more usable for development and debugging.

## What Changes

- **New SQL commands**: Add `SHOW TABLES` and `DESC <table>` / `DESCRIBE <table>` syntax support
- **Formatted output**: Implement MySQL-like table formatting with:
  - Box-drawing characters for borders (`|`, `+`, `-`)
  - Column-aligned data with proper padding
  - Header row with column names
  - Footer with row count and execution time
- **Table metadata support**: `SHOW TABLES` displays all tables in the current database with proper formatting
- **Table structure display**: `DESC table` shows column name, type, nullability, and key info

## Capabilities

### New Capabilities

- `formatted-output`: Implements MySQL-style table formatting with box borders, aligned columns, header/footer rows
- `show-tables`: New SQL command to list all tables in the database with formatted output
- `desc-table`: New SQL command (`DESC`/`DESCRIBE`) to display table structure (columns, types, constraints)
- `execution-time`: Displays query execution time in milliseconds in result footer
- `result-summary`: Shows number of rows returned in result footer

### Modified Capabilities

- (none — existing capabilities unchanged)

## Impact

- **CLI** (`src/cli/`): Major updates to output formatting logic
- **Parser** (`src/sql/`): Add parsing for `SHOW TABLES` and `DESC`/`DESCRIBE` statements
- **Executor** (`src/sql/`): Add execution handlers for new statement types
- **Protocol**: No changes to wire protocol (formatting is CLI-side presentation only)