## ADDED Requirements

### Requirement: DESC and DESCRIBE commands
The system SHALL support both `DESC <table>` and `DESCRIBE <table>` SQL commands as synonyms, which display the structure (column definitions) of a specified table. The result SHALL be a table with columns: `Field`, `Type`, `Null`, `Key`, `Default`, `Extra`.

### Requirement: DESC output format
When `DESC users` is executed, the CLI SHALL display a box-bordered table with columns: `Field`, `Type`, `Null`, `Key`, `Default`, `Extra`. Each row represents a column in the table. Example:
```
+-------------+-------------+------+-------------+-------+--------+
| Field       | Type        | Null | Key         | Default | Extra |
+-------------+-------------+------+-------------+-------+--------+
| id          | integer     | NO   | PRI         | NULL    |       |
| name        | text        | YES  |             | NULL    |       |
| email       | text        | YES  | UNI         | NULL    |       |
+-------------+-------------+------+-------------+-------+--------+
```

#### Scenario: DESC on existing table
- **WHEN** user executes `DESC users` on a table with columns `id` (integer, primary key) and `name` (text, nullable)
- **THEN** the CLI displays a box table with 6 columns and 2 data rows showing each column's metadata

#### Scenario: DESC on non-existent table
- **WHEN** user executes `DESC nonexistent`
- **THEN** the CLI displays an error message indicating the table does not exist

### Requirement: DESC parsing
The parser SHALL recognize `DESC <table>` and `DESCRIBE <table>` as valid SQL statements and produce an `AstDescribeTable` node for execution by the executor.

### Requirement: Column metadata retrieval
The executor SHALL retrieve column metadata (name, type, nullability, key info, default value) from the catalog entry for the specified table.

## REMOVED Requirements

None.