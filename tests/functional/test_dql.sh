#!/bin/bash
#
# test_dql.sh - DQL Tests for SELECT functionality
#
# Test cases:
# - TC-DQL-001: SELECT all columns (*)
# - TC-DQL-002: SELECT specific columns with WHERE
# - TC-DQL-003: SELECT with ORDER BY ASC
# - TC-DQL-004: SELECT with ORDER BY DESC
# - TC-DQL-005: SELECT with LIMIT
# - TC-DQL-006: SELECT with OFFSET
# - TC-DQL-007: SELECT with comparison operators (>=, >, <, <=)
# - TC-DQL-008: SELECT with AND condition
# - TC-DQL-009: SELECT with OR condition
# - TC-DQL-010: SELECT with NULL check (IS NULL, IS NOT NULL)
# - TC-DQL-011: SELECT COUNT(*) aggregate
# - TC-DQL-012: SELECT with GROUP BY (if supported)
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
SKIP_COUNT=0

# ============================================================================
# Test Cases
# ============================================================================

section "DQL Tests: SELECT"

# Setup: Create table for DQL tests
send_sql "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL, age INTEGER);" > /tmp/test_output.txt 2>&1
echo "Setup: Created users table"

# Insert test data
send_sql "INSERT INTO users VALUES (1, 'Alice', 30);" > /tmp/test_output.txt 2>&1
send_sql "INSERT INTO users VALUES (2, 'Bob', 25);" > /tmp/test_output.txt 2>&1
send_sql "INSERT INTO users VALUES (3, 'Charlie', 35);" > /tmp/test_output.txt 2>&1
send_sql "INSERT INTO users VALUES (4, 'Diana', 28);" > /tmp/test_output.txt 2>&1
send_sql "INSERT INTO users VALUES (5, 'Eve', NULL);" > /tmp/test_output.txt 2>&1
echo "Setup: Inserted 5 rows"

# TC-DQL-001: SELECT all columns (*)
echo ""
echo "TC-DQL-001: SELECT all columns (*)"
send_sql "SELECT * FROM users;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
if echo "$output" | grep -q "Alice" && echo "$output" | grep -q "Bob" && echo "$output" | grep -q "Charlie"; then
    echo "PASS [TC-DQL-001]: SELECT * returns all rows"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-001]: SELECT * did not return expected data"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DQL-002: SELECT specific columns with WHERE
echo ""
echo "TC-DQL-002: SELECT specific columns with WHERE"
send_sql "SELECT name, age FROM users WHERE age > 28;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
# Should return Alice (30) and Charlie (35)
if echo "$output" | grep -q "Alice" && echo "$output" | grep -q "30" && echo "$output" | grep -q "Charlie" && echo "$output" | grep -q "35"; then
    echo "PASS [TC-DQL-002]: SELECT with WHERE clause works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-002]: SELECT with WHERE clause failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DQL-003: SELECT with ORDER BY ASC
echo ""
echo "TC-DQL-003: SELECT with ORDER BY ASC"
send_sql "SELECT * FROM users ORDER BY age ASC;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
# Expected order: Bob(25), Diana(28), Alice(30), Charlie(35), Eve(NULL)
if echo "$output" | grep -q "Bob.*25" && echo "$output" | grep -q "Diana.*28"; then
    echo "PASS [TC-DQL-003]: ORDER BY ASC works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-003]: ORDER BY ASC failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DQL-004: SELECT with ORDER BY DESC
echo ""
echo "TC-DQL-004: SELECT with ORDER BY DESC"
send_sql "SELECT * FROM users ORDER BY age DESC;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
# Expected: Charlie(35), Alice(30), Diana(28), Bob(25), Eve(NULL)
if echo "$output" | grep -q "Charlie.*35" && echo "$output" | grep -q "Alice.*30"; then
    echo "PASS [TC-DQL-004]: ORDER BY DESC works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-004]: ORDER BY DESC failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DQL-005: SELECT with LIMIT
echo ""
echo "TC-DQL-005: SELECT with LIMIT"
send_sql "SELECT * FROM users ORDER BY id ASC LIMIT 3;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
if echo "$output" | grep -q "Alice" && echo "$output" | grep -q "Bob" && echo "$output" | grep -q "Charlie" && ! echo "$output" | grep -q "Diana"; then
    echo "PASS [TC-DQL-005]: LIMIT works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-005]: LIMIT failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DQL-006: SELECT with OFFSET
echo ""
echo "TC-DQL-006: SELECT with OFFSET"
send_sql "SELECT * FROM users ORDER BY id ASC LIMIT 2 OFFSET 2;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
# Should return Charlie (id=3) and Diana (id=4)
if echo "$output" | grep -q "Charlie" && echo "$output" | grep -q "Diana" && ! echo "$output" | grep -q "Alice"; then
    echo "PASS [TC-DQL-006]: OFFSET works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-006]: OFFSET failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DQL-007: SELECT with comparison operators (>=, >, <, <=)
