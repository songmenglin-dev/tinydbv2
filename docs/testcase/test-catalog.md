# Catalog 测试用例

## 测试范围

测试 `src/sql/catalog.c` 的目录管理功能，包括目录结构、常量、序列化/反序列化、生命周期管理、游标迭代、内存管理等。

## 实际测试用例 (基于 test-catalog.c)

### TC-CATALOG-001: Catalog 条目创建

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-001 |
| **标题** | Catalog 条目创建 |
| **优先级** | P0 |
| **实际测试** | `test(catalog_entry_creation)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 定义 `CatalogEntry` 结构体 2. 设置 type、name、tbl_name、sql 字段 3. 验证字段值正确 |
| **预期结果** | 条目字段正确赋值，type 为 CATALOG_TYPE_TABLE |
| **测试结果** | PASS |

---

### TC-CATALOG-002: Catalog 条目类型枚举

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-002 |
| **标题** | Catalog 条目类型枚举 |
| **优先级** | P0 |
| **实际测试** | `test(catalog_entry_types)` |
| **前置条件** | 无 |
| **测试步骤** | 验证 `CATALOG_TYPE_TABLE = 1`、`CATALOG_TYPE_INDEX = 2` |
| **预期结果** | 枚举值符合预期 |
| **测试结果** | PASS |

---

### TC-CATALOG-003: Catalog 游标结构体

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-003 |
| **标题** | Catalog 游标结构体 |
| **优先级** | P1 |
| **实际测试** | `test(catalog_cursor_struct_size)` |
| **前置条件** | 无 |
| **测试步骤** | 定义 `CatalogCursor` 结构体并验证非空 |
| **预期结果** | 游标结构体可正常创建 |
| **测试结果** | PASS |

---

### TC-CATALOG-004: Catalog 常量定义

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-004 |
| **标题** | Catalog 常量定义 |
| **优先级** | P0 |
| **实际测试** | `test(catalog_constants)` |
| **前置条件** | 无 |
| **测试步骤** | 验证 `CATALOG_TABLE_NAME = "tinydb_master"`、条目类型枚举值 |
| **预期结果** | 所有常量值正确 |
| **测试结果** | PASS |

---

### TC-CATALOG-005: ColumnInfo 结构体

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-005 |
| **标题** | ColumnInfo 结构体 |
| **优先级** | P0 |
| **实际测试** | `test(catalog_column_info_struct)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 定义 `ColumnInfo` 结构体 2. 设置 name、type、not_null、primary_key、autoincrement、default_val 3. 验证各字段正确 |
| **预期结果** | 所有字段正确赋值 |
| **测试结果** | PASS |

---

### TC-CATALOG-006: 列信息序列化与反序列化

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-006 |
| **标题** | 列信息序列化与反序列化 |
| **优先级** | P0 |
| **实际测试** | `test(column_serialize_deserialize)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 ColumnInfo 填充 "username" 字段 2. 调用 `serialize_column_info()` 序列化 3. 验证序列化后首字节为 'u'、末字节为 'e' |
| **预期结果** | 序列化成功，name 字段正确编码 |
| **测试结果** | PASS |

---

### TC-CATALOG-007: 列信息序列化往返测试

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-007 |
| **标题** | 列信息序列化往返测试 |
| **优先级** | P0 |
| **实际测试** | `test(column_serialize_round_trip)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 ColumnInfo（name="created_at", type=1, not_null=1） 2. `serialize_column_info()` 3. `deserialize_column_info()` 4. 验证所有字段一致 |
| **预期结果** | 往返后数据完全一致 |
| **测试结果** | PASS |

---

### TC-CATALOG-008: 目录条目序列化与反序列化

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-008 |
| **标题** | 目录条目序列化与反序列化 |
| **优先级** | P0 |
| **实际测试** | `test(entry_serialize_deserialize)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建含 2 列的 CatalogEntry（products 表） 2. `serialize_entry()` 3. `deserialize_entry()` 4. 验证 type、name、root_page、column_count、columns |
| **预期结果** | 往返后条目数据完全一致 |
| **测试结果** | PASS |

---

### TC-CATALOG-009: 目录条目序列化往返（无列）

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-009 |
| **标题** | 目录条目序列化往返（无列） |
| **优先级** | P0 |
| **实际测试** | `test(entry_serialize_round_trip)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建无列的 CatalogEntry（INDEX 类型） 2. `serialize_entry()` 3. `deserialize_entry()` 4. 验证 type、name、tbl_name、column_count=0、columns=NULL |
| **预期结果** | 往返后 index 条目数据一致，列数为 0 |
| **测试结果** | PASS |

---

### TC-CATALOG-010: Catalog 打开与关闭

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-010 |
| **标题** | Catalog 打开与关闭 |
| **优先级** | P0 |
| **实际测试** | `test(catalog_open_close)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 Pager 和 PageCache 2. 调用 `catalog_open()` 3. 验证返回非 NULL 且 is_open=1 4. 调用 `catalog_close()` 清理 |
| **预期结果** | Catalog 正常打开，关闭后资源正确释放 |
| **测试结果** | PASS |

---

### TC-CATALOG-011: Catalog 初始化与迭代

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-011 |
| **标题** | Catalog 初始化与迭代 |
| **优先级** | P0 |
| **实际测试** | `test(catalog_init_and_reopen)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建并初始化 Catalog 2. 创建游标 3. 验证游标有效 4. 获取 tinydb_master 条目 5. 验证名称正确 |
| **预期结果** | tinydb_master 条目可被正确查询 |
| **测试结果** | PASS |

