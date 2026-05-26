## 1. CLI Formatted Output Infrastructure

- [ ] 1.1 Add `cli_format_table()` function in `src/cli/cli.c` — calculates column widths and prints box borders
- [ ] 1.2 Add `cli_format_header()` — prints header row with column names and separator line
- [ ] 1.3 Add `cli_format_footer()` — prints row count and execution time
- [ ] 1.4 Add `cli_print_box_row()` — prints a single data row with proper column alignment
- [ ] 1.5 Integrate formatted output into `cli_execute_sql()` response parser

## 2. Execution Time Tracking

- [ ] 2.1 Add `gettimeofday()` timing in `cli_execute_sql()` — start before write, stop on first read
- [ ] 2.2 Store execution time and pass to result display functions
- [ ] 2.3 Format time as `(X.XX sec)` in footer output

## 3. Parser — SHOW TABLES

- [ ] 3.1 Add `AST_SHOW_TABLES` node type to `src/sql/ast.h`
- [ ] 3.2 Add `AstShowTables` struct definition to `src/sql/ast.h`
- [ ] 3.3 Add grammar rule for `SHOW TABLES` in `src/sql/parser.y` (or `parser.c`)
- [ ] 3.4 Add `parser_free_show_tables()` and `ast_print_show_tables()` if needed

## 4. Parser — DESC/DESCRIBE

- [ ] 4.1 Add `AST_DESCRIBE_TABLE` node type to `src/sql/ast.h`
- [ ] 4.2 Add `AstDescribeTable` struct with `table_name` field to `src/sql/ast.h`
- [ ] 4.3 Add grammar rule for `DESC <table>` and `DESCRIBE <table>` in parser
- [ ] 4.4 Add `parser_free_describe_table()` if needed

## 5. Executor — SHOW TABLES

- [ ] 5.1 Add `executor_exec_show_tables()` handler in `src/sql/executor.c`
- [ ] 5.2 Retrieve all table names from catalog via `catalog_get_all_tables()` or similar
- [ ] 5.3 Return result buffer with single `Tables_in_<dbname>` column and row per table
- [ ] 5.4 Wire `AST_SHOW_TABLES` in `executor_exec()` dispatch

## 6. Executor — DESC/DESCRIBE

- [ ] 6.1 Add `executor_exec_describe_table()` handler in `src/sql/executor.c`
- [ ] 6.2 Retrieve `CatalogEntry` for specified table from catalog
- [ ] 6.3 Extract column metadata (name, type, null, key, default) from catalog entry
- [ ] 6.4 Return result buffer with columns: Field, Type, Null, Key, Default, Extra
- [ ] 6.5 Wire `AST_DESCRIBE_TABLE` in `executor_exec()` dispatch

## 7. Catalog — Column Metadata

- [ ] 7.1 Ensure `catalog_lookup_type_name()` returns entry with column information
- [ ] 7.2 Ensure column type, nullability, key info, and default value are stored in catalog entry
- [ ] 7.3 Add `catalog_get_columns()` helper if not already present for retrieving column list

## 8. Testing

- [ ] 8.1 Test `SHOW TABLES` on empty database
- [ ] 8.2 Test `SHOW TABLES` with multiple tables
- [ ] 8.3 Test `DESC table` on existing table
- [ ] 8.4 Test `DESC` on non-existent table (error case)
- [ ] 8.5 Verify SELECT results display with box format and footer timing
- [ ] 8.6 Verify DML shows `Query OK, N row(s) affected`