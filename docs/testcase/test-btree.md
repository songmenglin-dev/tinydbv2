# B+Tree Storage 测试用例

## 测试范围

测试 `src/storage/btree.c` 的 B+Tree 存储引擎功能。

## 实际测试用例 (基于 test-btree.c)

### TC-BTREE-001: B+Tree 创建和关闭

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-001 |
| **标题** | B+Tree 创建和关闭 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `pager_create()` 创建 pager <br> 2. 调用 `page_cache_create()` 创建缓存 <br> 3. 调用 `btree_create()` 创建树 <br> 4. 调用 `btree_close()` 关闭 |
| **预期结果** | 创建成功，关闭返回 0，无资源泄漏 |
| **实际测试** | `test(btree_create_and_close)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-002: 插入和查找

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-002 |
| **标题** | 插入和查找 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 插入键值对 (100, "hello") 和 (200, "world") <br> 2. 调用 `btree_find(100)` |
| **预期结果** | 返回游标指向正确的值 |
| **实际测试** | `test(btree_insert_and_find)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-003: 游标到第一个元素

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-003 |
| **标题** | 游标到第一个元素 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 插入 3 个键值对 (50, 30, 70) <br> 2. 调用 `btree_first()` |
| **预期结果** | 游标指向最小键 |
| **实际测试** | `test(btree_cursor_first)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-004: 游标向前迭代

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-004 |
| **标题** | 游标向前迭代 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 插入 3 个键值对 <br> 2. 调用 `btree_first()` 后多次调用 `btree_cursor_next()` |
| **预期结果** | 游标正确遍历所有键 |
| **实际测试** | `test(btree_cursor_next)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-005: 游标到最后一个元素

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-005 |
| **标题** | 游标到最后一个元素 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 插入 3 个键值对 <br> 2. 调用 `btree_last()` |
| **预期结果** | 游标指向最大键 |
| **实际测试** | `test(btree_cursor_last)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-006: 游标有效性检查

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-006 |
| **标题** | 游标有效性检查 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 获取有效游标后调用 `btree_cursor_valid()` <br> 2. 迭代到末尾后再次检查 |
| **预期结果** | 有效时返回 1，无效时返回 0 |
| **实际测试** | `test(btree_cursor_valid)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-007: Get 和 Peek

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-007 |
| **标题** | Get 和 Peek |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 获取游标 <br> 2. 调用 `btree_cursor_peek()` 预览 <br> 3. 验证游标未移动 <br> 4. 调用 `btree_get()` 获取并移动 |
| **预期结果** | peek 不消耗游标，get 消耗并返回数据 |
| **实际测试** | `test(btree_get_and_peek)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-008: 范围扫描

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-008 |
| **标题** | 范围扫描 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 插入多个键值对 <br> 2. 调用 `btree_range_new()` 创建范围游标 |
| **预期结果** | 范围游标只返回范围内的键 |
| **实际测试** | `test(btree_range_scan)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-009: 删除基本操作

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-009 |
| **标题** | 删除基本操作 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 插入键值对 <br> 2. 调用 `btree_delete()` 删除 <br> 3. 尝试查找已删除的键 |
| **预期结果** | 删除成功，查找返回 NULL |
| **实际测试** | `test(btree_delete_basic)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-010: 删除不存在的键

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-010 |
| **标题** | 删除不存在的键 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `btree_delete(999)` |
| **预期结果** | 不崩溃，返回错误 |
| **实际测试** | `test(btree_delete_nonexistent)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-011: 更新已存在的键

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-011 |
| **标题** | 更新已存在的键 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 插入 (key, "old") <br> 2. 再次插入 (key, "new") <br> 3. 查找该键 |
| **预期结果** | 返回更新后的值 "new" |
| **实际测试** | `test(btree_update_existing)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-012: 查找不存在的键

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-012 |
| **标题** | 查找不存在的键 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 插入键值对 <br> 2. 调用 `btree_find(999)` |
| **预期结果** | 返回 NULL |
| **实际测试** | `test(btree_find_nonexistent)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-013: 游标向后迭代

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-013 |
| **标题** | 游标向后迭代 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 插入 3 个键值对 <br> 2. 调用 `btree_last()` 后调用 `btree_cursor_prev()` |
| **预期结果** | 游标正确移动到上一个键 |
| **实际测试** | `test(btree_cursor_prev)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-014: 多次插入

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-014 |
| **标题** | 多次插入 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 连续插入 100 个键值对 <br> 2. 验证所有键可找到 |
| **预期结果** | 所有插入成功，所有键可找到 |
| **实际测试** | `test(btree_multiple_inserts)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-015: 页面分裂

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-015 |
| **标题** | 页面分裂 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 插入大量数据触发页面分裂 <br> 2. 验证所有数据完整性 |
| **预期结果** | 页面正确分裂，数据不丢失 |
| **实际测试** | `test(btree_page_split)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-016: 空树验证

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-016 |
| **标题** | 空树验证 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建空树 <br> 2. 调用 `btree_verify()` |
| **预期结果** | 返回成功状态 |
| **实际测试** | `test(btree_verify_empty)` |
| **测试结果** | ✅ PASS |

---

### TC-BTREE-017: 带数据的树验证

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-BTREE-017 |
| **标题** | 带数据的树验证 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 插入数据后调用 `btree_verify()` |
| **预期结果** | 返回成功状态 |
| **实际测试** | `test(btree_verify_with_data)` |
| **测试结果** | ✅ PASS |

---

## 测试结果汇总

| 测试用例 | 实际测试函数 | 结果 |
|---------|-------------|------|
| TC-BTREE-001 | `btree_create_and_close` | ✅ PASS |
| TC-BTREE-002 | `btree_insert_and_find` | ✅ PASS |
| TC-BTREE-003 | `btree_cursor_first` | ✅ PASS |
| TC-BTREE-004 | `btree_cursor_next` | ✅ PASS |
| TC-BTREE-005 | `btree_cursor_last` | ✅ PASS |
| TC-BTREE-006 | `btree_cursor_valid` | ✅ PASS |
| TC-BTREE-007 | `btree_get_and_peek` | ✅ PASS |
| TC-BTREE-008 | `btree_range_scan` | ✅ PASS |
| TC-BTREE-009 | `btree_delete_basic` | ✅ PASS |
| TC-BTREE-010 | `btree_delete_nonexistent` | ✅ PASS |
| TC-BTREE-011 | `btree_update_existing` | ✅ PASS |
| TC-BTREE-012 | `btree_find_nonexistent` | ✅ PASS |
| TC-BTREE-013 | `btree_cursor_prev` | ✅ PASS |
| TC-BTREE-014 | `btree_multiple_inserts` | ✅ PASS |
| TC-BTREE-015 | `btree_page_split` | ✅ PASS |
| TC-BTREE-016 | `btree_verify_empty` | ✅ PASS |
| TC-BTREE-017 | `btree_verify_with_data` | ✅ PASS |

**通过率: 17/17 (100%)**