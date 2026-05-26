# TinyDB v2 - B+tree Debug 任务列表

## 问题
B+tree page split导致SELECT挂起

## 已修复 (5个bug)

| # | 问题 | 位置 | 修复 |
|---|------|------|------|
| 1 | cells_to_move=0当original_count=1 | 634行 | `(n+1)/2` + fallback=1 |
| 2 | content_start未重算 | 638行 | 设置为第一个cell指针 |
| 3 | 移动指针无边界检查 | 647行 | 加payload_size+8校验 |
| 4 | new_ptr下溢 | 647行 | 检查 `payload_size+8 > new_content_start` |
| 5 | memcpy无边界验证 | 654行 | 检查 `new_ptr + size <= PAGE_SIZE` |

## 当前状态
- 编译: 通过
- 单元测试: 通过
- **但SELECT仍然超时**

## 待调查根因

### 1. 页面分裂后right_sibling链断裂
**检查点**:
- 新页right_sibling = 原页旧sibling (行670)
- 原页right_sibling = new_page_num (行733)
- 最后一页right_sibling = 0

### 2. btree_first()可能返回无效cursor
**检查点**:
- 根页面类型判断(is_leaf vs is_internal)
- 下降到最左leaf的逻辑

### 3. btree_cursor_next()跨页失败
**检查点**:
- 行491: `get_right_sibling(page->data)`
- 行494-498: sibling!=0时正确加载新页
- 行502: sibling=0时正确标记is_end=1

### 4. server.c SELECT扫描死循环
**检查点**:
- 行256-263: while循环可能不退出
- `btree_cursor_valid()`可能始终返回1

## 调试命令
```bash
# 编译并运行
make clean && make
./tinydb_server &
nc -U /run/tinydb/tinydb.sock

QUERY:CREATE TABLE t (id INT)
QUERY:INSERT INTO t VALUES (1)
QUERY:INSERT INTO t VALUES (2)
QUERY:SELECT * FROM t
```

## 文件位置
- 主文件: `/mnt/c/sml/project/c_project/tinydbv2/src/storage/btree.c`
- 头文件: `/mnt/c/sml/project/c_project/tinydbv2/src/storage/btree.h`
- 服务端: `/mnt/c/sml/project/c_project/tinydbv2/src/server/server.c`

## 继续调查步骤
1. 加debug日志到page_split函数，检查right_sibling设置
2. 加debug日志到btree_cursor_next，检查跨页逻辑
3. 加debug日志到btree_first，检查是否正确找到最左叶
4. 加debug日志到server.c SELECT循环，检查死循环位置

---
创建时间: 2026-05-22
状态: 调查中