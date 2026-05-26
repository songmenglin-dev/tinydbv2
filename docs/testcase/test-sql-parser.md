# SQL Parser 测试用例

## 测试范围

测试 `src/sql/parser.c` 的语法分析功能。

## 测试用例

### TC-PAR-001: 简单 SELECT

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-001 |
| **标题** | 简单 SELECT |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"SELECT * FROM users"` |
| **预期结果** | AST 节点类型为 `AST_SELECT`，包含 table_name="users" |
| **测试结果** | ✅ PASS |

---

### TC-PAR-002: 带 WHERE 子句的 SELECT

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-002 |
| **标题** | 带 WHERE 子句的 SELECT |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"SELECT id, name FROM users WHERE age > 18"` |
| **预期结果** | AST 包含 WHERE 条件表达式 |
| **测试结果** | ✅ PASS |

---

### TC-PAR-003: 单行 INSERT

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-003 |
| **标题** | 单行 INSERT |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"INSERT INTO users (name, age) VALUES ('Alice', 25)"` |
| **预期结果** | AST 节点类型为 `AST_INSERT`，包含列名和值 |
| **测试结果** | ✅ PASS |

---

### TC-PAR-004: 多行 INSERT

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-004 |
| **标题** | 多行 INSERT |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"INSERT INTO users (name, age) VALUES ('Alice', 25), ('Bob', 30)"` |
| **预期结果** | AST 包含两行值元组 |
| **测试结果** | ✅ PASS |

---

### TC-PAR-005: UPDATE 语句

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-005 |
| **标题** | UPDATE 语句 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"UPDATE users SET age = 30 WHERE id = 1"` |
| **预期结果** | AST 节点类型为 `AST_UPDATE`，包含 SET 和 WHERE |
| **测试结果** | ✅ PASS |

---

### TC-PAR-006: DELETE 语句

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-006 |
| **标题** | DELETE 语句 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"DELETE FROM users WHERE id = 1"` |
| **预期结果** | AST 节点类型为 `AST_DELETE`，包含 WHERE 条件 |
| **测试结果** | ✅ PASS |

---

### TC-PAR-007: CREATE TABLE

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-007 |
| **标题** | CREATE TABLE |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL)"` |
| **预期结果** | AST 包含表名和列定义 |
| **测试结果** | ✅ PASS |

---

### TC-PAR-008: CREATE TABLE IF NOT EXISTS

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-008 |
| **标题** | CREATE TABLE IF NOT EXISTS |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"CREATE TABLE IF NOT EXISTS users (id INTEGER)"` |
| **预期结果** | AST 包含 `if_not_exists` 标志 |
| **测试结果** | ✅ PASS |

---

### TC-PAR-009: DROP TABLE

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-009 |
| **标题** | DROP TABLE |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"DROP TABLE users"` |
| **预期结果** | AST 节点类型为 `AST_DROP_TABLE` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-010: CREATE INDEX

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-010 |
| **标题** | CREATE INDEX |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"CREATE INDEX idx_name ON users (name)"` |
| **预期结果** | AST 包含索引名、表名和列名 |
| **测试结果** | ✅ PASS |

---

### TC-PAR-011: DROP INDEX

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-011 |
| **标题** | DROP INDEX |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"DROP INDEX idx_name"` |
| **预期结果** | AST 节点类型为 `AST_DROP_INDEX` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-012: BEGIN TRANSACTION

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-012 |
| **标题** | BEGIN TRANSACTION |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"BEGIN"` |
| **预期结果** | AST 节点类型为 `AST_TRANSACTION` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-013: COMMIT TRANSACTION

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-013 |
| **标题** | COMMIT TRANSACTION |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"COMMIT"` |
| **预期结果** | AST 节点类型为 `AST_TRANSACTION` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-014: ROLLBACK TRANSACTION

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-014 |
| **标题** | ROLLBACK TRANSACTION |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"ROLLBACK"` |
| **预期结果** | AST 节点类型为 `AST_TRANSACTION` |
| **测试结果** | ✅ PASS |

---

### TC-PAR-015: 解析错误处理

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-015 |
| **标题** | 解析错误处理 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"SELECT FROM"` (缺少列名) |
| **预期结果** | Parser 设置 has_error 标志 |
| **测试结果** | ✅ PASS |

---

### TC-PAR-016: INT 类型别名

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-016 |
| **标题** | INT 类型别名 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"CREATE TABLE t (c INT)"` |
| **预期结果** | 列类型正确解析为 INTEGER |
| **测试结果** | ✅ PASS |

---

### TC-PAR-017: Qualified column names

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAR-017 |
| **标题** | Qualified column names |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 解析 `"SELECT users.name FROM users"` |
| **预期结果** | 列名解析为 qualified 格式 |
| **测试结果** | ✅ PASS |

---

## 测试结果汇总

| 测试用例 | 结果 | 备注 |
|---------|------|------|
| TC-PAR-001 | ✅ PASS | |
| TC-PAR-002 | ✅ PASS | |
| TC-PAR-003 | ✅ PASS | |
| TC-PAR-004 | ✅ PASS | |
| TC-PAR-005 | ✅ PASS | |
| TC-PAR-006 | ✅ PASS | |
| TC-PAR-007 | ✅ PASS | |
| TC-PAR-008 | ✅ PASS | |
| TC-PAR-009 | ✅ PASS | |
| TC-PAR-010 | ✅ PASS | |
| TC-PAR-011 | ✅ PASS | |
| TC-PAR-012 | ✅ PASS | |
| TC-PAR-013 | ✅ PASS | |
| TC-PAR-014 | ✅ PASS | |
| TC-PAR-015 | ✅ PASS | |
| TC-PAR-016 | ✅ PASS | |
| TC-PAR-017 | ✅ PASS | |

**通过率: 17/17 (100%)**