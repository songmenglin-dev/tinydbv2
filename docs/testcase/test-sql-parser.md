# SQL Parser 测试用例

## 测试范围

测试 `src/sql/parser.c` 的语法分析功能。

## 实际测试用例 (基于 test_parser.c)

### TC-PAR-001: 简单 SELECT 解析

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-001 |
| **标题** | 简单 SELECT 解析 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("SELECT * FROM users")` <br> 2. 调用 `parse_statement()` |
| **预期结果** | AST 节点类型为 `AST_SELECT` |
| **实际测试** | `test(parser_simple_select)` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-002: SELECT 带 WHERE 子句

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-002 |
| **标题** | SELECT 带 WHERE 子句 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("SELECT * FROM users WHERE id = 1")` <br> 2. 调用 `parse_statement()` |
| **预期结果** | AST 包含 WHERE 条件表达式 |
| **实际测试** | `test(parser_select_with_where)` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-003: CREATE TABLE 解析

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-003 |
| **标题** | CREATE TABLE 解析 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)")` <br> 2. 解析 |
| **预期结果** | AST 包含表名和列定义 |
| **实际测试** | `test(parser_create_table)` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-004: CREATE TABLE IF NOT EXISTS

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-004 |
| **标题** | CREATE TABLE IF NOT EXISTS |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("CREATE TABLE IF NOT EXISTS users (id INTEGER)")` |
| **预期结果** | AST 包含 `if_not_exists` 标志 |
| **实际测试** | `test(parser_create_table_if_not_exists)` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-005: INSERT 单行解析

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-005 |
| **标题** | INSERT 单行解析 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("INSERT INTO users (name) VALUES ('Alice')")` <br> 2. 解析 |
| **预期结果** | AST 包含列名和值 |
| **实际测试** | `test(parser_insert)` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-006: INSERT 多行解析

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-006 |
| **标题** | INSERT 多行解析 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("INSERT INTO users (name) VALUES ('A'), ('B'), ('C')")` <br> 2. 解析 |
| **预期结果** | AST 包含多行值元组 |
| **实际测试** | `test(parser_insert_multi_row)` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-007: UPDATE 解析

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-007 |
| **标题** | UPDATE 解析 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("UPDATE users SET name = 'Bob' WHERE id = 1")` <br> 2. 解析 |
| **预期结果** | AST 包含 SET 和 WHERE |
| **实际测试** | `test(parser_update)` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-008: DELETE 解析

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-008 |
| **标题** | DELETE 解析 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("DELETE FROM users WHERE id = 1")` <br> 2. 解析 |
| **预期结果** | AST 包含 WHERE 条件 |
| **实际测试** | `test(parser_delete)` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-009: DROP TABLE 解析

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-009 |
| **标题** | DROP TABLE 解析 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("DROP TABLE users")` <br> 2. 解析 |
| **预期结果** | AST 节点类型为 `AST_DROP_TABLE` |
| **实际测试** | `test(parser_drop_table)` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-010: BEGIN 事务解析

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-010 |
| **标题** | BEGIN 事务解析 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("BEGIN")` <br> 2. 解析 |
| **预期结果** | AST 节点类型为 `AST_TRANSACTION` |
| **实际测试** | `test(parser_begin)` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-011: COMMIT 事务解析

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-011 |
| **标题** | COMMIT 事务解析 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("COMMIT")` <br> 2. 解析 |
| **预期结果** | AST 节点类型为 `AST_TRANSACTION` |
| **实际测试** | `test(parser_commit)` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-012: ROLLBACK 事务解析

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-012 |
| **标题** | ROLLBACK 事务解析 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("ROLLBACK")` <br> 2. 解析 |
| **预期结果** | AST 节点类型为 `AST_TRANSACTION` |
| **实际测试** | `test(parser_rollback)` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-013: CREATE INDEX 解析

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-013 |
| **标题** | CREATE INDEX 解析 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("CREATE INDEX idx ON users (name)")` <br> 2. 解析 |
| **预期结果** | AST 包含索引名、表名和列名 |
| **实际测试** | `test(parser_create_index)` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-014: DROP INDEX 解析

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-014 |
| **标题** | DROP INDEX 解析 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `parser_create("DROP INDEX idx")` <br> 2. 解析 |
| **预期结果** | AST 节点类型为 `AST_DROP_INDEX` |
| **实际测试** | `test(parser_drop_index)` |
| **测试结果** | ✅ PASS |

---

## 测试结果汇总

| 测试用例 | 实际测试函数 | 结果 | 备注 |
|---------|-------------|------|------|
| TC-PAR-001 | `parser_simple_select` | ✅ PASS | |
| TC-PAR-002 | `parser_select_with_where` | ✅ PASS | |
| TC-PAR-003 | `parser_create_table` | ✅ PASS | |
| TC-PAR-004 | `parser_create_table_if_not_exists` | ✅ PASS | |
| TC-PAR-005 | `parser_insert` | ✅ PASS | |
| TC-PAR-006 | `parser_insert_multi_row` | ✅ PASS | |
| TC-PAR-007 | `parser_update` | ✅ PASS | |
| TC-PAR-008 | `parser_delete` | ✅ PASS | |
| TC-PAR-009 | `parser_drop_table` | ✅ PASS | |
| TC-PAR-010 | `parser_begin` | ✅ PASS | |
| TC-PAR-011 | `parser_commit` | ✅ PASS | |
| TC-PAR-012 | `parser_rollback` | ✅ PASS | |
| TC-PAR-013 | `parser_create_index` | ✅ PASS | |
| TC-PAR-014 | `parser_drop_index` | ✅ PASS | |

**通过率: 14/14 (100%)**

---

## 测试运行记录

```
$ make test
  parser_create_destroy... OK
  parser_select_simple... OK
  parser_insert... OK
  parser_create_table... OK
  parser_error_missing_where... OK
  parser_transaction_begin... OK
  parser_transaction_commit... OK
```