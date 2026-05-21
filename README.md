# TinyDB v2

A lightweight SQL database written in C, similar to SQLite but with client-server architecture.

## Features

- ACID transactions with WAL (Write-Ahead Logging)
- B+tree storage engine with page cache
- SQL parser and executor
- Client-server architecture via Unix domain sockets
- Systemd integration for production deployment
- Comprehensive test suite

## Building

### Prerequisites

- GCC (or compatible C compiler)
- GNU Make
- Linux (for systemd support)

### Build Commands

```bash
# Build all targets (CLI and server)
make

# Build debug version with symbols
make debug

# Build release version with optimizations
make release

# Clean build artifacts
make clean
```

### Testing

```bash
# Run unit tests
make test
make unit-test

# Run integration tests
make integration-test

# Run with Valgrind memory checker
make valgrind

# Generate coverage report
make coverage
```

## Installation

### Manual Installation

```bash
# Build first
make

# Install to /usr/local/bin (default)
sudo make install

# Install to custom prefix
make PREFIX=/opt/tinydb install
```

### Systemd Installation

For production deployments with systemd:

```bash
# Create tinydb user and group
sudo groupadd tinydb
sudo useradd -r -g tinydb -s /sbin/nologin -d /var/lib/tinydb tinydb

# Create directories
sudo mkdir -p /var/lib/tinydb /var/log/tinydb /run/tinydb
sudo chown tinydb:tinydb /var/lib/tinydb /var/log/tinydb /run/tinydb

# Install binaries
sudo make install

# Install systemd unit files
sudo cp systemd/tinydb.service /etc/systemd/system/
sudo cp systemd/tinydb.socket /etc/systemd/system/
sudo cp systemd/tinydb-tmpfiles.conf /usr/lib/tmpfiles.d/tinydb.conf

# Reload systemd and enable service
sudo systemctl daemon-reload
sudo systemctl enable tinydb.socket
sudo systemctl start tinydb
```

## Running

### Starting the Server

```bash
# Run directly (development)
./build/tinydb-server

# Run via systemd (production)
sudo systemctl start tinydb
```

### Server Options

Environment variables control server behavior:

| Variable | Default | Description |
|----------|---------|-------------|
| `TINYDB_LOG_LEVEL` | `info` | Log level: `debug`, `info`, `warn`, `error` |
| `TINYDB_DATA_DIR` | `/var/lib/tinydb` | Database data directory |
| `TINYDB_PORT` | `5432` | Not used (socket-based) |
| `TINYDB_CACHE_SIZE` | `256` | Page cache size (pages) |

### Using the CLI Client

```bash
# Connect to default socket
./build/tinydb-cli

# Connect to custom socket
./build/tinydb-cli /run/tinydb/tinydb.sock

# Or use TCP (if enabled)
./build/tinydb-cli localhost 5432
```

### SQL Syntax

TinyDB supports standard SQL statements:

```sql
-- Create a table
CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL);

-- Insert data
INSERT INTO users (id, name) VALUES (1, 'Alice');
INSERT INTO users (id, name) VALUES (2, 'Bob');

-- Query data
SELECT * FROM users;
SELECT * FROM users WHERE id = 1;

-- Update data
UPDATE users SET name = 'Alicia' WHERE id = 1;

-- Delete data
DELETE FROM users WHERE id = 2;

-- Create index
CREATE INDEX idx_name ON users(name);

-- Transactions
BEGIN;
INSERT INTO users (id, name) VALUES (3, 'Charlie');
COMMIT;
```

## Architecture

```
src/
├── cli/           # Command-line client
├── server/       # Server daemon
├── sql/          # SQL parser, lexer, analyzer, executor
├── storage/      # Pager, page cache, B+tree, WAL
├── util/         # String utilities, error handling
└── include/      # Public headers

systemd/          # Systemd unit files
tests/
├── unit/         # Unit tests
└── integration/  # Integration tests
```

## Configuration

TinyDB reads configuration from `/etc/tinydb/tinydb.conf` (not implemented yet).

## Development

See [docs/development-workflow.md](docs/development-workflow.md) for detailed development guidelines.

## License

MIT