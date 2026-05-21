## ADDED Requirements

### Requirement: BEGIN transaction
The system SHALL support beginning a transaction with `BEGIN`.

#### Scenario: Transaction started
- **WHEN** `BEGIN` is executed
- **THEN** subsequent statements SHALL be part of the transaction
- **AND** changes SHALL NOT be visible to other connections until committed

### Requirement: COMMIT transaction
The system SHALL support committing a transaction with `COMMIT`.

#### Scenario: Transaction committed
- **WHEN** `COMMIT` is executed within an active transaction
- **THEN** all changes since BEGIN SHALL be persisted to the database
- **AND** the transaction SHALL be closed
- **AND** WAL SHALL be flushed to disk

### Requirement: ROLLBACK transaction
The system SHALL support rolling back a transaction with `ROLLBACK`.

#### Scenario: Transaction rolled back
- **WHEN** `ROLLBACK` is executed within an active transaction
- **THEN** all changes since BEGIN SHALL be discarded
- **AND** the database SHALL be restored to its pre-BEGIN state
- **AND** the transaction SHALL be closed

### Requirement: Write-Ahead Log
The system SHALL use WAL for durability:
- Before any change, the intended modification SHALL be written to the WAL
- WAL SHALL be flushed to disk before acknowledging commit
- On crash, WAL SHALL be used to recover incomplete transactions

#### Scenario: Crash during transaction
- **WHEN** the server crashes after COMMIT but before data is written to main database
- **THEN** on restart the committed changes SHALL be recovered from WAL

#### Scenario: Crash during uncommitted transaction
- **WHEN** the server crashes with an active transaction (uncommitted)
- **THEN** on restart the uncommitted changes SHALL NOT be applied

### Requirement: Single active transaction
Only one transaction SHALL be active at a time per connection.

#### Scenario: Nested BEGIN
- **WHEN** `BEGIN` is executed while a transaction is active
- **THEN** an error SHALL be returned

### Requirement: Auto-commit off in transaction
While in a transaction, statements SHALL NOT auto-commit.

#### Scenario: Multiple statements in transaction
- **WHEN** multiple INSERTs are executed after BEGIN
- **THEN** no changes are visible to other connections until COMMIT

### Requirement: UPDATE requires WHERE
The UPDATE statement SHALL require a WHERE clause.

#### Scenario: UPDATE without WHERE
- **WHEN** `UPDATE users SET name = 'Bob'` is executed without WHERE
- **THEN** an error SHALL be returned: "UPDATE requires a WHERE clause"

### Requirement: DELETE requires WHERE
The DELETE statement SHALL require a WHERE clause.

#### Scenario: DELETE without WHERE
- **WHEN** `DELETE FROM users` is executed without WHERE
- **THEN** an error SHALL be returned: "DELETE requires a WHERE clause"