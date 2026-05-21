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

**Agent role matching:** Architecture → `architect`, Dev → `engineering-senior-developer`, Review → `code-reviewer`, Test → `e2e-runner`, Security → `security-reviewer`, Docs → `doc-updater`