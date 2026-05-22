# TinyDB v2

A lightweight SQL database written in C, similar to SQLite but with client-server architecture.

**Core Features:** ACID transactions, WAL, B+tree storage, CLI client, systemd deployment
**Key Files:** `src/` (storage, parser, server, cli), `openspec/` (specs & design)

Behavioral guidelines to reduce common LLM coding mistakes. Merge with project-specific instructions as needed.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

---

**These guidelines are working if:** fewer unnecessary changes in diffs, fewer rewrites due to overcomplication, and clarifying questions come before implementation rather than after mistakes.

## 5. Spec-First Development

**Do not do anything other than the specs.** Implement exactly what is specified in the design and tasks documents. Do not add features, improvements, or changes beyond what is outlined in the specs.

## 6. Small-Step Commits

**Make small, focused commits that represent a single logical change.**

### Commit Size Guidelines
- One commit per task or logical unit of work
- If a change touches multiple files for the same purpose, it can be one commit
- If a change touches the same file for unrelated purposes, split into separate commits

### Commit Message Format
```
<type>: <short description>

<body (optional)>
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `refactor`: Code refactoring (no behavior change)
- `test`: Adding or updating tests
- `docs`: Documentation changes
- `chore`: Build process, tooling, dependencies
- `perf`: Performance improvement

### Examples
```
feat: add WAL flush and checkpoint

fix: resolve page_size attribute mismatch in FileStorage.open()

test: add integration tests for SELECT with WHERE conditions

refactor: extract _serialize_node method from BTree
```

### When to Commit
- After completing each task or subtask
- After passing all tests for a change
- Before switching to a different task
- Never commit broken code (tests must pass)

### Never
- Don't commit half-finished work
- Don't bundle unrelated changes into one commit
- Don't use vague messages like "updates" or "fixes"

---

## 7. Development Workflow

See [docs/development-workflow.md](docs/development-workflow.md) for full workflow, agent assignments, and quality gates.

**TL;DR:**
```
SPECS → BRAINSTORM (brainstorming) → DESIGN (writing-plans) → TASKS → 
  → AGENTS (subagent-driven-development) → IMPLEMENT → VALIDATE → COMMIT
```

**Agent role matching:** Architecture → `architect`, Dev → `c-pro`, Review → `code-reviewer`, Test → `e2e-runner`, Security → `security-reviewer`, Docs → `doc-updater`

---

## 8. Subagent Expert Configuration

Use specialized subagents for professional tasks instead of general-purpose.

### Expert Subagent Mapping

| Task Type        | Subagent                        | Trigger Keywords                              |
| ---------------- | ------------------------------- | --------------------------------------------- |
| C/系统开发       | `c-pro`                         | C语言, 系统编程, 存储层, 解析器, 执行器     |
| 数据库           | `engineering-database-optimizer` | SQL, 索引, 查询优化, B+tree                 |
| 安全分析         | `engineering-security-engineer` | 安全, 漏洞, 注入, 认证                       |
| 代码审查         | `engineering-code-reviewer`     | 审查, review, 检查代码                       |
| 架构设计         | `engineering-software-architect` | 架构, 设计模式, 系统设计                   |
| 性能优化         | `engineering-autonomous-optimization-architect` | 性能, 优化, 缓存               |

### Usage Rules

1. **优先使用专家**: When task matches an expert, use the corresponding subagent
2. **不要默认 general-purpose**: Don't default to general-purpose when an expert is available
3. **组合任务拆解**: Complex tasks should be split into parallel expert subtasks
4. **显式指定**: Follow user's explicit subagent preference when specified

### C Language Development (c-pro)

For TinyDB v2 C codebase:
- **Always use `c-pro`** for C language tasks
- Use for: storage layer, parser, executor, server, CLI development
- Includes: memory safety, pointer arithmetic, build fixes
- Priority: Use before general-purpose for C tasks

### Call Example

```bash
Agent({
  description: "B+tree storage development",
  prompt: "Implement btree_delete() function...",
  subagent_type: "c-pro"  # Use c-pro for C tasks
})
```