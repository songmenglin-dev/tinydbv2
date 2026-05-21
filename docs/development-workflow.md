# Development Workflow

This project uses a structured development workflow based on openspec and superpowers.

## Workflow Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           Development Workflow                           │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  1. SPECS   ──►  2. BRAINSTORM   ──►  3. DESIGN   ──►  4. TASKS        │
│       │               │                  │                │              │
│       │               │                  │                │              │
│       ▼               ▼                  ▼                ▼              │
│  Write specs      Challenge &         Detailed         Break into      │
│  via openspec     refine via          design via        small tasks     │
│                   brainstorming       superpowers                         │
│                                        │                                 │
│                                        ▼                                 │
│  6. IMPLEMENT  ◄──  5. ASSIGN AGENTS  (via subagents/teams)            │
│       │                                                               │
│       ▼                                                               │
│  7. VALIDATE & COMMIT                                               │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## Phase Descriptions

### Phase 1: Write Specs (openspec)
- Use `opsx:propose` to create initial change proposal
- Use `opsx:explore` to understand existing specs in `openspec/specs/`
- Write detailed specs in `openspec/changes/<change>/specs/**/*.md`
- Each capability gets its own `spec.md` file with requirements and scenarios

### Phase 2: Brainstorm & Challenge (superpowers:brainstorming)
- Invoke the `brainstorming` skill to critically review specs
- Challenge assumptions, identify gaps, question design decisions
- Refine specs based on brainstorming outcomes
- Iterate until specs are robust

### Phase 3: Detailed Design (superpowers:writing-plans)
- After specs are approved, use `writing-plans` skill for detailed design
- Design should cover: architecture, data structures, algorithms, interfaces
- Reference specs for requirements, design for implementation approach

### Phase 4: Task Breakdown
- Break design into small, trackable tasks in `tasks.md`
- Each task should be verifiable (you know when it's done)
- Order tasks by dependency
- Use checkbox format: `- [ ] 1.1 Task description`

### Phase 5: Agent Assignment (subagent-driven-development)
- Use `subagent-driven-development` skill to orchestrate agents
- Or use `agent-teams:*` skills for team-based development

## Agent Selection

### Hierarchy
When assigning work, search for agents in this order:
1. **Project-level agents** (`.claude/agents/`) - Project-specific implementations
2. **User-level agents** (`~/.claude/agents/`) - User's personal agent configurations

### Role-Based Assignment

| Task Type | Preferred Agents | Fallback Agents |
|-----------|------------------|-----------------|
| **Architecture/System Design** | `architect`, `engineering-software-architect`, `engineering-backend-architect` | `code-architect` |
| **Feature Planning** | `planner`, `engineering-rapid-prototyper` | `engineering-senior-developer` |
| **Core Development** | `engineering-senior-developer`, `engineering-backend-architect` | `planner` |
| **Code Review** | `code-reviewer`, `engineering-code-reviewer` | `engineering-senior-developer` |
| **Testing** | `e2e-runner`, `pr-test-analyzer` | `engineering-senior-developer` |
| **Performance** | `performance-optimizer`, `engineering-database-optimizer` | `engineering-senior-developer` |
| **Security** | `security-reviewer`, `engineering-security-engineer` | `engineering-senior-developer` |
| **Debugging** | `build-error-resolver`, `silent-failure-hunter` | `engineering-senior-developer` |
| **Documentation** | `doc-updater`, `engineering-technical-writer`, `documentation-writer` | `engineering-senior-developer` |
| **DevOps/Deployment** | `engineering-devops-automator`, `engineering-sre` | `engineering-senior-developer` |
| **Refactoring** | `refactor-cleaner`, `engineering-minimal-change-engineer` | `engineering-senior-developer` |

### Team Composition

For complex features, compose a team with multiple specialized agents:

```
Team Lead (planner/architect)
├── Dev Agent (engineering-senior-developer) - Implementation
├── Review Agent (code-reviewer) - Code quality & patterns
├── Test Agent (e2e-runner) - Testing strategy
├── Security Agent (security-reviewer) - Security review
└── Docs Agent (doc-updater) - Documentation
```

## Execution Commands

| Phase | Command | Skill/Agent |
|-------|---------|------------|
| Create change | `/opsx:propose <name>` | openspec-propose |
| Explore specs | `/opsx:explore` | openspec-explore |
| Apply change | `/opsx:apply` | openspec-apply-change |
| Brainstorm | Use `brainstorming` skill | brainstorming |
| Detailed design | Use `writing-plans` skill | writing-plans |
| Subagent dev | Use `subagent-driven-development` skill | subagent-driven-development |
| Team dev | Use `agent-teams:team-feature` | agent-teams |

## Quality Gates

### Before Implementation
- [ ] Specs reviewed and approved via brainstorming
- [ ] Design reviewed by architecture agent
- [ ] Tasks are atomic and independently verifiable
- [ ] Agent assignments match task types

### Before Committing
- [ ] All tests pass
- [ ] Code review completed (critical/high issues resolved)
- [ ] Security review for sensitive components
- [ ] Documentation updated