# Pager 测试用例

## 测试范围

测试 `src/storage/pager.c` 的页面管理功能，包括创建/关闭、页面分配、读写、释放与复用、头部操作、magic 验证、页面偏移计算等。

## 实际测试用例 (基于 test-pager.c)

### TC-PAGER-001: Pager 创建和关闭

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAGER-001 |
| **标题** | Pager 创建和关闭 |
| **优先级** | P0 |
| **实际测试** | `test(pager_create_and_close)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `pager_create(path)` 创建 Pager 实例 2. 验证返回非 NULL 3. 调用 `pager_close()` 关闭并清理临时文件 |
| **预期结果** | Pager 创建成功，关闭后临时文件被删除 |
| **测试结果** | PASS |

---

### TC-PAGER-002: Pager 创建和重新打开

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAGER-002 |
| **标题** | Pager 创建和重新打开 |
| **优先级** | P0 |
| **实际测试** | `test(pager_create_and_open)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `pager_create(path)` 创建 Pager 并关闭 2. 调用 `pager_open(path)` 重新打开同一文件 3. 验证两次打开均返回非 NULL |
| **预期结果** | 已关闭的数据库文件可通过 `pager_open` 重新打开 |
| **测试结果** | PASS |

---

### TC-PAGER-003: 页面分配

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAGER-003 |
| **标题** | 页面分配 |
| **优先级** | P0 |
| **实际测试** | `test(pager_allocate_pages)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 Pager 2. 连续分配三个页面 3. 验证页面编号依次为 1, 2, 3（跳过头部页 0） |
| **预期结果** | 分配的页面编号连续递增，首个分配页从 1 开始 |
| **测试结果** | PASS |

---

### TC-PAGER-004: 页面读写

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAGER-004 |
| **标题** | 页面读写 |
| **优先级** | P0 |
| **实际测试** | `test(pager_read_write_page)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 Pager 并分配一个页面 2. 向页面写入 PAGE_SIZE 字节的自定义数据 3. 读取页面内容 4. 逐字节验证数据一致性 |
| **预期结果** | 写入的数据与读取的数据完全一致 |
| **测试结果** | PASS |

---

### TC-PAGER-005: 页面释放与复用

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAGER-005 |
| **标题** | 页面释放与复用 |
| **优先级** | P1 |
| **实际测试** | `test(pager_free_and_reuse_pages)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 Pager 并分配三个页面 (page1, page2, page3) 2. 释放 page2 3. 再次分配新页面 page4 4. 验证 page4 编号等于 page2 |
| **预期结果** | 释放的页面被加入空闲列表，下次分配时优先复用 |
| **测试结果** | PASS |

---

### TC-PAGER-006: 头部操作与统计

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAGER-006 |
| **标题** | 头部操作与统计 |
| **优先级** | P1 |
| **实际测试** | `test(pager_header_operations)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 Pager 2. 调用 `pager_validate_magic()` 验证 magic 值 3. 调用 `pager_stats()` 获取统计信息 4. 验证 page_count=1 (仅头部页)、first_free_page=0、free_page_count=0 |
| **预期结果** | Magic 验证通过，初始统计信息正确 |
| **测试结果** | PASS |

---

### TC-PAGER-007: Magic 验证

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAGER-007 |
| **标题** | Magic 验证 |
| **优先级** | P1 |
| **实际测试** | `test(pager_validate_magic)` |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 Pager 2. 调用 `pager_validate_magic()` |
| **预期结果** | 新创建的 Pager magic 值验证通过，返回 0 |
| **测试结果** | PASS |

---

### TC-PAGER-008: 页面偏移计算

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-PAGER-008 |
| **标题** | 页面偏移计算 |
| **优先级** | P1 |
| **实际测试** | `test(pager_page_offset)` |
| **前置条件** | 无 |
| **测试步骤** | 调用 `pager_get_page_offset()` 验证以下计算: offset(0)=0, offset(1)=PAGE_SIZE, offset(2)=PAGE_SIZE*2, offset(100)=PAGE_SIZE*100 |
| **预期结果** | 页面偏移计算公式正确: offset = page_num * PAGE_SIZE |
| **测试结果** | PASS |

---

## 测试结果汇总

| 测试用例 | 实际测试函数 | 结果 |
|---------|-------------|------|
| TC-PAGER-001 | `pager_create_and_close` | PASS |
| TC-PAGER-002 | `pager_create_and_open` | PASS |
| TC-PAGER-003 | `pager_allocate_pages` | PASS |
| TC-PAGER-004 | `pager_read_write_page` | PASS |
| TC-PAGER-005 | `pager_free_and_reuse_pages` | PASS |
| TC-PAGER-006 | `pager_header_operations` | PASS |
| TC-PAGER-007 | `pager_validate_magic` | PASS |
| TC-PAGER-008 | `pager_page_offset` | PASS |

**通过率: 8/8 (100%)**

## 相关文件

- 测试代码: `tests/unit/test-pager.c`
- 被测代码: `src/storage/pager.c`
- 测试入口: `tests/unit/test-suite.c`