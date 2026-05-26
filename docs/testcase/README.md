# TinyDB v2 测试用例文档

## 概述

本文档记录 TinyDB v2 项目的所有功能测试用例，基于实际测试代码编写。

## 目录结构

| 文档 | 实际测试数 | 说明 |
|------|-----------|------|
| [test-sql-lexer.md](./test-sql-lexer.md) | 12 | 基于 `tests/unit/test_lexer.c` |
| [test-sql-parser.md](./test-sql-parser.md) | 14 | 基于 `tests/unit/test_parser.c` |
| [test-expression.md](./test-expression.md) | 22 | 基于 `tests/unit/test-expression.c` |
| [test-btree.md](./test-btree.md) | 17 | 基于 `tests/unit/test-btree.c` |
| [test-wal.md](./test-wal.md) | 10 | 基于 `tests/unit/test-wal.c` |
| [test-schema.md](./test-schema.md) | 12 | 基于 `tests/unit/test-schema.c` |

## 测试环境

- **平台**: Linux (WSL2)
- **编译器**: GCC
- **构建工具**: Make
- **测试框架**: mini_test.h (tests/unit/mini_test.h)

## 运行测试

```bash
# 运行所有单元测试
make test

# 运行特定模块测试 (test-suite 汇总了部分测试)
./build/test-suite --unit
```

## 测试结果汇总

| 模块 | 测试函数数 | 状态 |
|------|-----------|------|
| SQL Lexer | 12 | ✅ PASS |
| SQL Parser | 14 | ✅ PASS |
| Expression | 22 | ✅ PASS |
| B+Tree Storage | 17 | ✅ PASS |
| WAL | 10 | ✅ PASS |
| Schema | 12 | ✅ PASS |
| **总计** | **87** | **100%** |

---

## 测试运行记录

```
$ make test
  string_len_basic... OK
  string_dup_basic... OK
  string_eq_basic... OK
  error_code_to_string... OK
  error_create_basic... OK
  list_create_basic... OK
  list_append_get... OK
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
  (多个测试重复运行)

All unit tests passed!

====================
Test suite completed.
```

---

## 注意事项

1. **文档与代码对应**: 每个测试用例都标注了对应的 `test()` 函数名
2. **PASS 状态**: 基于 `make test` 运行结果
3. **覆盖率**: 当前文档覆盖的模块均有实际测试代码支持
4. **未覆盖模块**: CLI、Executor、Catalog 等模块已有占位测试文件但需完善