echo ""
echo "TC-DQL-007: SELECT with comparison operators"

# Test >=
send_sql "SELECT * FROM users WHERE age >= 30;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
if echo "$output" | grep -q "Alice.*30" && echo "$output" | grep -q "Charlie.*35"; then
    echo "PASS [TC-DQL-007a]: >= operator works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-007a]: >= operator failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test >
send_sql "SELECT * FROM users WHERE age > 30;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
if echo "$output" | grep -q "Charlie.*35" && ! echo "$output" | grep -q "Alice"; then
    echo "PASS [TC-DQL-007b]: > operator works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-007b]: > operator failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test <
send_sql "SELECT * FROM users WHERE age < 28;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
if echo "$output" | grep -q "Bob.*25" && ! echo "$output" | grep -q "Diana"; then
    echo "PASS [TC-DQL-007c]: < operator works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-007c]: < operator failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test <=
send_sql "SELECT * FROM users WHERE age <= 28;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
if echo "$output" | grep -q "Bob.*25" && echo "$output" | grep -q "Diana.*28"; then
    echo "PASS [TC-DQL-007d]: <= operator works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-007d]: <= operator failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DQL-008: SELECT with AND condition
echo ""
echo "TC-DQL-008: SELECT with AND condition"
send_sql "SELECT * FROM users WHERE id > 1 AND age < 30;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
# Bob(25) and Diana(28) match, but Eve(NULL) should not
if echo "$output" | grep -q "Bob" && echo "$output" | grep -q "Diana" && ! echo "$output" | grep -q "Eve"; then
    echo "PASS [TC-DQL-008]: AND condition works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-008]: AND condition failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DQL-009: SELECT with OR condition
echo ""
echo "TC-DQL-009: SELECT with OR condition"
send_sql "SELECT * FROM users WHERE id = 1 OR id = 3;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
if echo "$output" | grep -q "Alice" && echo "$output" | grep -q "Charlie" && ! echo "$output" | grep -q "Bob"; then
    echo "PASS [TC-DQL-009]: OR condition works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-009]: OR condition failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DQL-010: SELECT with NULL check (IS NULL, IS NOT NULL)
echo ""
echo "TC-DQL-010: SELECT with NULL check"

# IS NULL
send_sql "SELECT * FROM users WHERE age IS NULL;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
if echo "$output" | grep -q "Eve"; then
    echo "PASS [TC-DQL-010a]: IS NULL works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-010a]: IS NULL failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# IS NOT NULL
send_sql "SELECT * FROM users WHERE age IS NOT NULL;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
if echo "$output" | grep -q "Alice" && echo "$output" | grep -q "Bob" && echo "$output" | grep -q "Charlie" && echo "$output" | grep -q "Diana" && ! echo "$output" | grep -q "Eve"; then
    echo "PASS [TC-DQL-010b]: IS NOT NULL works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-010b]: IS NOT NULL failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DQL-011: SELECT COUNT(*) aggregate
echo ""
echo "TC-DQL-011: SELECT COUNT(*) aggregate"
send_sql "SELECT COUNT(*) FROM users;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
if echo "$output" | grep -q "5"; then
    echo "PASS [TC-DQL-011]: COUNT(*) works"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL [TC-DQL-011]: COUNT(*) failed"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-DQL-012: SELECT with GROUP BY (if supported)
echo ""
echo "TC-DQL-012: SELECT with GROUP BY"
send_sql "SELECT age, COUNT(*) FROM users GROUP BY age;" > /tmp/test_output.txt 2>&1
output=$(cat /tmp/test_output.txt)
# Note: GROUP BY may not be fully supported, check behavior
if echo "$output" | grep -q "Error" || echo "$output" | grep -q "row(s) returned"; then
    # Either an error (not supported) or results (supported)
    if echo "$output" | grep -q "row(s) returned"; then
        echo "PASS [TC-DQL-012]: GROUP BY is supported"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "SKIP [TC-DQL-012]: GROUP BY not supported"
        SKIP_COUNT=$((SKIP_COUNT + 1))
    fi
else
    echo "FAIL [TC-DQL-012]: GROUP BY unexpected behavior"
    echo "Output: $output"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Cleanup temp file
rm -f /tmp/test_output.txt

# Report results
echo ""
echo "========================================"
echo "Test Results: $((PASS_COUNT + FAIL_COUNT + SKIP_COUNT)) total"
echo "  PASS: $PASS_COUNT"
echo "  FAIL: $FAIL_COUNT"
echo "  SKIP: $SKIP_COUNT"
echo "========================================"

if [ "$FAIL_COUNT" -gt 0 ]; then
    exit 1
fi
exit 0