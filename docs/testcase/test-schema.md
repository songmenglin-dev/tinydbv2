# Schema 测试用例

## 测试范围

测试 `src/sql/schema.c` 的模式定义和验证功能。

## 实际测试用例 (基于 test-schema.c)

### TC-SCHEMA-001: Schema 创建

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-SCHEMA-001 |
| **标题** | Schema 创建 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 定义列定义数组 <br> 2. 调用 `schema_create()` |
| **预期结果** | Schema 创建成功 |
| **实际测试** | `test(schema_creation)` |
| **测试结果** | ✅ PASS |

---

### TC-SCHEMA-002: 列索引查找

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-SCHEMA-002 |
| **标题** | 列索引查找 |
| **优先级** | P0 |
| **前置条件** | 已创建 Schema |
| **测试步骤** | 1. 调用 `schema_column_index("列名")` |
| **预期结果** | 返回正确的列索引 |
| **实际测试** | `test(schema_column_index)` |
| **测试结果** | ✅ PASS |

---

### TC-SCHEMA-003: 按名称获取列

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-SCHEMA-003 |
| **标题** | 按名称获取列 |
| **优先级** | P0 |
| **前置条件** | 已创建 Schema |
| **测试步骤** | 1. 调用 `schema_column("列名")` |
| **预期结果** | 返回 ColumnDef 指针 |
| **实际测试** | `test(schema_column)` |
| **测试结果** | ✅ PASS |

---

### TC-SCHEMA-004: Schema 序列化

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-SCHEMA-004 |
| **标题** | Schema 序列化 |
| **优先级** | P0 |
| **前置条件** | 已创建 Schema |
| **测试步骤** | 1. 调用 `schema_serialize()` |
| **预期结果** | 返回序列化后的字节数据 |
| **实际测试** | `test(schema_serialization)` |
| **测试结果** | ✅ PASS |

---

### TC-SCHEMA-005: Schema 克隆

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-SCHEMA-005 |
| **标题** | Schema 克隆 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 Schema <br> 2. 调用 `schema_clone()` |
| **预期结果** | 返回相同的 Schema 副本 |
| **实际测试** | `test(schema_clone)` |
| **测试结果** | ✅ PASS |

---

### TC-SCHEMA-006: Schema 相等比较

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-SCHEMA-006 |
| **标题** | Schema 相等比较 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建两个相同的 Schema <br> 2. 调用 `schema_equal()` |
| **预期结果** | 返回 true |
| **实际测试** | `test(schema_equality)` |
| **测试结果** | ✅ PASS |

---

### TC-SCHEMA-007: 行验证 - 有效行

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-SCHEMA-007 |
| **标题** | 行验证 - 有效行 |
| **优先级** | P0 |
| **前置条件** | 已创建 Schema |
| **测试步骤** | 1. 准备符合 Schema 的行数据 <br> 2. 调用 `schema_validate_row()` |
| **预期结果** | 验证通过 |
| **实际测试** | `test(schema_validate_row_valid)` |
| **测试结果** | ✅ PASS |

---

### TC-SCHEMA-008: 行验证 - NOT NULL 违规

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-SCHEMA-008 |
| **标题** | 行验证 - NOT NULL 违规 |
| **优先级** | P0 |
| **前置条件** | Schema 中有 NOT NULL 列 |
| **测试步骤** | 1. 准备包含 NULL 的行数据 <br> 2. 调用 `schema_validate_row()` |
| **预期结果** | 返回验证错误 |
| **实际测试** | `test(schema_validate_row_not_null_violation)` |
| **测试结果** | ✅ PASS |

---

### TC-SCHEMA-009: 行验证 - 类型不匹配

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-SCHEMA-009 |
| **标题** | 行验证 - 类型不匹配 |
| **优先级** | P0 |
| **前置条件** | 已创建 Schema |
| **测试步骤** | 1. 准备类型不匹配的行数据 <br> 2. 调用 `schema_validate_row()` |
| **预期结果** | 返回验证错误 |
| **实际测试** | `test(schema_validate_row_type_mismatch)` |
| **测试结果** | ✅ PASS |

---

### TC-SCHEMA-010: 行验证 - 列数量不匹配

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-SCHEMA-010 |
| **标题** | 行验证 - 列数量不匹配 |
| **优先级** | P0 |
| **前置条件** | 已创建 Schema |
| **测试步骤** | 1. 准备列数量不匹配的行数据 <br> 2. 调用 `schema_validate_row()` |
| **预期结果** | 返回验证错误 |
| **实际测试** | `test(schema_validate_row_column_count_mismatch)` |
| **测试结果** | ✅ PASS |

---

### TC-SCHEMA-011: 行验证 - 损坏的缓冲区

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-SCHEMA-011 |
| **标题** | 行验证 - 损坏的缓冲区 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 准备损坏的行数据 <br> 2. 调用 `schema_validate_row()` |
| **预期结果** | 返回验证错误 |
| **实际测试** | `test(schema_validate_row_corrupt_buffer)` |
| **测试结果** | ✅ PASS |

---

### TC-SCHEMA-012: 行验证 - NULL 可为空列

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-SCHEMA-012 |
| **标题** | 行验证 - NULL 可为空列 |
| **优先级** | P0 |
| **前置条件** | Schema 中有可 NULL 列 |
| **测试步骤** | 1. 准备包含 NULL 的行数据 <br> 2. 调用 `schema_validate_row()` |
| **预期结果** | 验证通过 |
| **实际测试** | `test(schema_validate_row_null_nullable)` |
| **测试结果** | ✅ PASS |

---

## 测试结果汇总

| 测试用例 | 实际测试函数 | 结果 |
|---------|-------------|------|
| TC-SCHEMA-001 | `schema_creation` | ✅ PASS |
| TC-SCHEMA-002 | `schema_column_index` | ✅ PASS |
| TC-SCHEMA-003 | `schema_column` | ✅ PASS |
| TC-SCHEMA-004 | `schema_serialization` | ✅ PASS |
| TC-SCHEMA-005 | `schema_clone` | ✅ PASS |
| TC-SCHEMA-006 | `schema_equality` | ✅ PASS |
| TC-SCHEMA-007 | `schema_validate_row_valid` | ✅ PASS |
| TC-SCHEMA-008 | `schema_validate_row_not_null_violation` | ✅ PASS |
| TC-SCHEMA-009 | `schema_validate_row_type_mismatch` | ✅ PASS |
| TC-SCHEMA-010 | `schema_validate_row_column_count_mismatch` | ✅ PASS |
| TC-SCHEMA-011 | `schema_validate_row_corrupt_buffer` | ✅ PASS |
| TC-SCHEMA-012 | `schema_validate_row_null_nullable` | ✅ PASS |

**通过率: 12/12 (100%)**

---

## 测试运行记录

```
$ make test
  schema_validate_row_valid... OK
  schema_validate_row_not_null_violation... OK
  schema_validate_row_type_mismatch... OK
  schema_validate_row_column_count_mismatch... OK
  schema_validate_row_corrupt_buffer... OK
  schema_validate_row_null_nullable... OK
```