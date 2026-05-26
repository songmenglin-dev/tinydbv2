#!/bin/bash
#
# test_harness.sh - Functional test harness for TinyDB v2
#
# This script provides utility functions for black-box functional tests.
# Tests communicate with the database server via CLI commands through a Unix socket.
#
# Usage: Source this file from test scripts:
#   source "$(dirname "$0")/test_harness.sh"
#
# Each test script should define TEST_NAME and call start_server before running tests.
#

# ============================================================================
# Configuration
# ============================================================================

# Use process ID to create unique paths for each test run
DB_PATH="/tmp/tinydb_test_$$"
SOCKET="${DB_PATH}.sock"

# Binary locations
SERVERBIN="./build/tinydb-server"
CLIBIN="./build/tinydb-cli"

# ============================================================================
# Global State
# ============================================================================

# Server process ID ( populated by start_server )
SERVER_PID=""

# Flag to track if server was started by this harness
SERVER_STARTED="false"

# ============================================================================
# Cleanup Function
# ============================================================================

#
# Cleanup function to kill server and remove temporary files.
# This is called automatically on script EXIT via trap.
#
cleanup() {
    # Kill the server if it's running and was started by this harness
    if [ -n "$SERVER_PID" ] && [ "$SERVER_STARTED" = "true" ]; then
        if kill -0 "$SERVER_PID" 2>/dev/null; then
            kill "$SERVER_PID" 2>/dev/null
            # Wait for server to terminate gracefully
            wait "$SERVER_PID" 2>/dev/null
        fi
    fi

    # Remove temporary database files
    if [ -d "$DB_PATH" ]; then
        rm -rf "$DB_PATH"
    fi

    # Remove socket file
    if [ -S "$SOCKET" ]; then
        rm -f "$SOCKET"
    fi
}

# ============================================================================
# Trap Setup
# ============================================================================

# Ensure cleanup runs on script exit
trap cleanup EXIT

# ============================================================================
# Server Management Functions
# ============================================================================

#
# Start the TinyDB server in background mode.
# Uses DB_PATH and SOCKET paths configured above.
#
# Arguments:
#   None
#
# Returns:
#   0 on success, 1 on failure
#
start_server() {
    # Check that binaries exist
    if [ ! -x "$SERVERBIN" ]; then
        echo "ERROR: Server binary not found at $SERVERBIN" >&2
        return 1
    fi

    if [ ! -x "$CLIBIN" ]; then
        echo "ERROR: CLI binary not found at $CLIBIN" >&2
        return 1
    fi

    # Remove any stale files from previous runs
    cleanup 2>/dev/null

    # Start the server in background
    "$SERVERBIN" --db-path "$DB_PATH" --socket "$SOCKET" &
    SERVER_PID=$!

    # Wait briefly for server to start and create socket
    local max_attempts=30
    local attempt=0
    while [ ! -S "$SOCKET" ] && [ $attempt -lt $max_attempts ]; do
        sleep 0.1
        attempt=$((attempt + 1))
    done

    # Check if server started successfully
    if [ ! -S "$SOCKET" ]; then
        echo "ERROR: Server failed to create socket at $SOCKET" >&2
        if kill -0 "$SERVER_PID" 2>/dev/null; then
            kill "$SERVER_PID" 2>/dev/null
        fi
        SERVER_PID=""
        return 1
    fi

    # Verify server process is still running
    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "ERROR: Server process terminated unexpectedly" >&2
        SERVER_PID=""
        return 1
    fi

    SERVER_STARTED="true"
    return 0
}

# ============================================================================
# CLI Communication Functions
# ============================================================================

#
# Send SQL command to server via CLI and return output.
#
# Arguments:
#   $1 - SQL command to execute
#
# Returns:
#   Outputs the command result to stdout
#
send_sql() {
    local sql_cmd="$1"

    if [ -z "$sql_cmd" ]; then
        echo "ERROR: send_sql requires a SQL command" >&2
        return 1
    fi

    if [ ! -S "$SOCKET" ]; then
        echo "ERROR: Socket does not exist at $SOCKET - is server running?" >&2
        return 1
    fi

    # Send SQL via CLI to server
    "$CLIBIN" --socket "$SOCKET" -c "$sql_cmd"
}

#
# Send SQL command and capture both stdout and stderr.
#
# Arguments:
#   $1 - SQL command to execute
#
# Returns:
#   Outputs the command result to stdout
#
send_sql_stderr() {
    local sql_cmd="$1"

    if [ -z "$sql_cmd" ]; then
        echo "ERROR: send_sql_stderr requires a SQL command" >&2
        return 1
    fi

    if [ ! -S "$SOCKET" ]; then
        echo "ERROR: Socket does not exist at $SOCKET - is server running?" >&2
        return 1
    fi

    # Send SQL via CLI to server, capturing stderr
    "$CLIBIN" --socket "$SOCKET" -c "$sql_cmd" 2>&1
}

