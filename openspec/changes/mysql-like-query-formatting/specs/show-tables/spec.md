## ADDED Requirements

### Requirement: SHOW TABLES command
The system SHALL support the SQL command `SHOW TABLES` which lists all table names in the current database. The result SHALL be formatted as a single-column table with the header `Tables_in_<dbname>`.

### Requirement: SHOW TABLES output format
When `SHOW TABLES` is executed, the CLI SHALL display:
- A single column with header `Tables_in_<dbname>`
- Box-bordered format with one row per table name
- Footer with row count

#### Scenario: SHOW TABLES returns tables
- **WHEN** user executes `SHOW TABLES` on a database with tables `users` and `orders`
- **THEN** the CLI displays a box table with header `Tables_in_<dbname>` and two rows: `users` and `orders`

#### Scenario: SHOW TABLES returns empty
- **WHEN** user executes `SHOW TABLES` on a database with no tables
- **THEN** the CLI displays an empty box table with header `Tables_in_<dbname>` and footer `0 rows in set`

### Requirement: SHOW TABLES parsing
The parser SHALL recognize `SHOW TABLES` as a valid SQL statement and produce an `AstShowTables` node for execution by the executor.

## REMOVED Requirements

None.