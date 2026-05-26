# Page Cache 测试用例

## 测试范围
测试 `src/storage/page_cache.c` 的页面缓存功能。

## 实际测试用例 (基于 test-page-cache.c)

### TC-CACHE-001: Cache 创建和销毁
| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CACHE-001 |
| **标题** | Cache 创建和销毁 |
| **优先级** | P0 |
| **实际测试** | `test(page_cache_create_and_destroy)` |
| **测试结果** | PASS |

**测试描述**: 创建 PageCache 实例并正确释放资源，验证 cache 对象的生命周期管理。

---

### TC-CACHE-002: 页面获取
| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CACHE-002 |
| **标题** | 页面获取 |
| **优先级** | P0 |
| **实际测试** | `test(page_cache_get_page)` |
| **测试结果** | PASS |

**测试描述**: 通过 page_pin() 获取页面，验证页面 ID 匹配和资源分配正确。

---

### TC-CACHE-003: 页面 Pin/Unpin 引用计数
| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CACHE-003 |
| **标题** | 页面 Pin/Unpin 引用计数 |
| **优先级** | P0 |
| **实际测试** | `test(page_cache_pin_unpin)` |
| **测试结果** | PASS |

**测试描述**: 测试 page_pin() 增加引用计数，page_unpin() 减少引用计数，验证 refcount 状态正确。

---

### TC-CACHE-004: 脏页标记
| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CACHE-004 |
| **标题** | 脏页标记 |
| **优先级** | P0 |
| **实际测试** | `test(page_cache_mark_dirty)` |
| **测试结果** | PASS |

**测试描述**: 调用 page_mark_dirty() 标记页面为脏，验证 is_dirty 标志从 0 变为 1。

---

### TC-CACHE-005: 全量 Flush
| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CACHE-005 |
| **标题** | 全量 Flush |
| **优先级** | P0 |
| **实际测试** | `test(page_cache_flush)` |
| **测试结果** | PASS |

**测试描述**: 将脏页 flush 到磁盘，验证 page_cache_flush() 返回正确刷新的页面数量。

---

### TC-CACHE-006: Cache 统计信息
| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CACHE-006 |
| **标题** | Cache 统计信息 |
| **优先级** | P1 |
| **实际测试** | `test(page_cache_stats)` |
| **测试结果** | PASS |

**测试描述**: 测试 page_cache_stats() 获取缓存命中率、未命中数、大小和 pinned 页数。

---

### TC-CACHE-007: 缓存命中率
| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CACHE-007 |
| **标题** | 缓存命中率 |
| **优先级** | P1 |
| **实际测试** | `test(page_cache_hit_rate)` |
| **测试结果** | PASS |

**测试描述**: 测试首次访问产生 miss，后续访问产生 hit，验证 page_cache_hit_rate() > 0.0。

---

### TC-CACHE-008: LRU 驱逐策略
| 项目 | 内容 |
|------|------|
| **用例ID** | TC-CACHE-008 |
| **标题** | LRU 驱逐策略 |
| **优先级** | P1 |
| **实际测试** | `test(page_cache_lru_eviction)` |
| **测试结果** | PASS |

**测试描述**: 创建容量为 4 的缓存但分配 5 个页面，验证 LRU 驱逐机制正常工作。

---

## 测试结果汇总
| 测试用例 | 实际测试函数 | 结果 |
|---------|-------------|------|
| TC-CACHE-001 | `page_cache_create_and_destroy` | PASS |
| TC-CACHE-002 | `page_cache_get_page` | PASS |
| TC-CACHE-003 | `page_cache_pin_unpin` | PASS |
| TC-CACHE-004 | `page_cache_mark_dirty` | PASS |
| TC-CACHE-005 | `page_cache_flush` | PASS |
| TC-CACHE-006 | `page_cache_stats` | PASS |
| TC-CACHE-007 | `page_cache_hit_rate` | PASS |
| TC-CACHE-008 | `page_cache_lru_eviction` | PASS |

**通过率: 8/8 (100%)**

---

## 实现文件
- `src/storage/page_cache.c` - 页面缓存核心实现
- `src/storage/page_cache.h` - 页面缓存接口定义
- `tests/unit/test-page-cache.c` - 页面缓存单元测试

## 关键 API
| 函数 | 说明 |
|------|------|
| `page_cache_create(max_pages, pager)` | 创建页面缓存实例 |
| `page_cache_destroy(cache)` | 销毁页面缓存并释放资源 |
| `page_pin(cache, page_id)` | 获取页面并增加引用计数 |
| `page_unpin(page)` | 减少引用计数 |
| `page_mark_dirty(page)` | 标记页面为脏 |
| `page_cache_flush(cache)` | 将所有脏页刷新到磁盘 |
| `page_cache_stats(cache, ...)` | 获取缓存统计信息 |
| `page_cache_hit_rate(cache)` | 计算缓存命中率 |