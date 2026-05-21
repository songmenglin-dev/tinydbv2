# TinyDB v2 Design

## Overview

TinyDB v2 is a lightweight SQL database written in C, similar to SQLite but with client-server architecture.

**Core Features:** ACID transactions, WAL, B+tree storage, CLI client, systemd deployment

## Architecture

See [design/architecture.md](design/architecture.md) for complete system architecture.

## Design Documents

Detailed design documents are located in the `design/` directory:

| Document | Description |
|----------|-------------|
| [design/architecture.md](design/architecture.md) | System architecture, storage format, page layout |
| [design/api-design.md](design/api-design.md) | Public API, internal APIs, data structures |
| [design/build-system.md](design/build-system.md) | Makefile, source organization, test harness |
| [design/cli-ux.md](design/cli-ux.md) | CLI user experience, meta commands, display |
| [design/configuration.md](design/configuration.md) | Configuration file format |
| [design/observability.md](design/observability.md) | Virtual tables, statistics, monitoring |
| [design/security.md](design/security.md) | SQL escaping, permissions, best practices |

## Implementation Tasks

Detailed implementation tasks are in [design/tasks.md](design/tasks.md).

## Context

**Goals:**
- ACID transactions with WAL (Write-Ahead Logging)
- Page-based storage with B+tree for tables, B-tree for indexes
- SQL parser supporting DDL, DML, SELECT with WHERE conditions
- Interactive CLI client with meta-commands
- systemd service unit
- Schema storage via tinydb_master system table

**Non-Goals:**
- Network TCP support (Unix socket only)
- JOINs, subqueries, complex aggregations
- Multiple databases per server instance
- Foreign keys, triggers, views

**Technical Constraints:**
- Pure C, no external dependencies beyond libc
- POSIX-compliant (Linux primary target)
- Single database file with 4096-byte pages
- Single-threaded server with serial query processing