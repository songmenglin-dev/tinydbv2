## ADDED Requirements

### Requirement: Page-based storage format
The storage engine SHALL use a page-based structure with 4KB fixed-size pages.

#### Scenario: Database file opened
- **WHEN** a new database file is created
- **THEN** the first page SHALL be a header page
- **AND** subsequent pages SHALL be data pages

### Requirement: Header page structure
The header page (page 1) SHALL contain:
- 4-byte magic number identifying TinyDB format (`TNYD`)
- 2-byte version number
- 4-byte page count
- 4-byte freelist head page number (0 if empty)
- 4-byte WAL head page number (0 if no WAL)
- 4-byte checksum
- 4-byte reserved for future use

#### Scenario: Valid database opened
- **WHEN** opening an existing database file
- **THEN** the header magic number SHALL be `TNYD`
- **AND** the version SHALL be supported (1)
- **AND** the checksum SHALL be valid

### Requirement: tinydb_master system table
The system table `tinydb_master` SHALL be created automatically and store schema metadata.

#### Scenario: tinydb_master created
- **WHEN** a new database is created
- **THEN** tinydb_master table SHALL be created automatically
- **AND** it SHALL have columns: type TEXT, name TEXT, tbl_name TEXT, sql TEXT

#### Scenario: Table added to tinydb_master
- **WHEN** `CREATE TABLE users (id INTEGER, name TEXT)` is executed
- **THEN** a row SHALL be inserted into tinydb_master with type='table', name='users', sql='CREATE TABLE users...'

#### Scenario: Index added to tinydb_master
- **WHEN** `CREATE INDEX idx ON users(name)` is executed
- **THEN** a row SHALL be inserted into tinydb_master with type='index', name='idx', tbl_name='users'

#### Scenario: Table dropped removes from tinydb_master
- **WHEN** `DROP TABLE users` is executed
- **THEN** the corresponding row SHALL be removed from tinydb_master

### Requirement: B+tree for table storage
Table data SHALL be stored in a B+tree structure where:
- Leaf pages contain row data keyed by rowid
- Internal pages contain pointers to child pages
- Rowid is the implicit primary key (auto-incrementing)
- Rowid is stable across UPDATE (never changes)

#### Scenario: Row inserted into table
- **WHEN** a row is inserted
- **THEN** a unique rowid SHALL be assigned
- **AND** the row SHALL be stored in the B+tree leaf page
- **AND** the rowid SHALL NOT change on UPDATE

### Requirement: B-tree for index storage
Indexes SHALL use B-tree structure where:
- Key is the indexed column value
- Value is the corresponding rowid(s)

#### Scenario: Index created
- **WHEN** `CREATE INDEX idx ON users(name)` is executed
- **THEN** a B-tree index SHALL be created on users(name)
- **AND** index entries SHALL map name values to rowids

### Requirement: Freelist management
Free pages SHALL be managed via a freelist:
- Freed pages are added to the freelist chain
- New allocations reuse freelist pages before extending file

#### Scenario: Row deleted
- **WHEN** a row is deleted
- **THEN** its page SHALL be added to the freelist
- **AND** subsequent inserts MAY reuse this page

### Requirement: Page allocation strategy
Page allocation SHALL follow this order:
1. Reuse pages from freelist
2. Append new pages to end of file

#### Scenario: Space reclaimed from freelist
- **WHEN** a new page is needed
- **AND** freelist is not empty
- **THEN** a freelist page SHALL be allocated
- **AND** the freelist head SHALL be updated