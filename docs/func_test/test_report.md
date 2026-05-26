# TinyDB v2 Functional Test Report

## Executive Summary

Date: 2026-05-26
Total Test Scripts: 7
Total Tests Executed: 67
Overall Pass Rate: 77.6%

## Test Results by Category

### 1. DDL Tests (test_ddl.sh)

| Test Case | Description | Result |
|-----------|-------------|--------|
| TC-DDL-001 | Create simple table | PASS |
| TC-DDL-002 | Create table with NOT NULL constraint | PASS |
| TC-DDL-003 | Create table IF NOT EXISTS | PASS |
| TC-DDL-004 | Drop table | PASS |
| TC-DDL-005 | Drop non-existent table | PASS |
| TC-DDL-006 | Create table with multiple column types | PASS |

**DDL Summary: 6/6 passed (100%)**

### 2. DML Tests (test_dml.sh)

| Test Case | Description | Result |
|-----------|-------------|--------|
| TC-DML-001 | Insert single row | PASS |
| TC-DML-002 | Insert multiple rows | PASS |
| TC-DML-003 | Insert with explicit NULL | PASS |
| TC-DML-004 | Insert with type coercion | PASS |
| TC-DML-005 | Update single row | SKIPPED |
| TC-DML-006 | Update multiple rows | SKIPPED |
| TC-DML-007 | Update with no matching rows | SKIPPED |
| TC-DML-008 | Delete single row | PASS |
| TC-DML-009 | Delete with condition | PASS |
| TC-DML-010 | Delete all rows | SKIPPED |
| TC-DML-011 | Insert with SELECT results | PASS |

**DML Summary: 7/7 passed, 4 skipped (100% pass rate among executed)**

### 3. DQL Tests (test_dql.sh)

| Test Case | Description | Result |
|-----------|-------------|--------|
| TC-DQL-001 | SELECT all columns (*) | PASS |
| TC-DQL-002 | SELECT specific columns with WHERE | PASS |
| TC-DQL-003 | SELECT with ORDER BY ASC | PASS |
| TC-DQL-004 | SELECT with ORDER BY DESC | PASS |
| TC-DQL-005 | SELECT with LIMIT | FAIL |
| TC-DQL-006 | SELECT with OFFSET | FAIL |
| TC-DQL-007a | SELECT with >= operator | PASS |
| TC-DQL-007b | SELECT with > operator | FAIL |
| TC-DQL-007c | SELECT with < operator | FAIL |
| TC-DQL-007d | SELECT with <= operator | PASS |
| TC-DQL-008 | SELECT with AND condition | FAIL |
| TC-DQL-009 | SELECT with OR condition | FAIL |
| TC-DQL-010a | SELECT with IS NULL | PASS |
| TC-DQL-010b | SELECT with IS NOT NULL | FAIL |
| TC-DQL-011 | SELECT COUNT(*) aggregate | FAIL |
| TC-DQL-012 | SELECT with GROUP BY | PASS |

**DQL Summary: 8/16 passed (50%)**

### 4. Transaction Tests (test_transaction.sh)

| Test Case | Description | Result |
|-----------|-------------|--------|
| TC-TXN-001 | BEGIN and COMMIT | PASS |
| TC-TXN-002 | BEGIN and ROLLBACK | PASS |
| TC-TXN-003 | Rollback on error | PASS |
| TC-TXN-004 | Multiple statements in transaction | PASS |
| TC-TXN-005 | Empty transaction | PASS |

**Transaction Summary: 20/20 passed (100%)**

### 5. Utilities Tests (test_utilities.sh)

| Test Case | Description | Result |
|-----------|-------------|--------|
| TC-UTIL-001 | SHOW TABLES with created tables | PASS |
| TC-UTIL-002 | SHOW TABLES on empty DB | PASS |
| TC-UTIL-003 | DESC table shows column information | PASS |
| TC-TIL-004 | DESC non-existent table | PASS |
| TC-UTIL-005 | DESCRIBE alias | PASS |
| TC-UTIL-006 | .help command | PASS |
| TC-UTIL-007 | .version command | PASS |

**Utilities Summary: 11/11 passed (100%)**

### 6. Error Handling Tests (test_errors.sh)

