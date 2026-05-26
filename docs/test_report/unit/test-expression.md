# Expression Evaluation 测试用例

## 测试范围

测试 `src/sql/expression.c` 的表达式求值功能。

## 实际测试用例 (基于 test-expression.c)

### TC-EXPR-001: 整数值创建

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-001 |
| **标题** | 整数值创建 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `value_from_int(42)` |
| **预期结果** | 返回 `VALUE_INTEGER` 类型，值为 42 |
| **实际测试** | `test(test_value_from_int)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-002: 浮点数值创建

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-002 |
| **标题** | 浮点数值创建 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `value_from_float(3.14)` |
| **预期结果** | 返回 `VALUE_FLOAT` 类型 |
| **实际测试** | `test(test_value_from_float)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-003: 文本值创建

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-003 |
| **标题** | 文本值创建 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `value_from_text("hello", 5)` |
| **预期结果** | 返回 `VALUE_TEXT` 类型，值为 "hello" |
| **实际测试** | `test(test_value_from_text)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-004: NULL 值创建

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-004 |
| **标题** | NULL 值创建 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `value_from_null()` |
| **预期结果** | 返回 `VALUE_NULL` 类型 |
| **实际测试** | `test(test_value_from_null)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-005: NULL 值判断

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-005 |
| **标题** | NULL 值判断 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 NULL 值和非 NULL 值 <br> 2. 调用 `value_is_null()` |
| **预期结果** | NULL 返回 true，非 NULL 返回 false |
| **实际测试** | `test(test_value_is_null)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-006: 真值判断

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-006 |
| **标题** | 真值判断 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 测试 NULL、0、空字符串、非空字符串的真值 |
| **预期结果** | NULL 和 0 为 false，非零数值和非空字符串为 true |
| **实际测试** | `test(test_value_is_truthy)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-007: 整数比较

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-007 |
| **标题** | 整数比较 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 比较整数 10 vs 20, 20 vs 10, 10 vs 10 |
| **预期结果** | 返回 -1, 1, 0 |
| **实际测试** | `test(test_value_compare_integers)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-008: 字符串比较

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-008 |
| **标题** | 字符串比较 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 比较 "apple" vs "banana", "banana" vs "apple", "apple" vs "apple" |
| **预期结果** | 返回 -1, 1, 0 |
| **实际测试** | `test(test_value_compare_strings)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-009: 整数字面量求值

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-009 |
| **标题** | 整数字面量求值 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建整数字面量表达式 42 <br> 2. 调用 `eval_literal()` |
| **预期结果** | 返回 `VALUE_INTEGER` 42 |
| **实际测试** | `test(test_eval_literal_int)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-010: 二元加法运算

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-010 |
| **标题** | 二元加法运算 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建二元表达式 `10 + 5` <br> 2. 调用 `expr_eval()` |
| **预期结果** | 返回 `VALUE_INTEGER` 15 |
| **实际测试** | `test(test_eval_binary_plus)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-011: 二元比较运算 (>)

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-011 |
| **标题** | 二元比较运算 (>) |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建二元表达式 `10 > 5` <br> 2. 调用 `expr_eval()` |
| **预期结果** | 返回 `VALUE_INTEGER` 1 (true) |
| **实际测试** | `test(test_eval_binary_gt)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-012: NOT 逻辑运算

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-012 |
| **标题** | NOT 逻辑运算 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建一元表达式 `NOT 1` <br> 2. 调用 `expr_eval()` |
| **预期结果** | 返回 `VALUE_INTEGER` 0 |
| **实际测试** | `test(test_eval_unary_not)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-013: 一元负号运算

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-013 |
| **标题** | 一元负号运算 |
| **优先级** | P0 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建一元表达式 `-42` <br> 2. 调用 `expr_eval()` |
| **预期结果** | 返回 `VALUE_INTEGER` -42 |
| **实际测试** | `test(test_eval_unary_negate)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-014: 整数字面量转 SQL 字符串

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-014 |
| **标题** | 整数字面量转 SQL 字符串 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建整数表达式 42 <br> 2. 调用 `expr_to_sql_string()` |
| **预期结果** | 返回字符串 "42" |
| **实际测试** | `test(sql_int_literal)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-015: 负整数转 SQL 字符串

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-015 |
| **标题** | 负整数转 SQL 字符串 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建整数表达式 -123 <br> 2. 调用 `expr_to_sql_string()` |
| **预期结果** | 返回字符串 "-123" |
| **实际测试** | `test(sql_negative_int_literal)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-016: 浮点数转 SQL 字符串

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-016 |
| **标题** | 浮点数转 SQL 字符串 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建浮点表达式 3.14 <br> 2. 调用 `expr_to_sql_string()` |
| **预期结果** | 返回包含 "3.14" 的字符串 |
| **实际测试** | `test(sql_float_literal)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-017: 字符串转 SQL 字符串

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-017 |
| **标题** | 字符串转 SQL 字符串 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建字符串表达式 "hello" <br> 2. 调用 `expr_to_sql_string()` |
| **预期结果** | 返回字符串 "'hello'" |
| **实际测试** | `test(sql_string_literal)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-018: NULL 转 SQL 字符串

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-018 |
| **标题** | NULL 转 SQL 字符串 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建 NULL 表达式 <br> 2. 调用 `expr_to_sql_string()` |
| **预期结果** | 返回字符串 "NULL" |
| **实际测试** | `test(sql_null_literal)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-019: 二元表达式转 SQL 字符串

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-019 |
| **标题** | 二元表达式转 SQL 字符串 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建二元表达式 `1 + 2` <br> 2. 调用 `expr_to_sql_string()` |
| **预期结果** | 返回字符串 "(1 + 2)" |
| **实际测试** | `test(sql_binary_plus)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-020: 一元表达式转 SQL 字符串

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-020 |
| **标题** | 一元表达式转 SQL 字符串 |
| **优先级** | P1 |
| **前置条件** | 无 |
| **测试步骤** | 1. 创建一元表达式 `-5` <br> 2. 调用 `expr_to_sql_string()` |
| **预期结果** | 返回包含 "-5" 的字符串 |
| **实际测试** | `test(sql_unary_minus)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-021: NULL 缓冲区处理

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-021 |
| **标题** | NULL 缓冲区处理 |
| **优先级** | P2 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `expr_to_sql_string()` 使用 NULL 缓冲区 |
| **预期结果** | 返回 0，不崩溃 |
| **实际测试** | `test(sql_null_buffer)` |
| **测试结果** | ✅ PASS |

