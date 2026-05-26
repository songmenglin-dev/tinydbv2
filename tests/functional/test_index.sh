#!/bin/bash
#
# test_index.sh - Index Tests for CREATE INDEX and DROP INDEX
#
# Test cases:
# - TC-IDX-001: CREATE INDEX on table column
# - TC-IDX-002: CREATE INDEX on multiple columns (composite index)
# - TC-IDX-003: DROP INDEX
# - TC-IDX-004: DROP INDEX IF EXISTS (should succeed even if doesn't exist)
# - TC-IDX-005: CREATE INDEX on primary key column (implicit index)
# - TC-IDX-006: CREATE UNIQUE INDEX (if supported)
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

section "Index Tests: CREATE INDEX and DROP INDEX"

# TC-IDX-001: CREATE INDEX on table column
echo ""
echo "TC-IDX-001: CREATE INDEX on table column"
send_sql "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL);" > /tmp/test_output.txt 2>&1
send_sql "CREATE INDEX idx_users_name ON users (name);" > /tmp/test_output.txt 2>&1
if assert_contains "OK" "$(cat /tmp/test_output.txt)" "TC-IDX-001"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-IDX-002: CREATE INDEX on multiple columns (composite index)
echo ""
echo "TC-IDX-002: CREATE INDEX on multiple columns (composite index)"
send_sql "CREATE TABLE orders (id INTEGER PRIMARY KEY, user_id INTEGER NOT NULL, product_id INTEGER NOT NULL, amount REAL);" > /tmp/test_output.txt 2>&1
send_sql "CREATE INDEX idx_orders_user_product ON orders (user_id, product_id);" > /tmp/test_output.txt 2>&1
if assert_contains "OK" "$(cat /tmp/test_output.txt)" "TC-IDX-002"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-IDX-003: DROP INDEX
echo ""
echo "TC-IDX-003: DROP INDEX"
send_sql "DROP INDEX idx_users_name;" > /tmp/test_output.txt 2>&1
if assert_contains "OK" "$(cat /tmp/test_output.txt)" "TC-IDX-003"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-IDX-004: DROP INDEX IF EXISTS (should succeed even if doesn't exist)
echo ""
echo "TC-IDX-004: DROP INDEX IF EXISTS"
send_sql "DROP INDEX IF EXISTS idx_nonexistent;" > /tmp/test_output.txt 2>&1
if assert_contains "OK" "$(cat /tmp/test_output.txt)" "TC-IDX-004"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-IDX-005: CREATE INDEX on primary key column (explicit index on primary key)
echo ""
echo "TC-IDX-005: CREATE INDEX on primary key column"
send_sql "CREATE TABLE products (id INTEGER PRIMARY KEY, name TEXT NOT NULL);" > /tmp/test_output.txt 2>&1
send_sql "CREATE INDEX idx_products_id ON products (id);" > /tmp/test_output.txt 2>&1
if assert_contains "OK" "$(cat /tmp/test_output.txt)" "TC-IDX-005"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-IDX-006: CREATE UNIQUE INDEX (if supported)
echo ""
echo "TC-IDX-006: CREATE UNIQUE INDEX"
send_sql "CREATE TABLE accounts (id INTEGER PRIMARY KEY, email TEXT NOT NULL);" > /tmp/test_output.txt 2>&1
send_sql "CREATE UNIQUE INDEX idx_accounts_email ON accounts (email);" > /tmp/test_output.txt 2>&1
if assert_contains "OK" "$(cat /tmp/test_output.txt)" "TC-IDX-006"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Cleanup temp file
rm -f /tmp/test_output.txt

# Report results
report_results $PASS_COUNT $FAIL_COUNT
exit $?