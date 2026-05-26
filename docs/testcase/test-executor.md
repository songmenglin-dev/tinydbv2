# Executor 测试用例

## 测试范围

测试 `src/sql/executor.c` 的 SQL 执行器功能。

## 测试用例

### TC-EXEC-001: Executor 创建和销毁

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-001 |
| **标题** | Executor 创建和销毁 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `executor_create()` <br> 2. 调用 `executor_destroy()` |
| **预期结果** | Executor 创建成功，销毁后无内存泄漏 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-002: 执行 CREATE TABLE

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-002 |
| **标题** | 执行 CREATE TABLE |
| **优先级** | P0 |
| **前置条件** | Executor 已创建 |
| **测试步骤** | 1. 解析 `"CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)"` <br> 2. 调用 `executor_exec()` 执行 |
| **预期结果** | 表创建成功 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-003: 执行 DROP TABLE

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-003 |
| **标题** | 执行 DROP TABLE |
| **优先级** | P0 |
| **前置条件** | 已创建表 |
| **测试步骤** | 1. 执行 DROP TABLE 语句 |
| **预期结果** | 表删除成功 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-004: 执行 CREATE INDEX

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-004 |
| **标题** | 执行 CREATE INDEX |
| **优先级** | P0 |
| **前置条件** | 已创建表 |
| **测试步骤** | 1. 执行 `"CREATE INDEX idx ON users (name)"` |
| **预期结果** | 索引创建成功 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-005: 执行 DROP INDEX

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-005 |
| **标题** | 执行 DROP INDEX |
| **优先级** | P0 |
| **前置条件** | 已创建索引 |
| **测试步骤** | 1. 执行 `"DROP INDEX idx"` |
| **预期结果** | 索引删除成功 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-006: 执行 INSERT 单行

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-006 |
| **标题** | 执行 INSERT 单行 |
| **优先级** | P0 |
| **前置条件** | 已创建表 users (id INTEGER, name TEXT) |
| **测试步骤** | 1. 执行 `"INSERT INTO users (id, name) VALUES (1, 'Alice')"` |
| **预期结果** | 插入成功 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-007: 执行 INSERT 多行

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-007 |
| **标题** | 执行 INSERT 多行 |
| **优先级** | P1 |
| **前置条件** | 已创建表 |
| **测试步骤** | 1. 执行 `"INSERT INTO users (id, name) VALUES (1, 'A'), (2, 'B')"` |
| **预期结果** | 多行插入成功 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-008: 执行 UPDATE

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-008 |
| **标题** | 执行 UPDATE |
| **优先级** | P0 |
| **前置条件** | 已插入数据 |
| **测试步骤** | 1. 执行 `"UPDATE users SET name = 'Bob' WHERE id = 1"` |
| **预期结果** | 更新成功，影响一行 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-009: 执行 DELETE

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-009 |
| **标题** | 执行 DELETE |
| **优先级** | P0 |
| **前置条件** | 已插入数据 |
| **测试步骤** | 1. 执行 `"DELETE FROM users WHERE id = 1"` |
| **预期结果** | 删除成功 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-010: 执行 SELECT 全表扫描

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-010 |
| **标题** | 执行 SELECT 全表扫描 |
| **优先级** | P0 |
| **前置条件** | 已插入数据 |
| **测试步骤** | 1. 执行 `"SELECT * FROM users"` |
| **预期结果** | 返回所有行 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-011: 执行 SELECT 带 WHERE

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-011 |
| **标题** | 执行 SELECT 带 WHERE |
| **优先级** | P0 |
| **前置条件** | 已插入数据 |
| **测试步骤** | 1. 执行 `"SELECT * FROM users WHERE id = 1"` |
| **预期结果** | 返回匹配的行 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-012: 执行 SHOW TABLES

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-012 |
| **标题** | 执行 SHOW TABLES |
| **优先级** | P0 |
| **前置条件** | 已创建多个表 |
| **测试步骤** | 1. 执行 `"SHOW TABLES"` |
| **预期结果** | 返回所有表名列表 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-013: 执行 DESC table

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-013 |
| **标题** | 执行 DESC table |
| **优先级** | P0 |
| **前置条件** | 已创建表 |
| **测试步骤** | 1. 执行 `"DESC users"` |
| **预期结果** | 返回表的所有列信息 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-014: 执行 BEGIN 事务

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-014 |
| **标题** | 执行 BEGIN 事务 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 执行 `"BEGIN"` |
| **预期结果** | 事务开始成功 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-015: 执行 COMMIT 事务

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-015 |
| **标题** | 执行 COMMIT 事务 |
| **优先级** | P0 |
| **前置条件** | 已开始事务 |
| **测试步骤** | 1. 执行 `"COMMIT"` |
| **预期结果** | 事务提交成功 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-016: 执行 ROLLBACK 事务

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-016 |
| **标题** | 执行 ROLLBACK 事务 |
| **优先级** | P0 |
| **前置条件** | 已开始事务并有未提交的修改 |
| **测试步骤** | 1. 执行 `"ROLLBACK"` |
| **预期结果** | 事务回滚成功，修改被撤销 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-017: Result Set 创建和添加行

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-017 |
| **标题** | Result Set 创建和添加行 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `result_set_create()` <br> 2. 调用 `result_set_add_row()` <br> 3. 调用 `result_set_free()` |
| **预期结果** | Result Set 正确创建和释放 |
| **测试结果** | ✅ PASS |

---

### TC-EXEC-018: 错误处理 - 表不存在

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXEC-018 |
| **标题** | 错误处理 - 表不存在 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 执行 `"SELECT * FROM nonexistent"` |
| **预期结果** | 返回适当的错误 |
| **测试结果** | ✅ PASS |

---

## 测试结果汇总

| 测试用例 | 结果 | 备注 |
|---------|------|------|
| TC-EXEC-001 | ✅ PASS | |
| TC-EXEC-002 | ✅ PASS | |
| TC-EXEC-003 | ✅ PASS | |
| TC-EXEC-004 | ✅ PASS | |
| TC-EXEC-005 | ✅ PASS | |
| TC-EXEC-006 | ✅ PASS | |
| TC-EXEC-007 | ✅ PASS | |
| TC-EXEC-008 | ✅ PASS | |
| TC-EXEC-009 | ✅ PASS | |
| TC-EXEC-010 | ✅ PASS | |
| TC-EXEC-011 | ✅ PASS | |
| TC-EXEC-012 | ✅ PASS | |
| TC-EXEC-013 | ✅ PASS | |
| TC-EXEC-014 | ✅ PASS | |
| TC-EXEC-015 | ✅ PASS | |
| TC-EXEC-016 | ✅ PASS | |
| TC-EXEC-017 | ✅ PASS | |
| TC-EXEC-018 | ✅ PASS | |

**通过率: 18/18 (100%)**