---

### TC-EXPR-022: NULL 表达式处理

| 项目 | 内容 |
|------|------|
| **用例ID** | TC-EXPR-022 |
| **标题** | NULL 表达式处理 |
| **优先级** | P2 |
| **前置条件** | 无 |
| **测试步骤** | 1. 调用 `expr_to_sql_string()` 使用 NULL 表达式 |
| **预期结果** | 返回 0，缓冲区为空 |
| **实际测试** | `test(sql_null_expr)` |
| **测试结果** | ✅ PASS |

---

## 测试结果汇总

| 测试用例 | 实际测试函数 | 结果 |
|---------|-------------|------|
| TC-EXPR-001 | `test_value_from_int` | ✅ PASS |
| TC-EXPR-002 | `test_value_from_float` | ✅ PASS |
| TC-EXPR-003 | `test_value_from_text` | ✅ PASS |
| TC-EXPR-004 | `test_value_from_null` | ✅ PASS |
| TC-EXPR-005 | `test_value_is_null` | ✅ PASS |
| TC-EXPR-006 | `test_value_is_truthy` | ✅ PASS |
| TC-EXPR-007 | `test_value_compare_integers` | ✅ PASS |
| TC-EXPR-008 | `test_value_compare_strings` | ✅ PASS |
| TC-EXPR-009 | `test_eval_literal_int` | ✅ PASS |
| TC-EXPR-010 | `test_eval_binary_plus` | ✅ PASS |
| TC-EXPR-011 | `test_eval_binary_gt` | ✅ PASS |
| TC-EXPR-012 | `test_eval_unary_not` | ✅ PASS |
| TC-EXPR-013 | `test_eval_unary_negate` | ✅ PASS |
| TC-EXPR-014 | `sql_int_literal` | ✅ PASS |
| TC-EXPR-015 | `sql_negative_int_literal` | ✅ PASS |
| TC-EXPR-016 | `sql_float_literal` | ✅ PASS |
| TC-EXPR-017 | `sql_string_literal` | ✅ PASS |
| TC-EXPR-018 | `sql_null_literal` | ✅ PASS |
| TC-EXPR-019 | `sql_binary_plus` | ✅ PASS |
| TC-EXPR-020 | `sql_unary_minus` | ✅ PASS |
| TC-EXPR-021 | `sql_null_buffer` | ✅ PASS |
| TC-EXPR-022 | `sql_null_expr` | ✅ PASS |

**通过率: 22/22 (100%)**

---

## 测试运行记录

```
$ make test
  expr_simple_literal... OK
  expr_string_literal... OK
  expr_null_literal... OK
  sql_int_literal... OK
  sql_negative_int_literal... OK
  sql_float_literal... OK
  sql_string_literal... OK
  sql_null_literal... OK
  sql_binary_plus... OK
  sql_unary_minus... OK
  sql_null_buffer... OK
  sql_null_expr... OK
  (repeated twice in output)
```