# ============================================================================
# Assertion Functions
# ============================================================================

#
# Assert that command output contains expected string.
#
# Arguments:
#   $1 - The expected substring
#   $2 - The actual output (via stdin or as second arg)
#   $3 - Optional: test name for error message
#
# Returns:
#   0 if contains, 1 if not
#
assert_contains() {
    local expected="$1"
    local actual=""
    local test_name="assert_contains"

    # Handle different argument configurations
    if [ $# -eq 3 ]; then
        # Arguments: expected actual test_name
        actual="$2"
        test_name="$3"
    elif [ $# -eq 2 ]; then
        # Arguments: expected actual (no test name)
        actual="$2"
    else
        # Read from stdin if no arguments
        actual=$(cat)
    fi

    if [ -z "$expected" ]; then
        echo "ERROR: assert_contains requires an expected string" >&2
        return 1
    fi

    if [ -z "$actual" ]; then
        echo "FAIL [$test_name]: Expected to find '$expected' but output was empty"
        return 1
    fi

    if echo "$actual" | grep -q "$expected"; then
        echo "PASS [$test_name]: Found expected '$expected'"
        return 0
    else
        echo "FAIL [$test_name]: Expected to find '$expected'"
        echo "Actual output:"
        echo "$actual"
        return 1
    fi
}

#
# Assert that command output does not contain a string.
#
# Arguments:
#   $1 - The unexpected substring
#   $2 - The actual output (via stdin or as second arg)
#   $3 - Optional: test name for error message
#
# Returns:
#   0 if not found, 1 if found
#
assert_not_contains() {
    local unexpected="$1"
    local actual=""
    local test_name="assert_not_contains"

    if [ $# -eq 3 ]; then
        actual="$2"
        test_name="$3"
    elif [ $# -eq 2 ]; then
        actual="$2"
    else
        actual=$(cat)
    fi

    if [ -z "$unexpected" ]; then
        echo "ERROR: assert_not_contains requires an unexpected string" >&2
        return 1
    fi

    if [ -z "$actual" ]; then
        echo "PASS [$test_name]: Output is empty (does not contain '$unexpected')"
        return 0
    fi

    if echo "$actual" | grep -q "$unexpected"; then
        echo "FAIL [$test_name]: Expected NOT to find '$unexpected'"
        echo "Actual output:"
        echo "$actual"
        return 1
    else
        echo "PASS [$test_name]: Did not find '$unexpected'"
        return 0
    fi
}

#
# Assert that the count of occurrences matches expected count.
#
# Arguments:
#   $1 - The substring to count
#   $2 - The actual output
#   $3 - The expected count
#   $4 - Optional: test name for error message
#
# Returns:
#   0 if count matches, 1 if not
#
assert_count() {
    local substring="$1"
    local actual=""
    local expected_count=""
    local test_name="assert_count"

    if [ $# -eq 4 ]; then
        substring="$1"
        actual="$2"
        expected_count="$3"
        test_name="$4"
    elif [ $# -eq 3 ]; then
        substring="$1"
        actual="$2"
        expected_count="$3"
    else
        echo "ERROR: assert_count requires at least 3 arguments" >&2
        return 1
    fi

    if [ -z "$substring" ]; then
        echo "ERROR: assert_count requires a substring" >&2
        return 1
    fi

    if [ -z "$expected_count" ]; then
        echo "ERROR: assert_count requires an expected count" >&2
        return 1
    fi

    # Count occurrences
    local actual_count=$(echo "$actual" | grep -o "$substring" | wc -l)

    if [ "$actual_count" -eq "$expected_count" ]; then
        echo "PASS [$test_name]: Found $expected_count occurrence(s) of '$substring'"
        return 0
    else
        echo "FAIL [$test_name]: Expected $expected_count occurrence(s) of '$substring', found $actual_count"
        echo "Actual output:"
        echo "$actual"
        return 1
    fi
}

# ============================================================================
# Utility Functions
# ============================================================================

#
# Print a test section header.
#
# Arguments:
#   $1 - Section title
#
section() {
    echo ""
    echo "========================================"
    echo "$1"
    echo "========================================"
}

#
# Report test results summary.
#
# Arguments:
#   $1 - Pass count
#   $2 - Fail count
#
report_results() {
    local pass_count="${1:-0}"
    local fail_count="${2:-0}"
    local total=$((pass_count + fail_count))

    echo ""
    echo "========================================"
    echo "Test Results: $total total"
    echo "  PASS: $pass_count"
    echo "  FAIL: $fail_count"
    echo "========================================"

    if [ "$fail_count" -gt 0 ]; then
        return 1
    fi
    return 0
}