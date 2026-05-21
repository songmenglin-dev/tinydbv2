## ADDED Requirements

### Requirement: CREATE TABLE statement
The parser SHALL support `CREATE TABLE` syntax:
```sql
CREATE TABLE table_name (column_definitions)
```
Where column_definitions is comma-separated column_name TYPE pairs.

#### Scenario: Simple table created
- **WHEN** parsing `CREATE TABLE users (id INTEGER, name TEXT)`
- **THEN** the parser SHALL produce a valid AST with table name and columns

### Requirement: CREATE TABLE IF NOT EXISTS
The parser SHALL support `CREATE TABLE IF NOT EXISTS` to avoid errors when table exists.

```sql
CREATE TABLE IF NOT EXISTS table_name (...)
```

#### Scenario: Table with IF NOT EXISTS
- **WHEN** parsing `CREATE TABLE IF NOT EXISTS users (id INTEGER)`
- **THEN** the parser SHALL produce AST with if_not_exists flag set

### Requirement: DROP TABLE statement
The parser SHALL support `DROP TABLE` to remove tables.

```sql
DROP TABLE table_name
```

#### Scenario: Table dropped
- **WHEN** parsing `DROP TABLE users`
- **THEN** the parser SHALL produce AST with DROP TABLE type and table name

### Requirement: DROP TABLE IF EXISTS
The parser SHALL support `DROP TABLE IF EXISTS` to avoid errors when table doesn't exist.

```sql
DROP TABLE IF EXISTS table_name
```

#### Scenario: Table drop with IF EXISTS
- **WHEN** parsing `DROP TABLE IF EXISTS users`
- **THEN** the parser SHALL produce AST with if_exists flag set

### Requirement: CREATE INDEX statement
The parser SHALL support `CREATE INDEX` syntax:
```sql
CREATE INDEX index_name ON table_name(column)
```

#### Scenario: Index created
- **WHEN** parsing `CREATE INDEX idx_name ON users(name)`
- **THEN** the parser SHALL produce AST with index name, table name, and column

### Requirement: DROP INDEX statement
The parser SHALL support `DROP INDEX` syntax:
```sql
DROP INDEX index_name
```

#### Scenario: Index dropped
- **WHEN** parsing `DROP INDEX idx_name`
- **THEN** the parser SHALL produce AST with index name

### Requirement: SELECT statement
The parser SHALL support full SELECT syntax:
```sql
SELECT columns FROM table_name [WHERE condition] [ORDER BY column [ASC|DESC]] [LIMIT n] [DISTINCT]
```

#### Scenario: Simple select
- **WHEN** parsing `SELECT * FROM users`
- **THEN** the parser SHALL produce AST with star columns and table name

#### Scenario: Select with WHERE
- **WHEN** parsing `SELECT name FROM users WHERE id = 1`
- **THEN** the parser SHALL produce AST with column list, table, and WHERE condition

#### Scenario: Select with ORDER BY and LIMIT
- **WHEN** parsing `SELECT * FROM users ORDER BY id DESC LIMIT 10`
- **THEN** the parser SHALL produce AST with order_by column, direction DESC, and limit 10

#### Scenario: Select with DISTINCT
- **WHEN** parsing `SELECT DISTINCT name FROM users`
- **THEN** the parser SHALL produce AST with distinct flag set

#### Scenario: Select with all clauses
- **WHEN** parsing `SELECT DISTINCT name FROM users WHERE id > 5 ORDER BY name ASC LIMIT 20`
- **THEN** the parser SHALL produce AST with all clauses properly parsed

### Requirement: INSERT statement
The parser SHALL support `INSERT` syntax:
```sql
INSERT INTO table_name VALUES (value1, value2, ...)
```

#### Scenario: Insert values
- **WHEN** parsing `INSERT INTO users VALUES (1, 'Alice')`
- **THEN** the parser SHALL produce AST with table name and value list

### Requirement: UPDATE statement (WHERE required)
The parser SHALL support `UPDATE` syntax:
```sql
UPDATE table_name SET column = value [, column2 = value2 ...] WHERE condition
```

#### Scenario: Update with WHERE
- **WHEN** parsing `UPDATE users SET name = 'Bob' WHERE id = 1`
- **THEN** the parser SHALL produce AST with table, set clause, and WHERE

#### Scenario: Update without WHERE
- **WHEN** parsing `UPDATE users SET name = 'Bob'`
- **THEN** the parser SHALL produce an error AST indicating missing WHERE

### Requirement: DELETE statement (WHERE required)
The parser SHALL support `DELETE` syntax:
```sql
DELETE FROM table_name WHERE condition
```

#### Scenario: Delete with WHERE
- **WHEN** parsing `DELETE FROM users WHERE id = 1`
- **THEN** the parser SHALL produce AST with table name and WHERE condition

#### Scenario: Delete without WHERE
- **WHEN** parsing `DELETE FROM users`
- **THEN** the parser SHALL produce an error AST indicating missing WHERE

### Requirement: Transaction statements
The parser SHALL support transaction statements:
```sql
BEGIN
COMMIT
ROLLBACK
```

#### Scenario: Transaction begun
- **WHEN** parsing `BEGIN`
- **THEN** the parser SHALL produce AST with BEGIN transaction type

#### Scenario: Transaction committed
- **WHEN** parsing `COMMIT`
- **THEN** the parser SHALL produce AST with COMMIT transaction type

#### Scenario: Transaction rolled back
- **WHEN** parsing `ROLLBACK`
- **THEN** the parser SHALL produce AST with ROLLBACK transaction type

### Requirement: Column data types
The parser SHALL support these column types:
- INTEGER (64-bit signed)
- REAL (64-bit floating point)
- TEXT (UTF-8 string)
- BLOB (binary data)

#### Scenario: Various types parsed
- **WHEN** parsing column definitions with all supported types
- **THEN** each type SHALL be correctly identified in the AST

### Requirement: WHERE condition operators
The parser SHALL support WHERE conditions with:
- Comparison: `=`, `>`, `<`, `>=`, `<=`, `!=`, `<>`
- Logical: `AND`, `OR`, `NOT`
- Special: `LIKE`, `IN`, `BETWEEN`, `IS NULL`, `IS NOT NULL`

#### Scenario: Comparison operators
- **WHEN** parsing `SELECT * FROM users WHERE id > 5 AND age <= 30`
- **THEN** the parser SHALL produce AST with GT, LTE operators and AND

#### Scenario: LIKE pattern matching
- **WHEN** parsing `SELECT * FROM users WHERE name LIKE 'A%'`
- **THEN** the parser SHALL produce AST with LIKE operator

#### Scenario: IN clause
- **WHEN** parsing `SELECT * FROM users WHERE id IN (1, 2, 3)`
- **THEN** the parser SHALL produce AST with IN operator and value list

#### Scenario: BETWEEN clause
- **WHEN** parsing `SELECT * FROM users WHERE age BETWEEN 18 AND 65`
- **THEN** the parser SHALL produce AST with BETWEEN operator

#### Scenario: NULL checks
- **WHEN** parsing `SELECT * FROM users WHERE name IS NOT NULL`
- **THEN** the parser SHALL produce AST with IS NOT NULL operator

### Requirement: String escaping
The parser SHALL handle SQL-standard string escaping where `''` represents a single quote inside a string.

#### Scenario: Escaped quote
- **WHEN** parsing `INSERT INTO users VALUES ('O''Brien')`
- **THEN** the parser SHALL produce AST with string value `O'Breien`