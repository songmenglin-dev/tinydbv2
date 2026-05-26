# SQL Lexer 测试用例

## 测试范围

测试 `src/sql/lexer.c` 的词法分析功能。

## 实际测试用例 (基于 test_lexer.c)

### TC-LEX-001: Lexer 创建和销毁

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-LEX-001 |
| **标题** | Lexer 创建和销毁 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `lexer_create("SELECT * FROM users", strlen(sql))` <br> 2. 调用 `lexer_destroy(lex)` |
| **预期结果** | Lexer 创建成功，销毁后无内存泄漏 |
| **实际测试** | `test(lexer_create_destroy)` |
| **测试结果** | ✅ PASS |

---

### TC-LEX-002: 简单 SELECT 语句分词

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-LEX-002 |
| **标题** | 简单 SELECT 语句分词 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 lexer，传入 `"SELECT * FROM users"` <br> 2. 依次获取 token 序列并验证类型 |
| **预期结果** | `TOKEN_SELECT`, `TOKEN_STAR`, `TOKEN_FROM`, `TOKEN_IDENTIFIER("users")`, `TOKEN_EOF` |
| **实际测试** | `test(lexer_simple_select)` |
| **测试结果** | ✅ PASS |

---

### TC-LEX-003: 整数和十六进制 tokenization

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-LEX-003 |
| **标题** | 整数和十六进制 tokenization |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 lexer，传入 `"123 456 0xFF"` <br> 2. 获取前3个 token |
| **预期结果** | `TOKEN_INTEGER(123)`, `TOKEN_INTEGER(456)`, `TOKEN_INTEGER(255)` |
| **实际测试** | `test(lexer_integer_tokens)` |
| **测试结果** | ✅ PASS |

---

### TC-LEX-004: 浮点数 tokenization

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-LEX-004 |
| **标题** | 浮点数 tokenization |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 lexer，传入 `"3.14 1.5e-3"` <br> 2. 获取两个 token |
| **预期结果** | `TOKEN_REAL` (3.14), `TOKEN_REAL` (1.5e-3) |
| **实际测试** | `test(lexer_real_tokens)` |
| **测试结果** | ✅ PASS |

---

### TC-LEX-005: 字符串 tokenization

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-LEX-005 |
| **标题** | 字符串 tokenization |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 lexer，传入 `"'hello world'"` <br> 2. 获取 token |
| **预期结果** | `TOKEN_STRING`，长度为 11 |
| **实际测试** | `test(lexer_string_token)` |
| **测试结果** | ✅ PASS |

---

### TC-LEX-006: 转义字符串

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-LEX-006 |
| **标题** | 转义字符串 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 lexer，传入 `"'hello''world'"` <br> 2. 获取 token |
| **预期结果** | `TOKEN_STRING`，值为 `"hello'world"`，长度为 11 |
| **实际测试** | `test(lexer_string_escape)` |
| **测试结果** | ✅ PASS |

---

### TC-LEX-007: 操作符识别

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-LEX-007 |
| **标题** | 操作符识别 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 lexer，传入 `"= != <> < > <= >="` <br> 2. 依次获取所有 token |
| **预期结果** | `TOKEN_EQ`, `TOKEN_NEQ`, `TOKEN_NEQ`, `TOKEN_LT`, `TOKEN_GT`, `TOKEN_LTE`, `TOKEN_GTE` |
| **实际测试** | `test(lexer_operators)` |
| **测试结果** | ✅ PASS |

---

### TC-LEX-008: 关键字大小写不敏感

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-LEX-008 |
| **标题** | 关键字大小写不敏感 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 lexer，传入 `"select SELECT Select"` <br> 2. 获取所有 token |
| **预期结果** | 所有变体都识别为 `TOKEN_SELECT` |
| **实际测试** | `test(lexer_keywords_case_insensitive)` |
| **测试结果** | ✅ PASS |

---

### TC-LEX-009: 标点符号

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-LEX-009 |
| **标题** | 标点符号 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 lexer，传入 `"( ), ;"` <br> 2. 依次获取 token |
| **预期结果** | `TOKEN_LPAREN`, `TOKEN_RPAREN`, `TOKEN_COMMA`, `TOKEN_SEMICOLON` |
| **实际测试** | `test(lexer_punctuation)` |
| **测试结果** | ✅ PASS |

---

### TC-LEX-010: 复杂 SQL 语句

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-LEX-010 |
| **标题** | 复杂 SQL 语句 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 lexer，传入完整 SQL 语句 <br> 2. 验证每个 token 类型和值 |
| **预期结果** | 所有 token 正确识别 |
| **实际测试** | `test(lexer_complex_sql)` |
| **测试结果** | ✅ PASS |

---

### TC-LEX-011: Peek token (向前查看)

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-LEX-011 |
| **标题** | Peek token (向前查看) |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 lexer，传入 `"SELECT * FROM t"` <br> 2. 调用 `lexer_peek_token()` <br> 3. 调用 `lexer_next_token()` |
| **预期结果** | peek 不消耗 token，后续 advance 正常消耗 |
| **实际测试** | `test(lexer_peek)` |
| **测试结果** | ✅ PASS |

---

### TC-LEX-012: Positional placeholders

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-LEX-012 |
| **标题** | Positional placeholders |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 lexer，传入 `"$1 $2 $123"` <br> 2. 获取三个 token |
| **预期结果** | 三个 `TOKEN_PLACEHOLDER` |
| **实际测试** | `test(lexer_placeholder)` |
| **测试结果** | ✅ PASS |

---

## 测试结果汇总

| 测试用例 | 实际测试函数 | 结果 | 备注 |
|---------|-------------|------|------|
| TC-LEX-001 | `lexer_create_destroy` | ✅ PASS | |
| TC-LEX-002 | `lexer_simple_select` | ✅ PASS | |
| TC-LEX-003 | `lexer_integer_tokens` | ✅ PASS | |
| TC-LEX-004 | `lexer_real_tokens` | ✅ PASS | |
| TC-LEX-005 | `lexer_string_token` | ✅ PASS | |
| TC-LEX-006 | `lexer_string_escape` | ✅ PASS | |
| TC-LEX-007 | `lexer_operators` | ✅ PASS | |
| TC-LEX-008 | `lexer_keywords_case_insensitive` | ✅ PASS | |
| TC-LEX-009 | `lexer_punctuation` | ✅ PASS | |
| TC-LEX-010 | `lexer_complex_sql` | ✅ PASS | |
| TC-LEX-011 | `lexer_peek` | ✅ PASS | |
| TC-LEX-012 | `lexer_placeholder` | ✅ PASS | |

**通过率: 12/12 (100%)**

---

## 测试运行记录

```
$ make test
  lexer_basic_tokens... OK
  lexer_integer_tokens... OK
  lexer_real_tokens... OK
  lexer_string_tokens... OK
  lexer_operators... OK
  lexer_punctuation... OK
  lexer_keywords... OK
  lexer_identifiers... OK
  lexer_line_column... OK
  lexer_error... OK
  lexer_peek... OK
```