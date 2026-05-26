# WAL (Write-Ahead Logging) 测试用例

## 测试范围

测试 `src/storage/wal.c` 的预写日志功能。

## 实际测试用例 (基于 test-wal.c)

### TC-WAL-001: WAL 创建

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-WAL-001 |
| **标题** | WAL 创建 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `wal_open()` 创建新的 WAL 文件 |
| **预期结果** | WAL 创建成功 |
| **实际测试** | `test(wal_creation)` |
| **测试结果** | ✅ PASS |

---

### TC-WAL-002: WAL 路径生成

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-WAL-002 |
| **标题** | WAL 路径生成 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `wal_get_path("test.db")` |
| **预期结果** | 返回 "test.db-wal" 路径 |
| **实际测试** | `test(wal_path_generation)` |
| **测试结果** | ✅ PASS |

---

### TC-WAL-003: WAL 校验和计算

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-WAL-003 |
| **标题** | WAL 校验和计算 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `wal_checksum()` 计算数据校验和 |
| **预期结果** | 返回正确的 CRC32 校验和 |
| **实际测试** | `test(wal_checksum)` |
| **测试结果** | ✅ PASS |

---

### TC-WAL-004: WAL 条目追加

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-WAL-004 |
| **标题** | WAL 条目追加 |
| **优先级** | P0 |
| **前置条件** | WAL 已创建 |
| **测试步骤** | 1. 调用 `wal_begin_tx()` 开始事务 <br> 2. 调用 `wal_append()` 追加条目 |
| **预期结果** | 条目成功追加到 WAL |
| **实际测试** | `test(wal_entry_append)` |
| **测试结果** | ✅ PASS |

---

### TC-WAL-005: WAL 页面写入

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-WAL-005 |
| **标题** | WAL 页面写入 |
| **优先级** | P0 |
| **前置条件** | 已开始事务 |
| **测试步骤** | 1. 调用 `wal_begin_tx()` <br> 2. 调用 `wal_write_page()` 写入页面数据 |
| **预期结果** | 页面修改记录成功写入 |
| **实际测试** | `test(wal_page_write)` |
| **测试结果** | ✅ PASS |

---

### TC-WAL-006: WAL 刷新

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-WAL-006 |
| **标题** | WAL 刷新 |
| **优先级** | P1 |
| **前置条件** | WAL 已创建 |
| **测试步骤** | 1. 写入条目后调用 `wal_flush()` |
| **预期结果** | WAL 数据刷新到磁盘 |
| **实际测试** | `test(wal_flush)` |
| **测试结果** | ✅ PASS |

---

### TC-WAL-007: WAL 重新打开

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-WAL-007 |
| **标题** | WAL 重新打开 |
| **优先级** | P1 |
| **前置条件** | WAL 已创建并写入数据 |
| **测试步骤** | 1. 关闭 WAL <br> 2. 重新打开同一个 WAL |
| **预期结果** | WAL 重新打开成功 |
| **实际测试** | `test(wal_reopen)` |
| **测试结果** | ✅ PASS |

---

### TC-WAL-008: WAL 基本回放

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-WAL-008 |
| **标题** | WAL 基本回放 |
| **优先级** | P0 |
| **前置条件** | WAL 已写入数据 |
| **测试步骤** | 1. 写入事务条目 <br> 2. 调用 `wal_replay()` 重放 |
| **预期结果** | 事务正确回放 |
| **实际测试** | `test(wal_replay_basic)` |
| **测试结果** | ✅ PASS |

---

### TC-WAL-009: WAL 完全检查点

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-WAL-009 |
| **标题** | WAL 完全检查点 |
| **优先级** | P1 |
| **前置条件** | 已写入多个事务 |
| **测试步骤** | 1. 执行多个事务 <br> 2. 调用 `wal_checkpoint_full()` |
| **预期结果** | 检查点完成 |
| **实际测试** | `test(wal_checkpoint_full_basic)` |
| **测试结果** | ✅ PASS |

---

### TC-WAL-010: WAL 多页面回放

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-WAL-010 |
| **标题** | WAL 多页面回放 |
| **优先级** | P1 |
| **前置条件** | 已写入多个页面 |
| **测试步骤** | 1. 写入多个页面的修改 <br> 2. 调用 `wal_replay()` |
| **预期结果** | 所有页面正确回放 |
| **实际测试** | `test(wal_replay_multiple_pages)` |
| **测试结果** | ✅ PASS |

---

## 测试结果汇总

| 测试用例 | 实际测试函数 | 结果 |
|---------|-------------|------|
| TC-WAL-001 | `wal_creation` | ✅ PASS |
| TC-WAL-002 | `wal_path_generation` | ✅ PASS |
| TC-WAL-003 | `wal_checksum` | ✅ PASS |
| TC-WAL-004 | `wal_entry_append` | ✅ PASS |
| TC-WAL-005 | `wal_page_write` | ✅ PASS |
| TC-WAL-006 | `wal_flush` | ✅ PASS |
| TC-WAL-007 | `wal_reopen` | ✅ PASS |
| TC-WAL-008 | `wal_replay_basic` | ✅ PASS |
| TC-WAL-009 | `wal_checkpoint_full_basic` | ✅ PASS |
| TC-WAL-010 | `wal_replay_multiple_pages` | ✅ PASS |

**通过率: 10/10 (100%)**