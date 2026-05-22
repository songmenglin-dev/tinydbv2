# TinyDB v2 测试报告

**生成日期:** 2026/05/22
**测试团队:** tinydb-test-team
**成员:** security-reviewer, performance-engineer, test-automator, code-reviewer

---

## 一、安全审计报告 (security-reviewer) - 17 issues

### CRITICAL (3)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| S1 | 命令注入 | `cli.c:519` | `.shell` 命令执行任意shell命令，未过滤用户输入 |
| S2 | 缓冲区溢出 | `executor.c:314` | unbounded `strcpy` 到1024字节栈缓冲区 |
| S3 | 无认证 | `server.c` | 任何客户端可发送 SHUTDOWN/QUERY 命令 |

### HIGH (5)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| S4 | Unsafe strcpy | `server.c:145,99` | 潜在溢出 |
| S5 | 整数溢出 | `btree.c:604` | cell_size计算可能溢出 |
| S6 | 缓冲区溢出 | `btree.c:649` | uint32_t vs uint16_t 比较问题 |
| S7 | realloc泄漏 | `executor.c:65-66` | 失败时原始指针丢失 |
| S8 | 权限过宽 | `/run/tinydb` | 0755应为0700 |

### MEDIUM (5)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| S9 | rowid重复 | `executor.c:296` | 静态counter重启后重置，可能重复 |
| S10 | 名称无验证 | - | table/column名未拒绝特殊字符 |
| S11 | 内存泄漏 | `parser.c:774,783,792` | parser错误路径泄漏 |
| S12 | 缺失错误处理 | `btree.c:740-750` | 页分裂后btree_insert错误处理 |
| S13 | 无边界检查 | `executor.c:199-218` | snprintf SQL构造无边界检查 |

### LOW (4)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| S14 | 缓冲区不足 | SQL buffer 512字节 | 复杂表可能截断 |
| S15 | 无超时 | server | 无服务端查询超时 |
| S16 | 路径遍历 | `cli.c:526` | `.read`命令路径遍历 |
| S17 | SIGPIPE | - | 信号处理不完整 |

---

## 二、性能分析报告 (performance-engineer) - 8 issues

### Critical (3)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| P1 | O(n)线性搜索 | `btree.c:287` | `btree_find()`应为二分查找 |
| P2 | mutex持有阻塞I/O | `page_cache.c:370` | `page_cache_flush()`持有mutex期间做阻塞I/O |
| P3 | btree_delete未实现 | `btree.c:780` | 返回-1错误码，btree_update()只调用insert |

### High Priority (3)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| P4 | RWLock被注释 | `page_cache.c:262` | 读者互相阻塞 |
| P5 | rowid非线程安全 | `executor.c:296` | 静态counter，非线程安全，不持久化 |
| P6 | 单线程服务器 | `server.c:364` | 长查询阻塞新连接 |

### Medium (2)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| P7 | O(n²)拼接 | `executor.c:649` | 结果缓冲区字符串拼接 |
| P8 | N+1查询 | `executor.c:278` | 每查询做catalog查找 |

---

## 三、测试规划报告 (test-automator)

### 当前测试覆盖

**已有覆盖:**
- String utilities (len, dup, eq)
- Error handling (code_to_string, create)
- List operations (create, append, get)
- Lexer: 基本token、整数、实数、字符串、运算符、关键字、标识符
- Parser: 简单SELECT、CREATE TABLE、事务(BEGIN/COMMIT)、WHERE表达式

### 关键缺口

#### 1. SQL DML操作 (HIGH PRIORITY)

**INSERT测试缺件:**
- `INSERT INTO table VALUES (...)` - 多值插入
- `INSERT INTO table (col1, col2) VALUES (...)` - 列名指定
- `INSERT with duplicate rowid` - 碰撞处理
- `INSERT with mismatched column/value count` - 错误处理

**SELECT测试缺件:**
- `SELECT * FROM table` - 全表扫描
- `SELECT with WHERE multiple conditions (AND/OR)`
- `SELECT with ORDER BY` - executor.c:434 TODO
- `SELECT with LIMIT/OFFSET`
- `SELECT with DISTINCT`
- 空表SELECT

**UPDATE/DELETE:** 目前是stub，需要实现后才能测试

#### 2. B+tree存储 (CRITICAL - 无单元测试)

**需要测试场景:**
- `btree_create()` - 验证初始空叶页
- `btree_insert()` - 单插入、多插入
- `btree_find()` - 存在/不存在key
- `btree_first()/btree_last()` - 迭代器边界
- `btree_cursor_next()` - 跨页遍历
- 页分裂测试 - 插入导致叶页溢出
- 分裂后50/50分布验证
- 右兄弟链接正确性
- 多连续分裂（树增高）
- 边界条件: 空树、单cell、满页插入、最大key、零长度value

#### 3. Parser鲁棒性 (MEDIUM-HIGH)

**畸形SQL测试:**
- 未闭合括号: `SELECT * FROM users (`
- 缺失表名: `SELECT * FROM`
- 缺失values: `INSERT INTO t VALUES`
- 多余逗号: `SELECT a, FROM users`
- 无效关键字: `SELECK * FROM users`
- 空语句: `;`
- SQL注入尝试: `' OR '1'='1`

#### 4. 集成测试建议

**完整CRUD周期:**
```
CREATE TABLE t (id INT, name TEXT);
INSERT INTO t VALUES (1, 'alice');
INSERT INTO t VALUES (2, 'bob');
SELECT * FROM t;  -- 2 rows
UPDATE t SET name='charlie' WHERE id=1;
SELECT * FROM t WHERE id=1;  -- 'charlie'
DELETE FROM t WHERE id=2;
SELECT * FROM t;  -- 1 row
```

