#!/bin/bash
#
# test_ddl.sh - DDL Tests for CREATE TABLE and DROP TABLE
#
# Test cases:
# - TC-DDL-001: Create simple table
# - TC-DDL-002: Create table with NOT NULL
# - TC-DDL-003: Create table IF NOT EXISTS
# - TC-DDL-004: Drop table
# - TC-DDL-005: Drop non-existent table
# - TC-DDL-006: Create table with multiple column types (INTEGER, REAL, TEXT)
#

set -e

# Source the test harness
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/test_harness.sh"

# Start the server before running tests
start_server || {
    echo "ERROR: Failed to start server"
    exit 1
}

# Test counter
PASS_COUNT=0
FAIL_COUNT=0

# ============================================================================
# Test Cases
# ============================================================================

section "DDL Tests: CREATE TABLE and DROP TABLE"

# TC-DDL-001: Create simple table
echo ""
echo "TC-DDL-001: Create simple table"
send_sql "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL);" > /tmp/test_output.txt 2>&1
if assert_contains "OK: 1 row(s) affected" "$(cat /tmp/test_output.txt)" "TC-DDL-001"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DDL-002: Create table with NOT NULL constraint
echo ""
echo "TC-DDL-002: Create table with NOT NULL constraint"
send_sql "CREATE TABLE test_null (col INTEGER NOT NULL);" > /tmp/test_output.txt 2>&1
if assert_contains "OK: 1 row(s) affected" "$(cat /tmp/test_output.txt)" "TC-DDL-002"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DDL-003: Create table IF NOT EXISTS
echo ""
echo "TC-DDL-003: Create table IF NOT EXISTS"
send_sql "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY);" > /tmp/test_output.txt 2>&1
if assert_contains "OK: 1 row(s) affected" "$(cat /tmp/test_output.txt)" "TC-DDL-003"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DDL-004: Drop table
echo ""
echo "TC-DDL-004: Drop table"
send_sql "DROP TABLE users;" > /tmp/test_output.txt 2>&1
if assert_contains "OK: 1 row(s) affected" "$(cat /tmp/test_output.txt)" "TC-DDL-004"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DDL-005: Drop non-existent table (should return error)
echo ""
echo "TC-DDL-005: Drop non-existent table"
send_sql "DROP TABLE nonexistent;" > /tmp/test_output.txt 2>&1
if assert_contains "Error: execution failed" "$(cat /tmp/test_output.txt)" "TC-DDL-005"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DDL-006: Create table with multiple column types (INTEGER, REAL, TEXT)
echo ""
echo "TC-DDL-006: Create table with multiple column types"
send_sql "CREATE TABLE products (id INTEGER PRIMARY KEY, price REAL NOT NULL, description TEXT);" > /tmp/test_output.txt 2>&1
if assert_contains "OK: 1 row(s) affected" "$(cat /tmp/test_output.txt)" "TC-DDL-006"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Cleanup temp file
rm -f /tmp/test_output.txt

# Report results
report_results $PASS_COUNT $FAIL_COUNT
exit $?