---

### TC-CATALOG-012: Catalog 插入条目

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-012 |
| **标题** | Catalog 插入条目 |
| **优先级** | P0 |
| **实际测试** | `test(catalog_insert_entry)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建并初始化 Catalog 2. 构造 CatalogEntry（orders 表） 3. 调用 `catalog_insert()` 4. 验证返回 SUCCESS |
| **预期结果** | 条目成功插入，无错误 |
| **测试结果** | PASS |

---

### TC-CATALOG-013: 按类型和名称查找条目

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-013 |
| **标题** | 按类型和名称查找条目 |
| **优先级** | P0 |
| **实际测试** | `test(catalog_lookup_by_type_name)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建并初始化 Catalog 2. 插入 customers 表 3. 调用 `catalog_lookup_type_name()` 查找 customers 4. 验证找到的名称、类型 5. 查找不存在条目，验证返回 NULL 6. 用错误类型查找，验证返回 NULL |
| **预期结果** | 正常查找返回正确条目，不存在或类型错误返回 NULL |
| **测试结果** | PASS |

---

### TC-CATALOG-014: 获取所有表

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-014 |
| **标题** | 获取所有表 |
| **优先级** | P0 |
| **实际测试** | `test(catalog_get_tables)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建并初始化 Catalog 2. 插入 users 表和 posts 表 3. 插入 posts 的索引 4. 调用 `catalog_get_tables()` 5. 验证 count=2（不包含 tinydb_master 和索引） |
| **预期结果** | 返回用户定义的表，过滤 tinydb_master 和索引 |
| **测试结果** | PASS |

---

### TC-CATALOG-015: 空目录获取表

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-015 |
| **标题** | 空目录获取表 |
| **优先级** | P1 |
| **实际测试** | `test(catalog_get_tables_empty)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建并初始化 Catalog（仅含 tinydb_master） 2. 调用 `catalog_get_tables()` 3. 验证 count=0 |
| **预期结果** | 无用户表时返回 count=0 |
| **测试结果** | PASS |

---

### TC-CATALOG-016: 游标遍历目录

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-016 |
| **标题** | 游标遍历目录 |
| **优先级** | P0 |
| **实际测试** | `test(catalog_cursor_iterate)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建并初始化 Catalog 2. 创建游标 3. 验证游标有效 4. 获取第一条目 5. 验证条目名称为 tinydb_master |
| **预期结果** | 游标可正确遍历目录首条目 |
| **测试结果** | PASS |

---

### TC-CATALOG-017: 游标移动和有效性检查

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-017 |
| **标题** | 游标移动和有效性检查 |
| **优先级** | P0 |
| **实际测试** | `test(catalog_cursor_next_and_valid)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建并初始化 Catalog 2. 插入 logs 表 3. 使用游标遍历所有条目 4. 统计条目数 5. 验证 count=2（tinydb_master + logs） |
| **预期结果** | 游标可依次访问所有有效条目 |
| **测试结果** | PASS |

---

### TC-CATALOG-018: 目录条目数组释放

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CATALOG-018 |
| **标题** | 目录条目数组释放 |
| **优先级** | P1 |
| **实际测试** | `test(catalog_free_entries)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `catalog_free_entries(NULL, 0)` 和 `catalog_free_entries(NULL, 5)` 验证安全 2. 分配 3 个 CatalogEntry 并填充 3. 调用 `catalog_free_entries()` 释放 |
| **预期结果** | NULL 安全，分配的条目正确释放无内存泄漏 |
| **测试结果** | PASS |

---

## 测试结果汇总

| 测试用例 | 实际测试函数 | 结果 |
|---------|-------------|------|
| TC-CATALOG-001 | `catalog_entry_creation` | PASS |
| TC-CATALOG-002 | `catalog_entry_types` | PASS |
| TC-CATALOG-003 | `catalog_cursor_struct_size` | PASS |
| TC-CATALOG-004 | `catalog_constants` | PASS |
| TC-CATALOG-005 | `catalog_column_info_struct` | PASS |
| TC-CATALOG-006 | `column_serialize_deserialize` | PASS |
| TC-CATALOG-007 | `column_serialize_round_trip` | PASS |
| TC-CATALOG-008 | `entry_serialize_deserialize` | PASS |
| TC-CATALOG-009 | `entry_serialize_round_trip` | PASS |
| TC-CATALOG-010 | `catalog_open_close` | PASS |
| TC-CATALOG-011 | `catalog_init_and_reopen` | PASS |
| TC-CATALOG-012 | `catalog_insert_entry` | PASS |
| TC-CATALOG-013 | `catalog_lookup_by_type_name` | PASS |
| TC-CATALOG-014 | `catalog_get_tables` | PASS |
| TC-CATALOG-015 | `catalog_get_tables_empty` | PASS |
| TC-CATALOG-016 | `catalog_cursor_iterate` | PASS |
| TC-CATALOG-017 | `catalog_cursor_next_and_valid` | PASS |
| TC-CATALOG-018 | `catalog_free_entries` | PASS |

**通过率: 18/18 (100%)**

## 相关文件

- 测试代码: `tests/unit/test-catalog.c`
- 被测代码: `src/sql/catalog.c`
- 测试入口: `tests/unit/test-suite.c`