#### 5. CLI命令测试

- `.help`, `.quit`, `.tables`, `.schema`
- `.mode <mode>`, `.headers`, `.null`, `.timer`
- `.shell <cmd>` - 安全风险见S1
- CLI参数: `-c`, `-f`, `-s`, `--help`

### 测试框架

已有测试框架(mini_test.h)，包含:
- `test()`, `assert_eq()`, `assert_true()`, `assert_false()`
- `assert_null()`, `assert_non_null()`, `assert_str_eq()`

---

## 四、代码质量审查报告 (code-reviewer) - 19 issues

### CRITICAL (4)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| C1 | 内存泄漏 | `executor.c:324-327` | btree_insert失败时table_entry未释放 |
| C2 | rowid非线程安全 | `executor.c:296` | 静态counter，重启重置，非线程安全 |
| C3 | 死代码 | `parser.c:529-530` | 第二条parser_set_error和return不可达 |
| C4 | Debug语句 | `server.c:232-341` | ~30条fprintf(stderr)应移除或改用日志 |

### HIGH (5)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| C5 | 函数过长 | `btree.c:583-778` | btree_insert约195行，应拆分 |
| C6 | 内存泄漏 | `parser.c:855-858,908-910,964-965` | 多处错误路径泄漏 |
| C7 | 潜在泄漏 | `executor.c:587-665` | select_result_buf早期return跳过free |
| C8 | Unbounded strcpy | `cli.c:60` | 无边界检查 |
| C9 | buf复用 | `server.c:181,232,280` | buffer被调试消息覆盖 |

### MEDIUM (5)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| C10 | btree_delete stub | `btree.c:780-785` | 返回-1而非SUCCESS |
| C11 | 注释缺失 | `btree.c:787-789` | btree_update行为未解释 |
| C12 | 函数过长 | `cli.c:84-201` | cli_execute_sql约117行 |
| C13 | 函数过长 | `cli.c:444-535` | cli_handle_meta_command约91行 |
| C14 | 错误响应不一致 | `server.c` | 各种错误格式不统一 |

### LOW (5)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| C15 | 重复标准库 | `string.c` | str_eq/str_ne就是strcmp |
| C16 | 缺失NULL检查 | `btree.c:268-272` | btree_find的NULL返回值未被检查 |
| C17 | 空stub | `executor.c:434-439` | select_apply_order_by是空实现 |
| C18 | xmalloc行为 | `executor.c:19-26` | OOM时exit()而非返回NULL |
| C19 | 注释风格不一致 | parser.c | 函数注释缺失 |

### 超长函数 (>100行)

| 文件 | 函数 | 行数 |
|------|------|------|
| parser.c | parse_binary_expr | ~145 |
| btree.c | btree_insert | ~195 |
| server.c | server_handle_client | ~170 |
| cli.c | cli_execute_sql | ~117 |
| cli.c | cli_handle_meta_command | ~91 |
| parser.c | parse_select | ~108 |
| executor.c | executor_exec_select | ~123 |

---

## 五、修复优先级

### 第一批 (立即修复)

| 优先级 | Issue | 问题 |
|--------|-------|------|
| P0 | S2, C1 | executor.c:314 strcpy溢出, table_entry泄漏 |
| P0 | S1 | cli.c:519 shell命令注入 |
| P0 | S3 | server.c无认证 |
| P0 | C2, P5 | rowid非线程安全/不持久化 |

### 第二批 (近期修复)

| 优先级 | Issue | 问题 |
|--------|-------|------|
| P1 | P1 | btree.c:287改为二分查找 |
| P1 | P3 | btree_delete()实现 |
| P1 | C5 | btree_insert拆分 |
| P1 | P2 | page_cache mutex I/O问题 |
| P1 | C7 | select_result_buf泄漏 |
| P1 | P4 | page_cache RWLock |

### 第三批 (测试完善)

| 优先级 | Issue | 问题 | 状态 |
|--------|-------|------|------|
| P2 | T-1 | 添加B+tree单元测试 | ✅ 已完成 |
| P2 | T-2 | 添加INSERT/SELECT完整测试 | ⏳ 待完成 |
| P2 | T-3 | Parser鲁棒性测试 | ⏳ 待完成 |
| P2 | T-4 | UPDATE/DELETE实现后测试 | ⏳ 待完成 |

---

## 六、文件索引

| 文件 | 描述 |
|------|------|
| `src/cli/cli.c` | CLI接口 - S1,S8,C4,C8,C12,C13 |
| `src/cli/cli.h` | CLI头文件 |
| `src/server/server.c` | 服务器 - S3,S4,C4,C9,P6 |
| `src/sql/executor.c` | 查询执行器 - S2,S7,S9,S13,C1,C2,C7,P5,P7,P8 |
| `src/sql/parser.c` | SQL解析器 - S10,S11,C3,C6,C19 |
| `src/storage/btree.c` | B+tree存储 - S5,S6,P1,P3,C5,C10,C11 |
| `src/storage/btree.h` | B+tree头文件 |
| `src/storage/page_cache.c` | 页缓存 - P2,P4 |
| `src/util/string.c` | 字符串工具 - C15 |
| `tests/unit/test-suite.c` | 单元测试 - 需扩展 |

---

*报告生成: 2026/05/22 by tinydb-test-team*