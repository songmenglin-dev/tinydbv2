# Build System

## Build Tool: Make

All builds use GNU Make. No configure scripts, no CMake, no autotools.

## Directory Layout

```
tinydbv2/
├── Makefile                 # Top-level targets
├── src/
│   ├── Makefile            # Source compilation
│   ├── cli/
│   │   ├── Makefile        # CLI-specific
│   │   └── *.c
│   ├── server/
│   │   ├── Makefile
│   │   └── *.c
│   ├── sql/
│   │   ├── Makefile
│   │   └── *.c
│   └── storage/
│       ├── Makefile
│       └── *.c
├── tests/
│   ├── Makefile            # Test runner
│   ├── unit/
│   │   ├── Makefile
│   │   ├── parser_test.c
│   │   ├── executor_test.c
│   │   ├── btree_test.c
│   │   ├── wal_test.c
│   │   └── storage_test.c
│   └── integration/
│       ├── Makefile
│       ├── cli_test.sh
│       └── sql_test.sh
├── docs/
│   ├── Makefile            # Doc generation (optional)
│   └── design/
└── build/                  # Generated during build
    ├── cli/
    ├── server/
    ├── tests/
    └── lib/
```

## Top-Level Makefile

```makefile
# Project root Makefile

CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -I$(SRC_DIR) -I$(INC_DIR)
LDFLAGS = -lm

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
TEST_DIR = tests
PREFIX = /usr/local

# Source file lists
CLI_SRC = $(SRC_DIR)/cli/main.c $(SRC_DIR)/cli/cli.c $(SRC_DIR)/cli/commands.c \
          $(SRC_DIR)/util/error.c $(SRC_DIR)/util/string.c

SERVER_SRC = $(SRC_DIR)/server/main.c $(SRC_DIR)/server/protocol.c \
             $(SRC_DIR)/sql/parser.c $(SRC_DIR)/sql/scanner.c \
             $(SRC_DIR)/sql/analyzer.c $(SRC_DIR)/sql/executor.c \
             $(SRC_DIR)/sql/expression.c $(SRC_DIR)/sql/table.c \
             $(SRC_DIR)/storage/page.c $(SRC_DIR)/storage/btree.c \
             $(SRC_DIR)/storage/btree_insert.c $(SRC_DIR)/storage/btree_delete.c \
             $(SRC_DIR)/storage/wal.c $(SRC_DIR)/storage/arena.c \
             $(SRC_DIR)/storage/file.c $(SRC_DIR)/util/error.c $(SRC_DIR)/util/string.c

# Object file conversion
CLI_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(CLI_SRC))
SERVER_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SERVER_SRC))

# Targets
.PHONY: all clean install test unit-test integration-test valgrind gcov

all: $(BUILD_DIR)/tinydb-cli $(BUILD_DIR)/tinydb-server

# Build CLI client
$(BUILD_DIR)/tinydb-cli: $(CLI_OBJS)
    @mkdir -p $(dir $@)
    $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build server daemon
$(BUILD_DIR)/tinydb-server: $(SERVER_OBJS)
    @mkdir -p $(dir $@)
    $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Object file compilation rule
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
    @mkdir -p $(dir $@)
    $(CC) $(CFLAGS) -c -o $@ $<

# Install
install: all
    install -d $(PREFIX)/bin
    install -m 0755 $(BUILD_DIR)/tinydb-cli $(PREFIX)/bin/
    install -m 0755 $(BUILD_DIR)/tinydb-server $(PREFIX)/bin/

# Test targets
unit-test: $(BUILD_DIR)/test-suite
    $(BUILD_DIR)/test-suite --unit

integration-test: $(BUILD_DIR)/test-suite
    $(BUILD_DIR)/test-suite --integration

valgrind: $(BUILD_DIR)/tinydb-cli
    valgrind --leak-check=full $(BUILD_DIR)/tinydb-cli

# Coverage
gcov:
    gcov -r $(SRC_DIR)/*.c

clean:
    rm -rf $(BUILD_DIR)
```

## Source Compilation

