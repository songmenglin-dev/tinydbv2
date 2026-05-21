## ADDED Requirements

### Requirement: Interactive CLI mode
The CLI client SHALL provide an interactive prompt when started without arguments:
```
$ tinydb-cli
tinydb> SELECT * FROM users;
...
```

#### Scenario: CLI starts in interactive mode
- **WHEN** `tinydb-cli` is run without arguments
- **THEN** a prompt "tinydb> " SHALL be displayed
- **AND** input SHALL be read line by line

### Requirement: Single query mode
The CLI SHALL accept a SQL query via command line:
```bash
$ tinydb-cli "SELECT * FROM users"
```

#### Scenario: Single query executed
- **WHEN** `tinydb-cli "SELECT * FROM users"` is executed
- **THEN** the query SHALL be sent to the server
- **AND** results SHALL be printed to stdout
- **AND** the CLI SHALL exit

### Requirement: Meta-commands
The CLI SHALL support these meta-commands in interactive mode:
- `.tables` - List all tables
- `.schema [table]` - Show CREATE statement for table(s)
- `.quit` or `.exit` - Exit the CLI
- `.help` - Show available meta-commands

#### Scenario: List tables
- **WHEN** user types `.tables` in interactive mode
- **THEN** the CLI SHALL query tinydb_master for type='table'
- **AND** display table names

#### Scenario: Show schema
- **WHEN** user types `.schema users`
- **THEN** the CLI SHALL query tinydb_master for the table's CREATE statement
- **AND** display the SQL

#### Scenario: Quit CLI
- **WHEN** user types `.quit` or `.exit`
- **THEN** the CLI SHALL exit cleanly

### Requirement: Result formatting
Query results SHALL be formatted as:
- Tabular output with aligned columns for SELECT
- Row count displayed for INSERT/UPDATE/DELETE
- Error messages prefixed with "ERROR:"

#### Scenario: SELECT results displayed
- **WHEN** a SELECT returns multiple rows
- **THEN** results SHALL be displayed in aligned columns
- **AND** column headers SHALL be shown
- **AND** a footer SHALL show row count

### Requirement: Server connection
The CLI SHALL connect to the server via Unix domain socket at `/run/tinydb/tinydb.sock`.

#### Scenario: CLI connects to server
- **WHEN** CLI starts and server is running
- **THEN** a connection SHALL be established to the Unix socket
- **AND** queries SHALL be sent over this connection

### Requirement: Connection error handling
The CLI SHALL handle connection failures gracefully.

#### Scenario: Server not running
- **WHEN** CLI is started but server is not running
- **THEN** an error message "ERROR: Cannot connect to server" SHALL be displayed
- **AND** CLI SHALL exit with non-zero status

### Requirement: Multi-line input
The CLI SHALL support multi-line input for long queries (detect semicolon to execute).

#### Scenario: Multi-line query
- **WHEN** user types a query spanning multiple lines
- **AND** ends with semicolon
- **THEN** the complete query SHALL be sent to server