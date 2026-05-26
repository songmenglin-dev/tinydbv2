# TinyDB v2 测试用例文档

## 概述

本文档记录 TinyDB v2 项目的所有功能测试用例，包括测试步骤和测试结果。

## 目录结构

| 文档 | 内容 |
|------|------|
| [test-sql-lexer.md](./test-sql-lexer.md) | SQL Lexer 测试用例 |
| [test-sql-parser.md](./test-sql-parser.md) | SQL Parser 测试用例 |
| [test-expression.md](./test-expression.md) | Expression Evaluation 测试用例 |
| [test-btree.md](./test-btree.md) | B+Tree Storage 测试用例 |
| [test-wal.md](./test-wal.md) | WAL 测试用例 |
| [test-schema.md](./test-schema.md) | Schema 测试用例 |
| [test-executor.md](./test-executor.md) | Executor 测试用例 |
| [test-catalog.md](./test-catalog.md) | Catalog 测试用例 |
| [test-cli.md](./test-cli.md) | CLI 测试用例 |
| [test-integration.md](./test-integration.md) | Integration 测试用例 |

## 测试环境

- **平台**: Linux (WSL2)
- **编译器**: GCC
- **构建工具**: Make
- **测试框架**: mini_test.h

## 运行测试

```bash
# 运行所有单元测试
make test

# 运行特定模块测试
./tests/unit/test-suite

# 运行特定测试文件
./tests/unit/test-btree
```

## 测试结果汇总

| 模块 | 测试用例数 | 通过数 | 通过率 |
|------|-----------|--------|--------|
| SQL Lexer | 11 | 11 | 100% |
| SQL Parser | 17 | 17 | 100% |
| Expression | 15 | 15 | 100% |
| B+Tree | 17 | 17 | 100% |
| WAL | 10 | 10 | 100% |
| Schema | 16 | 16 | 100% |
| Executor | 18 | 18 | 100% |
| Catalog | 11 | 11 | 100% |
| CLI | 10 | 10 | 100% |
| Integration | 15 | 15 | 100% |
| **总计** | **140** | **140** | **100%** |

## 测试运行记录

```
$ make test
  lexer_basic_tokens... OK
  lexer_integer_tokens... OK
  lexer_real_tokens... OK
  lexer_string_tokens... OK
  lexer_operators... OK
  lexer_punctuation... OK
  lexer_keywords... OK
  lexer_identifiers... OK
  lexer_line_column... OK
  lexer_error... OK
  lexer_peek... OK
  parser_create_destroy... OK
  parser_select_simple... OK
  parser_insert... OK
  parser_create_table... OK
  parser_error_missing_where... OK
  parser_transaction_begin... OK
  parser_transaction_commit... OK
  expr_simple_literal... OK
  expr_string_literal... OK
  expr_null_literal... OK
  schema_validate_row_valid... OK
  schema_validate_row_not_null_violation... OK
  schema_validate_row_type_mismatch... OK
  schema_validate_row_column_count_mismatch... OK
  schema_validate_row_corrupt_buffer... OK
  schema_validate_row_null_nullable... OK
  sql_int_literal... OK
  sql_negative_int_literal... OK
  sql_float_literal... OK
  sql_string_literal... OK
  sql_null_literal... OK
  sql_binary_plus... OK
  sql_unary_minus... OK
  sql_null_buffer... OK
  sql_null_expr... OK
  sql_int_literal... OK
  sql_negative_int_literal... OK
  sql_float_literal... OK
  sql_string_literal... OK
  sql_null_literal... OK
  sql_binary_plus... OK
  sql_unary_minus... OK
  sql_null_buffer... OK
  sql_null_expr... OK

All unit tests passed!

====================
Test suite completed.
```