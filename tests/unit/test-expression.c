#include "../../src/sql/expression.h"
#include "../../src/sql/ast.h"
#include "mini_test.h"
#include <string.h>

/*============================================================================
 * Expression Evaluation Tests
 *============================================================================*/

test(test_value_from_int) {
    Value* v = value_from_int(42);
    assert_non_null(v);
    assert_eq(v->type, VALUE_INTEGER);
    assert_eq(v->as_int, 42);
    value_free(v);
}

test(test_value_from_float) {
    Value* v = value_from_float(3.14);
    assert_non_null(v);
    assert_eq(v->type, VALUE_FLOAT);
    value_free(v);
}

test(test_value_from_text) {
    Value* v = value_from_text("hello", 5);
    assert_non_null(v);
    assert_eq(v->type, VALUE_TEXT);
    assert_str_eq(v->as_text.str, "hello");
    assert_eq(v->as_text.len, 5);
    value_free(v);
}

test(test_value_from_null) {
    Value* v = value_from_null();
    assert_non_null(v);
    assert_eq(v->type, VALUE_NULL);
    value_free(v);
}

test(test_value_is_null) {
    Value* null_val = value_from_null();
    Value* int_val = value_from_int(0);

    assert_true(value_is_null(null_val));
    assert_false(value_is_null(int_val));

    value_free(null_val);
    value_free(int_val);
}

test(test_value_is_truthy) {
    Value* null_val = value_from_null();
    Value* zero_int = value_from_int(0);
    Value* non_zero_int = value_from_int(1);
    Value* empty_text = value_from_text("", 0);
    Value* non_empty_text = value_from_text("x", 1);

    assert_false(value_is_truthy(null_val));
    assert_false(value_is_truthy(zero_int));
    assert_true(value_is_truthy(non_zero_int));
    assert_false(value_is_truthy(empty_text));
    assert_true(value_is_truthy(non_empty_text));

    value_free(null_val);
    value_free(zero_int);
    value_free(non_zero_int);
    value_free(empty_text);
    value_free(non_empty_text);
}

test(test_value_compare_integers) {
    Value* a = value_from_int(10);
    Value* b = value_from_int(20);
    Value* c = value_from_int(10);

    assert_eq(value_compare(a, b), -1);
    assert_eq(value_compare(b, a), 1);
    assert_eq(value_compare(a, c), 0);

    value_free(a);
    value_free(b);
    value_free(c);
}

test(test_value_compare_strings) {
    Value* a = value_from_text("apple", 5);
    Value* b = value_from_text("banana", 6);
    Value* c = value_from_text("apple", 5);

    assert_eq(value_compare(a, b), -1);
    assert_eq(value_compare(b, a), 1);
    assert_eq(value_compare(a, c), 0);

    value_free(a);
    value_free(b);
    value_free(c);
}

test(test_eval_literal_int) {
    Expression* expr = expr_create(EXPR_LITERAL_INT, sizeof(Expression));
    expr->as_int = 42;

    Value* result = eval_literal(expr);
    assert_non_null(result);
    assert_eq(result->type, VALUE_INTEGER);
    assert_eq(result->as_int, 42);

    expr_unref(expr);
    value_free(result);
}

test(test_eval_binary_plus) {
    Expression* left = expr_create(EXPR_LITERAL_INT, sizeof(Expression));
    left->as_int = 10;
    Expression* right = expr_create(EXPR_LITERAL_INT, sizeof(Expression));
    right->as_int = 5;

    Expression* expr = expr_create(EXPR_BINARY, sizeof(Expression));
    expr->as_binary.left = left;
    expr->as_binary.right = right;
    expr->as_binary.op = '+';

    Value* result = expr_eval(expr, NULL, 0, NULL);
    assert_non_null(result);
    assert_eq(result->type, VALUE_INTEGER);
    assert_eq(result->as_int, 15);

    expr_unref(expr);
    value_free(result);
}

test(test_eval_binary_gt) {
    Expression* left = expr_create(EXPR_LITERAL_INT, sizeof(Expression));
    left->as_int = 10;
    Expression* right = expr_create(EXPR_LITERAL_INT, sizeof(Expression));
    right->as_int = 5;

    Expression* expr = expr_create(EXPR_BINARY, sizeof(Expression));
    expr->as_binary.left = left;
    expr->as_binary.right = right;
    expr->as_binary.op = '>';

    Value* result = expr_eval(expr, NULL, 0, NULL);
    assert_non_null(result);
    assert_eq(result->type, VALUE_INTEGER);
    assert_eq(result->as_int, 1);

    expr_unref(expr);
    value_free(result);
}

test(test_eval_unary_not) {
    Expression* operand = expr_create(EXPR_LITERAL_INT, sizeof(Expression));
    operand->as_int = 1;

    Expression* expr = expr_create(EXPR_UNARY, sizeof(Expression));
    expr->as_unary.operand = operand;
    expr->as_unary.op = '!';

    Value* result = expr_eval(expr, NULL, 0, NULL);
    assert_non_null(result);
    assert_eq(result->type, VALUE_INTEGER);
    assert_eq(result->as_int, 0);

    expr_unref(expr);
    value_free(result);
}

test(test_eval_unary_negate) {
    Expression* operand = expr_create(EXPR_LITERAL_INT, sizeof(Expression));
    operand->as_int = 42;

    Expression* expr = expr_create(EXPR_UNARY, sizeof(Expression));
    expr->as_unary.operand = operand;
    expr->as_unary.op = '-';

    Value* result = expr_eval(expr, NULL, 0, NULL);
    assert_non_null(result);
    assert_eq(result->type, VALUE_INTEGER);
    assert_eq(result->as_int, -42);

    expr_unref(expr);
    value_free(result);
}

/*============================================================================
 * Run all tests
 *============================================================================*/
int main(void) {
    printf("Expression Evaluation Tests\n");
    printf("============================\n");

    run(test_value_from_int);
    run(test_value_from_float);
    run(test_value_from_text);
    run(test_value_from_null);
    run(test_value_is_null);
    run(test_value_is_truthy);
    run(test_value_compare_integers);
    run(test_value_compare_strings);
    run(test_eval_literal_int);
    run(test_eval_binary_plus);
    run(test_eval_binary_gt);
    run(test_eval_unary_not);
    run(test_eval_unary_negate);

    printf("\nAll tests passed!\n");
    return 0;
}