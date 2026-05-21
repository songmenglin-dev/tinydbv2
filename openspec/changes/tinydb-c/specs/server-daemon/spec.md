## ADDED Requirements

### Requirement: Unix socket listener
The server SHALL listen on Unix domain socket `/run/tinydb/tinydb.sock` for client connections.

#### Scenario: Server starts
- **WHEN** the server daemon starts
- **THEN** it SHALL create the socket at `/run/tinydb/tinydb.sock`
- **AND** set socket permissions to 0666 (read/write for all)
- **AND** begin listening for connections

### Requirement: Request/Response Protocol
The server SHALL use a simple text-based protocol:

**Request:** SQL string ending with newline
**Response:** Tab-separated values per row, newline to end result set, then either:
- "OK n" where n is rows affected (for INSERT/UPDATE/DELETE)
- "ERROR: message" for errors

#### Scenario: SELECT query executed
- **WHEN** server receives `SELECT * FROM users\n`
- **THEN** server SHALL return tab-separated rows
- **AND** end with newline
- **AND** rows SHALL be one per line

#### Scenario: INSERT executed
- **WHEN** server receives `INSERT INTO users VALUES (1, 'Alice')\n`
- **THEN** server SHALL return "OK 1\n"

#### Scenario: Error occurred
- **WHEN** server encounters an error during query
- **THEN** server SHALL return "ERROR: descriptive message\n"

### Requirement: One query per connection
Each client connection SHALL handle one query then close.

#### Scenario: Query completed
- **WHEN** a query has been executed and results sent
- **THEN** the connection SHALL be closed by the server

### Requirement: PID file
The server SHALL write its PID to `/run/tinydb/tinydb.pid`.

#### Scenario: Server starts
- **WHEN** the server daemon starts successfully
- **THEN** it SHALL write its PID to `/run/tinydb/tinydb.pid`

### Requirement: Data directory
The server SHALL store database files in `/var/lib/tinydb/db`.

#### Scenario: Database stored
- **WHEN** a database is created or opened
- **THEN** files SHALL be stored in `/var/lib/tinydb/`
- **AND** main database file SHALL be `/var/lib/tinydb/db`

### Requirement: Initialization
On first start with no database, the server SHALL create a new database with tinydb_master table.

#### Scenario: First start
- **WHEN** server starts and no database exists at `/var/lib/tinydb/db`
- **THEN** a new database file SHALL be created
- **AND** tinydb_master table SHALL be created automatically

### Requirement: Single instance lock
The server SHALL prevent multiple instances using a lock file at `/run/tinydb/tinydb.lock`.

#### Scenario: Second server start attempt
- **WHEN** server is already running
- **AND** a second server start is attempted
- **THEN** the second instance SHALL fail with "ERROR: Database already in use"
- **AND** exit with non-zero status

### Requirement: Graceful shutdown
The server SHALL handle SIGTERM gracefully:
- Close database properly
- Flush any pending writes
- Remove PID file
- Exit cleanly

#### Scenario: SIGTERM received
- **WHEN** server receives SIGTERM signal
- **THEN** it SHALL complete any in-progress query
- **AND** close database cleanly
- **AND** remove PID file
- **AND** exit with code 0