```makefile
# src/Makefile

CLI_SRC = cli/main.c cli/cli.c cli/commands.c
SERVER_SRC = server/main.c server/protocol.c
SQL_SRC = sql/parser.c sql/scanner.c sql/analyzer.c \
          sql/executor.c sql/expression.c sql/table.c
STORAGE_SRC = storage/page.c storage/btree.c storage/btree_insert.c \
              storage/btree_delete.c storage/wal.c storage/arena.c \
              storage/file.c

COMMON_SRC = util/error.c util/string.c

CLI_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(CLI_SRC))
SERVER_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SERVER_SRC))
SQL_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SQL_SRC))
STORAGE_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(STORAGE_SRC))
COMMON_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(COMMON_SRC))

$(BUILD_DIR)/%.o: %.c
    @mkdir -p $(dir $@)
    $(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/tinydb-cli: $(CLI_OBJS) $(COMMON_OBJS)
    $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/tinydb-server: $(SERVER_OBJS) $(SQL_OBJS) $(STORAGE_OBJS) $(COMMON_OBJS)
    $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lm
```

## Test Infrastructure

### Test Framework: Custom Minimal

No external test framework. Tests use a minimal custom harness:

```c
// tests/unit/mini_test.h
#define test(name) void test_##name(void)
#define run(name) do { \
    printf("  %s... ", #name); \
    fflush(stdout); \
    test_##name(); \
    printf("OK\n"); \
} while(0)

#define assert_eq(a, b) do { \
    if ((a) != (b)) { \
        printf("FAIL: %s != %s (%ld != %ld)\n", #a, #b, \
               (long)(a), (long)(b)); \
        exit(1); \
    } \
} while(0)

#define assert_null(p) do { \
    if ((p) != NULL) { \
        printf("FAIL: %s should be NULL\n", #p); \
        exit(1); \
    } \
} while(0)

#define assert_non_null(p) do { \
    if ((p) == NULL) { \
        printf("FAIL: %s should not be NULL\n", #p); \
        exit(1); \
    } \
} while(0)

#define assert_str_eq(a, b) do { \
    if (strcmp((a), (b)) != 0) { \
        printf("FAIL: \"%s\" != \"%s\"\n", (a), (b)); \
        exit(1); \
    } \
} while(0)
```

### Test Suite Runner

```c
// tests/unit/test-suite.c
int main(int argc, char** argv) {
    int run_unit = 0, run_integration = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--unit") == 0) run_unit = 1;
        if (strcmp(argv[i], "--integration") == 0) run_integration = 1;
    }
    
    printf("TinyDB v2 Test Suite\n");
    printf("====================\n\n");
    
    if (run_unit) {
        printf("Unit Tests:\n");
        run(parser_basic);
        run(parser_select);
        run(parser_insert);
        run(executor_simple);
        run(btree_insert_find);
        run(btree_delete);
        run(wal_write);
        run(storage_txn);
    }
    
    printf("\n%d tests, %d passed, %d failed\n",
           total, passed, failed);
    return failed > 0 ? 1 : 0;
}
```

## Coverage Report

```makefile
gcov: CFLAGS += -fprofile-arcs -ftest-coverage -g
gcov: LDFLAGS += -fprofile-arcs -ftest-coverage

coverage: gcov
    @mkdir -p coverage
    gcov -r $(SRC_DIR)/*.c -o coverage/
    @echo "Coverage report in coverage/*.gcov"
```

## Valgrind Suppressions

```text
# tests/valgrind.supp
{
   <insert_a suppression_name_here>
   Memcheck:Leak
   ...
   fun:malloc
}
```

## Continuous Integration

```yaml
# .github/workflows/ci.yml
name: CI
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Build
        run: make all
      - name: Unit tests
        run: make unit-test
      - name: Integration tests
        run: make integration-test
      - name: Valgrind
        run: make valgrind
      - name: Coverage
        run: make coverage
```

## Compiler Flags Summary

| Flag | Purpose |
|------|---------|
| `-Wall -Wextra` | Enable all warnings |
| `-Werror` | Treat warnings as errors |
| `-pedantic` | Strict ISO C11 compliance |
| `-std=c11` | C11 standard |
| `-fprofile-arcs -ftest-coverage` | Code coverage |
| `-g` | Debug symbols |
| `-O2` | Production optimization |

## Build Artifacts

| Binary | Purpose |
|--------|---------|
| `build/tinydb-cli` | CLI client binary |
| `build/tinydb-server` | Server daemon binary |
| `build/test-suite` | Test runner |
| `build/*.o` | Object files (intermediate) |
