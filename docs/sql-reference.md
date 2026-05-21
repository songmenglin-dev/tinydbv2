# TinyDB SQL Reference

This document describes the SQL syntax supported by TinyDB v2.

## Data Types

TinyDB supports the following column types:

| Type | Description | Storage |
|------|-------------|---------|
| `INTEGER` | 32-bit signed integer | 4 bytes |
| `REAL` | 64-bit floating point | 8 bytes |
| `TEXT` | Variable-length text | Variable |
| `BLOB` | Binary data | Variable |

## CREATE TABLE

Creates a new table in the database.

```sql
CREATE TABLE table_name (
    column_name column_type [constraints],
    column_name column_type [constraints],
    ...
)
```

### Column Constraints

- `PRIMARY KEY` - Column is the primary key (unique, not null)
- `NOT NULL` - Column cannot contain null values
- `UNIQUE` - Column values must be unique
- `DEFAULT value` - Default value for column

### Examples

```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    email TEXT,
    created_at REAL DEFAULT (datetime('now'))
);

CREATE TABLE products (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    price REAL NOT NULL,
    stock INTEGER DEFAULT 0
);
```

## CREATE INDEX

Creates an index on one or more columns.

```sql
CREATE INDEX index_name ON table_name (column_name, ...)
```

### Examples

```sql
CREATE INDEX idx_email ON users (email);
CREATE INDEX idx_name ON users (name, created_at);
```

## SELECT

Queries data from one or more tables.

```sql
SELECT [DISTINCT] column_name [, ...]
FROM table_name
[WHERE condition]
[ORDER BY column_name [ASC | DESC]]
[LIMIT count [OFFSET start]]
```

### Examples

```sql
SELECT * FROM users;
SELECT name, email FROM users WHERE id = 1;
SELECT * FROM users ORDER BY created_at DESC;
SELECT * FROM users LIMIT 10 OFFSET 20;
SELECT DISTINCT category FROM products;
```

### WHERE Clause

Comparison operators: `=`, `!=`, `<`, `<=`, `>`, `>=`

Logical operators: `AND`, `OR`, `NOT`

```sql
SELECT * FROM users WHERE id > 10 AND name = 'Alice';
SELECT * FROM users WHERE email IS NOT NULL;
SELECT * FROM products WHERE price BETWEEN 10 AND 100;
```

## INSERT

Inserts new rows into a table.

```sql
INSERT INTO table_name [(column_name, ...)]
VALUES (value, ...)
```

### Examples

```sql
INSERT INTO users (name, email) VALUES ('Alice', 'alice@example.com');
INSERT INTO users VALUES (1, 'Bob', 'bob@example.com');
INSERT INTO products (name, price) VALUES ('Widget', 19.99);
```

## UPDATE

Updates existing rows in a table.

```sql
UPDATE table_name
SET column_name = value [, ...]
WHERE condition
```

### Examples

```sql
UPDATE users SET email = 'new@example.com' WHERE id = 1;
UPDATE products SET stock = stock - 1 WHERE id = 100;
```

## DELETE

Deletes rows from a table.

```sql
DELETE FROM table_name WHERE condition
```

### Examples

```sql
DELETE FROM users WHERE id = 1;
DELETE FROM products WHERE stock = 0;
```

## DROP TABLE

Removes a table from the database.

```sql
DROP TABLE table_name
```

### Examples

```sql
DROP TABLE users;
DROP TABLE products;
```

## DROP INDEX

Removes an index from the database.

```sql
DROP INDEX index_name ON table_name
```

### Examples

```sql
DROP INDEX idx_email ON users;
```

## Transactions

### BEGIN

Starts a new transaction.

```sql
BEGIN
```

### COMMIT

Commits the current transaction.

```sql
COMMIT
```

### ROLLBACK

Rolls back the current transaction.

```sql
ROLLBACK
```

### Examples

```sql
BEGIN;
INSERT INTO users (name) VALUES ('Alice');
INSERT INTO users (name) VALUES ('Bob');
COMMIT;

BEGIN;
DELETE FROM users WHERE id = 1;
ROLLBACK;  -- Reverts the delete
```

## String Escape Conventions

String literals are enclosed in single quotes:

```sql
SELECT * FROM users WHERE name = 'Alice''s Restaurant';
```

Use two single quotes to escape a single quote within a string.

## Operators

### Arithmetic Operators

| Operator | Description |
|----------|-------------|
| `+` | Addition |
| `-` | Subtraction |
| `*` | Multiplication |
| `/` | Division |
| `%` | Modulo |

### Comparison Operators

| Operator | Description |
|----------|-------------|
| `=` | Equal |
| `!=` or `<>` | Not equal |
| `<` | Less than |
| `<=` | Less than or equal |
| `>` | Greater than |
| `>=` | Greater than or equal |

### Logical Operators

| Operator | Description |
|----------|-------------|
| `AND` | Logical AND |
| `OR` | Logical OR |
| `NOT` | Logical NOT |

### Special Operators

| Operator | Description |
|----------|-------------|
| `IS NULL` | Tests for NULL |
| `IS NOT NULL` | Tests for non-NULL |
| `BETWEEN a AND b` | Tests if value is in range |
| `LIKE pattern` | Pattern matching (future) |
| `IN (a, b, c)` | Tests if value is in list |

## Limitations

- Subqueries not yet supported
- JOINs not yet supported
- Foreign keys not yet supported
- Triggers not yet supported
- Views not yet supported
- ALTER TABLE not yet supported