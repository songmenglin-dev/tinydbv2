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
| [test-pager.md](./test-pager.md) | 8 | 基于 `tests/unit/test-pager.c` |
| [test-page-cache.md](./test-page-cache.md) | 8 | 基于 `tests/unit/test-page-cache.c` |
| [test-catalog.md](./test-catalog.md) | 18 | 基于 `tests/unit/test-catalog.c` |
| [test-executor.md](./test-executor.md) | 4 | 基于 `tests/unit/test-executor.c` |

## 测试环境

- **平台**: Linux (WSL2)
- **编译器**: GCC
- **构建工具**: Make
- **测试框架**: mini_test.h (tests/unit/mini_test.h)

## 运行测试

```bash
# 运行所有单元测试
make test
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
| Pager | 8 | ✅ PASS |
| Page Cache | 8 | ✅ PASS |
| Catalog | 18 | ✅ PASS |
| Executor | 4 | ✅ PASS |
| **总计** | **129** | **100%** |

---

## 测试运行记录

```
$ make test
All unit tests passed!

====================
Test suite completed.
```

---

## 注意事项

1. **文档与代码对应**: 每个测试用例都标注了对应的 `test()` 函数名
2. **PASS 状态**: 基于 `make test` 运行结果
3. **覆盖率**: 所有核心模块均有测试覆盖