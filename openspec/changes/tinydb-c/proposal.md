## Why

We need a lightweight, embedded SQL database written in C that provides SQLite-like functionality with a CLI interface, systemd integration for server deployment, and persistent storage. This addresses the need for a simple, zero-dependency database that can run as a long-running service accessible via CLI, suitable for embedded systems or dedicated database servers.

## What Changes

- Create a new C-based SQL database engine with persistent storage
- Implement a CLI client for interactive database access
- Add systemd service configuration for server deployment
- Support standard SQL operations: CREATE TABLE, SELECT, UPDATE, DELETE
- Implement basic transaction support (BEGIN, COMMIT, ROLLBACK)
- Build a client-server architecture with a daemon process

## Capabilities

### New Capabilities

- **storage-engine**: Page-based storage with B-tree indexes for efficient data access
- **sql-parser**: SQL parsing for CREATE, SELECT, INSERT, UPDATE, DELETE, and transaction statements
- **cli-client**: Interactive command-line interface for querying the database
- **server-daemon**: Long-running server process accepting client connections
- **systemd-service**: Systemd unit file for managing the database server
- **transaction-support**: ACID transaction support with write-ahead logging

### Modified Capabilities

- (none - new project)

## Impact

- New codebase in C targeting POSIX systems
- Binary: `tinydb` (server daemon)
- Binary: `tinydb-cli` (client application)
- Data directory: `/var/lib/tinydb/` for persistent storage
- Config directory: `/etc/tinydb/` for configuration
- Socket: `/run/tinydb/tinydb.sock` for client-server communication