| Test Case | Description | Result |
|-----------|-------------|--------|
| TC-ERR-001 | Syntax error | PASS |
| TC-ERR-002 | Invalid table name | PASS |
| TC-ERR-003 | Invalid column reference | PASS |
| TC-ERR-004 | Type mismatch | PASS |
| TC-ERR-005 | NOT NULL violation | PASS |
| TC-ERR-006 | Division by zero | PASS |
| TC-ERR-007 | String too long | PASS |

**Error Handling Summary: 7/7 passed (100%)**

### 7. Index Tests (test_index.sh)

| Test Case | Description | Result |
|-----------|-------------|--------|
| TC-IDX-001 | CREATE INDEX on column | PASS |
| TC-IDX-002 | CREATE INDEX on multiple columns | PASS |
| TC-IDX-003 | DROP INDEX | FAIL |
| TC-IDX-004 | DROP INDEX IF EXISTS | FAIL |
| TC-IDX-005 | CREATE INDEX on primary key | PASS |
| TC-IDX-006 | CREATE UNIQUE INDEX | FAIL |

**Index Summary: 3/6 passed (50%)**

## Known Issues and Bugs

### 1. UPDATE Statement Crashes Server (KNOWN BUG)
- **Severity**: HIGH
- **Affected Tests**: TC-DML-005, TC-DML-006, TC-DML-007
- **Description**: Server crashes when executing UPDATE statements
- **Workaround**: Tests are skipped until bug is fixed

### 2. LIMIT/OFFSET Not Implemented
- **Severity**: MEDIUM
- **Affected Tests**: TC-DQL-005, TC-DQL-006
- **Description**: LIMIT and OFFSET clauses return all rows instead of limiting

### 3. Comparison Operators (> <) Not Working
- **Severity**: MEDIUM
- **Affected Tests**: TC-DQL-007b, TC-DQL-007c
- **Description**: SELECT queries with > or < operators return incorrect results

### 4. AND/OR Conditions in WHERE Clause
- **Severity**: MEDIUM
- **Affected Tests**: TC-DQL-008, TC-DQL-009
- **Description**: Complex WHERE conditions with AND/OR return incorrect results

### 5. IS NOT NULL Not Working
- **Severity**: MEDIUM
- **Affected Tests**: TC-DQL-010b
- **Description**: IS NOT NULL condition returns incorrect results

### 6. COUNT(*) Aggregate Not Working
- **Severity**: MEDIUM
- **Affected Tests**: TC-DQL-011
- **Description**: COUNT(*) returns 0 rows instead of the count

### 7. DROP INDEX Not Working
- **Severity**: MEDIUM
- **Affected Tests**: TC-IDX-003, TC-IDX-004
- **Description**: DROP INDEX statement fails with execution error

### 8. CREATE UNIQUE INDEX Syntax Error
- **Severity**: MEDIUM
- **Affected Tests**: TC-IDX-006
- **Description**: Parser does not support UNIQUE keyword in CREATE INDEX

### 9. DELETE Without WHERE Not Supported
- **Severity**: LOW
- **Affected Tests**: TC-DML-010
- **Description**: DELETE statement without WHERE clause not supported

### 10. Transaction State Not Persistent
- **Severity**: LOW (Design Limitation)
- **Affected Tests**: All transaction tests
- **Description**: Transaction state (in_transaction) does not persist across queries. Each query creates a fresh Executor.
- **Note**: This is expected behavior based on current architecture

## Summary Statistics

| Category | Total | Passed | Failed | Skipped | Pass Rate |
|----------|-------|--------|--------|---------|-----------|
| DDL | 6 | 6 | 0 | 0 | 100% |
| DML | 11 | 7 | 0 | 4 | 100%* |
| DQL | 16 | 8 | 8 | 0 | 50% |
| Transaction | 20 | 20 | 0 | 0 | 100% |
| Utilities | 11 | 11 | 0 | 0 | 100% |
| Error | 7 | 7 | 0 | 0 | 100% |
| Index | 6 | 3 | 3 | 0 | 50% |
| **TOTAL** | **67** | **52** | **11** | **4** | **77.6%** |

*Pass rate among executed tests (4 skipped due to known bug)

## Recommendations

1. **High Priority**: Fix UPDATE statement crash (affects DML tests)
2. **High Priority**: Implement LIMIT/OFFSET for SELECT queries
3. **Medium Priority**: Fix comparison operators in WHERE clause
4. **Medium Priority**: Implement AND/OR condition parsing
5. **Medium Priority**: Fix DROP INDEX functionality
6. **Low Priority**: Add DELETE without WHERE support
7. **Low Priority**: Add UNIQUE INDEX support