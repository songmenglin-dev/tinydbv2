## ADDED Requirements

### Requirement: Row count display
The CLI SHALL display the number of rows returned in the result footer. The format SHALL be `N row(s) in set` for SELECT queries and `Query OK, N row(s) affected` for DML statements.

### Requirement: Row count for SELECT
For `SELECT` queries, the footer line SHALL be formatted as part of the box table border when using box format, or as a separate line when not using box format.

#### Scenario: SELECT returns rows
- **WHEN** user executes `SELECT * FROM users` and 3 rows are returned
- **THEN** the footer displays `3 rows in set (X.XX sec)` after the box table

#### Scenario: SELECT returns no rows
- **WHEN** user executes `SELECT * FROM users WHERE id = 9999` and 0 rows are returned
- **THEN** the footer displays `0 rows in set (X.XX sec)` after the box table (empty data section)

### Requirement: Row count for DML
For `INSERT`, `UPDATE`, and `DELETE` statements, the CLI SHALL display `Query OK, N row(s) affected` where N is the number of rows affected.

## REMOVED Requirements

None.