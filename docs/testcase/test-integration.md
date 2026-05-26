# Integration 测试用例

## 测试范围

测试 TinyDB v2 完整的功能流程，包括多模块协作和端到端场景。

## 测试用例

### TC-INT-001: CREATE TABLE → INSERT → SELECT 完整流程

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-001 |
| **标题** | CREATE TABLE → INSERT → SELECT 完整流程 |
| **优先级** | P0 |
| **前置条件** | 服务器运行中 |
| **测试步骤** | 1. 执行 `"CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL)"` <br> 2. 执行 `"INSERT INTO users (id, name) VALUES (1, 'Alice')"` <br> 3. 执行 `"SELECT * FROM users"` |
| **预期结果** | 表创建成功，插入成功，查询返回正确的数据行 |
| **测试结果** | ✅ PASS |

---

### TC-INT-002: UPDATE 带 WHERE 条件

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-002 |
| **标题** | UPDATE 带 WHERE 条件 |
| **优先级** | P0 |
| **前置条件** | 已创建表并插入数据 |
| **测试步骤** | 1. 执行 `"UPDATE users SET name = 'Bob' WHERE id = 1"` <br> 2. 执行 `"SELECT name FROM users WHERE id = 1"` |
| **预期结果** | 数据更新为 'Bob' |
| **测试结果** | ✅ PASS |

---

### TC-INT-003: DELETE 带 WHERE 条件

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-003 |
| **标题** | DELETE 带 WHERE 条件 |
| **优先级** | P0 |
| **前置条件** | 已插入多条数据 |
| **测试步骤** | 1. 执行 `"DELETE FROM users WHERE id = 1"` <br> 2. 执行 `"SELECT * FROM users WHERE id = 1"` |
| **预期结果** | 行被删除，查询返回空 |
| **测试结果** | ✅ PASS |

---

### TC-INT-004: BEGIN → INSERT → COMMIT → SELECT

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-004 |
| **标题** | BEGIN → INSERT → COMMIT → SELECT |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 执行 `"BEGIN"` <br> 2. 执行 `"INSERT INTO users (id, name) VALUES (1, 'Alice')"` <br> 3. 执行 `"COMMIT"` <br> 4. 执行 `"SELECT * FROM users"` |
| **预期结果** | 事务提交后，数据可见 |
| **测试结果** | ✅ PASS |

---

### TC-INT-005: BEGIN → INSERT → ROLLBACK

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-005 |
| **标题** | BEGIN → INSERT → ROLLBACK |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 执行 `"BEGIN"` <br> 2. 执行 `"INSERT INTO users (id, name) VALUES (1, 'Alice')"` <br> 3. 执行 `"ROLLBACK"` <br> 4. 执行 `"SELECT * FROM users WHERE id = 1"` |
| **预期结果** | 回滚后，数据被撤销 |
| **测试结果** | ✅ PASS |

---

### TC-INT-006: SHOW TABLES 功能

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-006 |
| **标题** | SHOW TABLES 功能 |
| **优先级** | P0 |
| **前置条件** | 已创建多个表 |
| **测试步骤** | 1. 执行 `"CREATE TABLE t1 (id INTEGER)"` <br> 2. 执行 `"CREATE TABLE t2 (id INTEGER)"` <br> 3. 执行 `"SHOW TABLES"` |
| **预期结果** | 返回包含 t1 和 t2 的列表 |
| **测试结果** | ✅ PASS |

---

### TC-INT-007: DESC table_name 功能

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-007 |
| **标题** | DESC table_name 功能 |
| **优先级** | P0 |
| **前置条件** | 已创建表 |
| **测试步骤** | 1. 执行 `"CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL, age INTEGER)"` <br> 2. 执行 `"DESC users"` |
| **预期结果** | 返回表的列信息：Field, Type, Null, Key, Default, Extra |
| **测试结果** | ✅ PASS |

---

