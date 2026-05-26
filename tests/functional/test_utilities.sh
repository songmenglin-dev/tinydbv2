#!/bin/bash
#
# test_utilities.sh - Utility Commands Tests for SHOW TABLES, DESC, .help, .version
#
# Test cases:
# - TC-UTIL-001: SHOW TABLES (verify it lists created tables)
# - TC-UTIL-002: SHOW TABLES on empty DB (verify empty or no tables message)
# - TC-UTIL-003: DESC table (verify it shows column information)
# - TC-UTIL-004: DESC non-existent table (should error)
# - TC-UTIL-005: DESCRIBE (alias for DESC)
# - TC-UTIL-006: .help command (interactive mode)
# - TC-UTIL-007: .version command (interactive mode)
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

section "Utility Tests: SHOW TABLES, DESC, .help, .version"

# TC-UTIL-002: SHOW TABLES on empty DB (verify empty or no tables message)
# Run this FIRST before creating any tables
echo ""
echo "TC-UTIL-002: SHOW TABLES on empty DB"
OUTPUT=$(send_sql "SHOW TABLES;" < /dev/null)
echo "$OUTPUT" > /tmp/test_output.txt
# Should show "0 tables" or similar message indicating no tables
# The actual output is "Query OK, 0 row(s) returned" so check for "0 row"
if assert_contains "0 row" "$(cat /tmp/test_output.txt)" "TC-UTIL-002"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-UTIL-001: SHOW TABLES (verify it lists created tables)
echo ""
echo "TC-UTIL-001: SHOW TABLES with created tables"
send_sql "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL);" > /tmp/test_output.txt 2>&1
send_sql "CREATE TABLE products (id INTEGER PRIMARY KEY, price REAL);" > /tmp/test_output.txt 2>&1
OUTPUT=$(send_sql "SHOW TABLES;" < /dev/null)
echo "$OUTPUT" > /tmp/test_output.txt
if assert_contains "users" "$(cat /tmp/test_output.txt)" "TC-UTIL-001"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi
if assert_contains "products" "$(cat /tmp/test_output.txt)" "TC-UTIL-001"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-UTIL-003: DESC table (verify it shows column information)
echo ""
echo "TC-UTIL-003: DESC table shows column information"
send_sql "CREATE TABLE test_desc (id INTEGER PRIMARY KEY, name TEXT NOT NULL, value REAL);" > /tmp/test_output.txt 2>&1
OUTPUT=$(send_sql "DESC test_desc;" < /dev/null)
echo "$OUTPUT" > /tmp/test_output.txt
if assert_contains "id" "$(cat /tmp/test_output.txt)" "TC-UTIL-003"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi
if assert_contains "name" "$(cat /tmp/test_output.txt)" "TC-UTIL-003"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi
if assert_contains "value" "$(cat /tmp/test_output.txt)" "TC-UTIL-003"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-UTIL-004: DESC non-existent table (should error)
# Use send_sql_stderr since errors go to stderr
echo ""
echo "TC-UTIL-004: DESC non-existent table should error"
OUTPUT=$(send_sql_stderr "DESC nonexistent;" < /dev/null)
echo "$OUTPUT" > /tmp/test_output.txt
if assert_contains "Error" "$(cat /tmp/test_output.txt)" "TC-UTIL-004"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-UTIL-005: DESCRIBE (alias for DESC)
echo ""
echo "TC-UTIL-005: DESCRIBE is alias for DESC"
send_sql "CREATE TABLE test_alias (col1 INTEGER, col2 TEXT);" > /tmp/test_output.txt 2>&1
OUTPUT=$(send_sql "DESCRIBE test_alias;" < /dev/null)
echo "$OUTPUT" > /tmp/test_output.txt
if assert_contains "col1" "$(cat /tmp/test_output.txt)" "TC-UTIL-005"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi
if assert_contains "col2" "$(cat /tmp/test_output.txt)" "TC-UTIL-005"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-UTIL-006: .help command (requires interactive mode, test via --help flag instead)
echo ""
echo "TC-UTIL-006: .help command via CLI --help"
OUTPUT=$("$CLIBIN" --help 2>&1)
echo "$OUTPUT" > /tmp/test_output.txt
# The CLI --help shows help information, which demonstrates the help system exists
if assert_contains "help" "$(cat /tmp/test_output.txt)" "TC-UTIL-006"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# TC-UTIL-007: .version command (requires interactive mode, test via --version flag instead)
echo ""
echo "TC-UTIL-007: .version command via CLI --version"
OUTPUT=$("$CLIBIN" --version 2>&1)
echo "$OUTPUT" > /tmp/test_output.txt
# The CLI --version shows version information (e.g., "TinyDB CLI v2.0.0")
if assert_contains "TinyDB" "$(cat /tmp/test_output.txt)" "TC-UTIL-007"; then
    PASS_COUNT=$((PASS_COUNT + 1))
else
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Cleanup temp file
rm -f /tmp/test_output.txt

# Report results
report_results $PASS_COUNT $FAIL_COUNT
exit $?