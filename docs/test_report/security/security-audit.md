# TinyDB v2 安全审计报告

**生成日期:** 2026/05/22
**审计团队:** security-reviewer

---

## 审计结果汇总

| 严重级别 | 数量 |
|---------|------|
| CRITICAL | 3 |
| HIGH | 5 |
| MEDIUM | 5 |
| LOW | 4 |
| **总计** | **17** |

---

## CRITICAL (3)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| S1 | 命令注入 | `cli.c:519` | `.shell` 命令执行任意shell命令，未过滤用户输入 |
| S2 | 缓冲区溢出 | `executor.c:314` | unbounded `strcpy` 到1024字节栈缓冲区 |
| S3 | 无认证 | `server.c` | 任何客户端可发送 SHUTDOWN/QUERY 命令 |

---

## HIGH (5)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| S4 | Unsafe strcpy | `server.c:145,99` | 潜在溢出 |
| S5 | 整数溢出 | `btree.c:604` | cell_size计算可能溢出 |
| S6 | 缓冲区溢出 | `btree.c:649` | uint32_t vs uint16_t 比较问题 |
| S7 | realloc泄漏 | `executor.c:65-66` | 失败时原始指针丢失 |
| S8 | 权限过宽 | `/run/tinydb` | 0755应为0700 |

---

## MEDIUM (5)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| S9 | rowid重复 | `executor.c:296` | 静态counter重启后重置，可能重复 |
| S10 | 名称无验证 | - | table/column名未拒绝特殊字符 |
| S11 | 内存泄漏 | `parser.c:774,783,792` | parser错误路径泄漏 |
| S12 | 缺失错误处理 | `btree.c:740-750` | 页分裂后btree_insert错误处理 |
| S13 | 无边界检查 | `executor.c:199-218` | snprintf SQL构造无边界检查 |

---

## LOW (4)

| # | 问题 | 位置 | 描述 |
|---|------|------|------|
| S14 | 缓冲区不足 | SQL buffer 512字节 | 复杂表可能截断 |
| S15 | 无超时 | server | 无服务端查询超时 |
| S16 | 路径遍历 | `cli.c:526` | `.read`命令路径遍历 |
| S17 | SIGPIPE | - | 信号处理不完整 |

---

## 修复优先级

### P0 (立即修复)

| Issue | 问题 |
|-------|------|
| S1 | cli.c:519 shell命令注入 |
| S2 | executor.c:314 strcpy溢出 |
| S3 | server.c无认证 |

### P1 (近期修复)

| Issue | 问题 |
|-------|------|
| S4 | server.c unsafe strcpy |
| S5 | btree.c 整数溢出 |
| S6 | btree.c 缓冲区问题 |
| S7 | executor.c realloc泄漏 |
| S8 | 权限过宽 |