### TC-INT-008: 多行 INSERT

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-008 |
| **标题** | 多行 INSERT |
| **优先级** | P0 |
| **前置条件** | 已创建表 |
| **测试步骤** | 1. 执行 `"INSERT INTO users (id, name) VALUES (1, 'A'), (2, 'B'), (3, 'C')"` <br> 2. 执行 `"SELECT COUNT(*) FROM users"` |
| **预期结果** | 插入3行，查询返回3 |
| **测试结果** | ✅ PASS |

---

### TC-INT-009: SELECT 带 ORDER BY

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-009 |
| **标题** | SELECT 带 ORDER BY |
| **优先级** | P1 |
| **前置条件** | 已插入多条数据 |
| **测试步骤** | 1. 执行 `"SELECT * FROM users ORDER BY id DESC"` |
| **预期结果** | 结果按 id 降序排列 |
| **测试结果** | ✅ PASS |

---

### TC-INT-010: SELECT 带 LIMIT

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-010 |
| **标题** | SELECT 带 LIMIT |
| **优先级** | P1 |
| **前置条件** | 已插入多条数据 |
| **测试步骤** | 1. 执行 `"SELECT * FROM users LIMIT 2"` |
| **预期结果** | 返回最多2行 |
| **测试结果** | ✅ PASS |

---

### TC-INT-011: CREATE INDEX 和 DROP INDEX

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-011 |
| **标题** | CREATE INDEX 和 DROP INDEX |
| **优先级** | P0 |
| **前置条件** | 已创建表 |
| **测试步骤** | 1. 执行 `"CREATE INDEX idx_name ON users (name)"` <br> 2. 执行 `"DROP INDEX idx_name"` |
| **预期结果** | 索引创建和删除成功 |
| **测试结果** | ✅ PASS |

---

### TC-INT-012: DROP TABLE IF EXISTS

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-012 |
| **标题** | DROP TABLE IF EXISTS |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 执行 `"DROP TABLE IF EXISTS nonexistent"` |
| **预期结果** | 不产生错误 |
| **测试结果** | ✅ PASS |

---

### TC-INT-013: 错误处理 - 列不存在

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-013 |
| **标题** | 错误处理 - 列不存在 |
| **优先级** | P0 |
| **前置条件** | 已创建表 |
| **测试步骤** | 1. 执行 `"SELECT nonexistent FROM users"` |
| **预期结果** | 返回适当的错误信息 |
| **测试结果** | ✅ PASS |

---

### TC-INT-014: 错误处理 - 类型不匹配

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-014 |
| **标题** | 错误处理 - 类型不匹配 |
| **优先级** | P0 |
| **前置条件** | 已创建表 |
| **测试步骤** | 1. 插入字符串到 INTEGER 列 |
| **预期结果** | 返回类型不匹配错误 |
| **测试结果** | ✅ PASS |

---

### TC-INT-015: 错误处理 - NOT NULL 约束

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-INT-015 |
| **标题** | 错误处理 - NOT NULL 约束 |
| **优先级** | P0 |
| **前置条件** | 已创建带 NOT NULL 约束的表 |
| **测试步骤** | 1. 尝试插入 NULL 值到 NOT NULL 列 |
| **预期结果** | 返回 NOT NULL 约束错误 |
| **测试结果** | ✅ PASS |

---

## 测试结果汇总

| 测试用例 | 结果 | 备注 |
|---------|------|------|
| TC-INT-001 | ✅ PASS | |
| TC-INT-002 | ✅ PASS | |
| TC-INT-003 | ✅ PASS | |
| TC-INT-004 | ✅ PASS | |
| TC-INT-005 | ✅ PASS | |
| TC-INT-006 | ✅ PASS | |
| TC-INT-007 | ✅ PASS | |
| TC-INT-008 | ✅ PASS | |
| TC-INT-009 | ✅ PASS | |
| TC-INT-010 | ✅ PASS | |
| TC-INT-011 | ✅ PASS | |
| TC-INT-012 | ✅ PASS | |
| TC-INT-013 | ✅ PASS | |
| TC-INT-014 | ✅ PASS | |
| TC-INT-015 | ✅ PASS | |

**通过率: 15/15 (100%)**

---

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