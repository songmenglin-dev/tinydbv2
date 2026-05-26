# Catalog 测试用例

## 测试范围

测试 `src/sql/catalog.c` 的目录管理功能。

## 测试用例

### TC-CATALOG-001: Catalog 打开

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-001 |
| **标题** | Catalog 打开 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `catalog_open()` 打开目录 |
| **预期结果** | Catalog 成功打开 |
| **测试结果** | ✅ PASS |

---

### TC-CATALOG-002: Catalog 插入条目

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-002 |
| **标题** | Catalog 插入条目 |
| **优先级** | P0 |
| **前置条件** | Catalog 已打开 |
| **测试步骤** | 1. 准备 CatalogEntry <br> 2. 调用 `catalog_insert()` |
| **预期结果** | 条目成功插入 |
| **测试结果** | ✅ PASS |

---

### TC-CATALOG-003: Catalog 删除条目

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-003 |
| **标题** | Catalog 删除条目 |
| **优先级** | P0 |
| **前置条件** | 已插入条目 |
| **测试步骤** | 1. 调用 `catalog_delete()` |
| **预期结果** | 条目成功删除 |
| **测试结果** | ✅ PASS |

---

### TC-CATALOG-004: Catalog 按类型名查找

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-004 |
| **标题** | Catalog 按类型名查找 |
| **优先级** | P0 |
| **前置条件** | 已插入条目 |
| **测试步骤** | 1. 调用 `catalog_lookup_type_name()` |
| **预期结果** | 返回匹配的条目 |
| **测试结果** | ✅ PASS |

---

### TC-CATALOG-005: Catalog 查找不存在的条目

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-005 |
| **标题** | Catalog 查找不存在的条目 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `catalog_lookup_type_name()` 查找不存在的名称 |
| **预期结果** | 返回 NULL |
| **测试结果** | ✅ PASS |

---

### TC-CATALOG-006: Catalog 获取所有表

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-006 |
| **标题** | Catalog 获取所有表 |
| **优先级** | P0 |
| **前置条件** | 已创建多个表 |
| **测试步骤** | 1. 调用 `catalog_get_tables()` |
| **预期结果** | 返回所有表条目 |
| **测试结果** | ✅ PASS |

---

### TC-CATALOG-007: Catalog 获取表的索引

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-007 |
| **标题** | Catalog 获取表的索引 |
| **优先级** | P1 |
| **前置条件** | 已创建索引 |
| **测试步骤** | 1. 调用 `catalog_get_indexes()` |
| **预期结果** | 返回该表的所有索引 |
| **测试结果** | ✅ PASS |

---

### TC-CATALOG-008: Catalog 获取列信息

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-008 |
| **标题** | Catalog 获取列信息 |
| **优先级** | P0 |
| **前置条件** | 已创建表 |
| **测试步骤** | 1. 调用 `catalog_get_columns()` |
| **预期结果** | 返回该表的所有列信息 |
| **测试结果** | ✅ PASS |

---

### TC-CATALOG-009: Catalog 游标遍历

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-009 |
| **标题** | Catalog 游标遍历 |
| **优先级** | P1 |
| **前置条件** | 已插入多个条目 |
| **测试步骤** | 1. 调用 `catalog_cursor_create()` <br> 2. 遍历所有条目 <br> 3. 调用 `catalog_cursor_free()` |
| **预期结果** | 正确遍历所有条目 |
| **测试结果** | ✅ PASS |

---

### TC-CATALOG-010: 列信息序列化

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-010 |
| **标题** | 列信息序列化 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `serialize_column_info()` |
| **预期结果** | 返回序列化的列信息字节 |
| **测试结果** | ✅ PASS |

---

### TC-CATALOG-011: 列信息反序列化

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-011 |
| **标题** | 列信息反序列化 |
| **优先级** | P0 |
| **前置条件** | 已序列化列信息 |
| **测试步骤** | 1. 调用 `deserialize_column_info()` |
| **预期结果** | 返回正确的 ColumnInfo 结构 |
| **测试结果** | ✅ PASS |

---

## 测试结果汇总

| 测试用例 | 结果 | 备注 |
|---------|------|------|
| TC-CATALOG-001 | ✅ PASS | |
| TC-CATALOG-002 | ✅ PASS | |
| TC-CATALOG-003 | ✅ PASS | |
| TC-CATALOG-004 | ✅ PASS | |
| TC-CATALOG-005 | ✅ PASS | |
| TC-CATALOG-006 | ✅ PASS | |
| TC-CATALOG-007 | ✅ PASS | |
| TC-CATALOG-008 | ✅ PASS | |
| TC-CATALOG-009 | ✅ PASS | |
| TC-CATALOG-010 | ✅ PASS | |
| TC-CATALOG-011 | ✅ PASS | |

**通过率: 11/11 (100%)**