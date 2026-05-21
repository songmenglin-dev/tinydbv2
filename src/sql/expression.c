#include "expression.h"
#include "token.h"
#include "../util/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

/*============================================================================
 * Memory allocation helper
 *============================================================================*/
static void* xmalloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return ptr;
}

/*============================================================================
 * Value creation functions
 *============================================================================*/
Value* value_from_int(int64_t val) {
    Value* v = xmalloc(sizeof(Value));
    v->type = VALUE_INTEGER;
    v->as_int = val;
    return v;
}

Value* value_from_float(double val) {
    Value* v = xmalloc(sizeof(Value));
    v->type = VALUE_FLOAT;
    v->as_float = val;
    return v;
}

Value* value_from_text(const char* str, size_t len) {
    Value* v = xmalloc(sizeof(Value));
    v->type = VALUE_TEXT;
    v->as_text.str = xmalloc(len + 1);
    memcpy(v->as_text.str, str, len);
    v->as_text.str[len] = '\0';
    v->as_text.len = len;
    return v;
}

Value* value_from_null(void) {
    Value* v = xmalloc(sizeof(Value));
    v->type = VALUE_NULL;
    memset(&v->as_int, 0, sizeof(v->as_int));
    return v;
}

Value* value_from_blob(const void* blob, size_t len) {
    Value* v = xmalloc(sizeof(Value));
    v->type = VALUE_BLOB;
    v->as_blob.blob = xmalloc(len);
    memcpy(v->as_blob.blob, blob, len);
    v->as_blob.len = len;
    return v;
}

void value_free(Value* val) {
    if (!val) return;
    switch (val->type) {
        case VALUE_TEXT:
            free(val->as_text.str);
            break;
        case VALUE_BLOB:
            free(val->as_blob.blob);
            break;
        default:
            break;
    }
    free(val);
}

/*============================================================================
 * Value comparison
 *============================================================================*/
int value_compare(const Value* a, const Value* b) {
    /* Handle null cases */
    if (value_is_null(a) && value_is_null(b)) return 0;
    if (value_is_null(a)) return -1;
    if (value_is_null(b)) return 1;

    /* Type mismatch - compare by underlying type order */
    if (a->type != b->type) {
        /* Integer < Float < Text < Blob */
        int type_order_a = 0, type_order_b = 0;
        switch (a->type) {
            case VALUE_INTEGER: type_order_a = 0; break;
            case VALUE_FLOAT: type_order_a = 1; break;
            case VALUE_TEXT: type_order_a = 2; break;
            case VALUE_BLOB: type_order_a = 3; break;
            default: type_order_a = 0;
        }
        switch (b->type) {
            case VALUE_INTEGER: type_order_b = 0; break;
            case VALUE_FLOAT: type_order_b = 1; break;
            case VALUE_TEXT: type_order_b = 2; break;
            case VALUE_BLOB: type_order_b = 3; break;
            default: type_order_b = 0;
        }
        return (type_order_a < type_order_b) ? -1 : 1;
    }

    /* Same type comparison */
    switch (a->type) {
        case VALUE_INTEGER:
            return (a->as_int < b->as_int) ? -1 : (a->as_int > b->as_int) ? 1 : 0;
        case VALUE_FLOAT:
            return (a->as_float < b->as_float) ? -1 : (a->as_float > b->as_float) ? 1 : 0;
        case VALUE_TEXT: {
            size_t min_len = (a->as_text.len < b->as_text.len) ? a->as_text.len : b->as_text.len;
            int cmp = memcmp(a->as_text.str, b->as_text.str, min_len);
            if (cmp != 0) return cmp;
            if (a->as_text.len < b->as_text.len) return -1;
            if (a->as_text.len > b->as_text.len) return 1;
            return 0;
        }
        case VALUE_BLOB: {
            size_t min_len = (a->as_blob.len < b->as_blob.len) ? a->as_blob.len : b->as_blob.len;
            int cmp = memcmp(a->as_blob.blob, b->as_blob.blob, min_len);
            if (cmp != 0) return cmp;
            if (a->as_blob.len < b->as_blob.len) return -1;
            if (a->as_blob.len > b->as_blob.len) return 1;
            return 0;
        }
        default:
            return 0;
    }
}

int value_is_null(const Value* val) {
    return val == NULL || val->type == VALUE_NULL;
}

int value_is_truthy(const Value* val) {
    if (value_is_null(val)) return 0;
    switch (val->type) {
        case VALUE_INTEGER:
            return val->as_int != 0;
        case VALUE_FLOAT:
            return val->as_float != 0.0;
        case VALUE_TEXT:
            return val->as_text.len > 0;
        case VALUE_BLOB:
            return val->as_blob.len > 0;
        default:
            return 0;
    }
}

/*============================================================================
 * Expression evaluation
 *============================================================================*/
Value* expr_eval(Expression* expr, Value* row_data, int column_count, const char** column_names) {
    if (!expr) return value_from_null();

    switch (expr->type) {
        case EXPR_LITERAL_INT:
            return value_from_int(expr->as_int);
        case EXPR_LITERAL_FLOAT:
            return value_from_float(expr->as_float);
        case EXPR_LITERAL_STRING:
            return value_from_text(expr->as_string.str, expr->as_string.len);
        case EXPR_LITERAL_NULL:
            return value_from_null();
        case EXPR_COLUMN:
            return eval_column(expr, row_data, column_count, column_names);
        case EXPR_BINARY:
            return eval_binary(expr, row_data, column_count, column_names);
        case EXPR_UNARY:
            return eval_unary(expr, row_data, column_count, column_names);
        case EXPR_LIKE:
            return eval_like(expr, row_data, column_count, column_names);
        case EXPR_IN:
            return eval_in(expr, row_data, column_count, column_names);
        case EXPR_BETWEEN:
            return eval_between(expr, row_data, column_count, column_names);
        case EXPR_FUNC:
        case EXPR_CASE:
        case EXPR_SUBQUERY:
            /* TODO: Implement these later */
            return value_from_null();
    }
    return value_from_null();
}

Value* eval_literal(Expression* expr) {
    return expr_eval(expr, NULL, 0, NULL);
}

Value* eval_column(Expression* expr, Value* row_data, int column_count, const char** column_names) {
    if (!expr || !row_data || !column_names) return value_from_null();

    int col_index = expr->as_column.col_index;
    if (col_index < 0 || col_index >= column_count) return value_from_null();

    /* If column name is provided, verify it matches */
    if (expr->as_column.col_name && column_names[col_index]) {
        if (strcmp(expr->as_column.col_name, column_names[col_index]) != 0) {
            /* Name mismatch, try to find by name */
            for (int i = 0; i < column_count; i++) {
                if (column_names[i] && strcmp(expr->as_column.col_name, column_names[i]) == 0) {
                    col_index = i;
                    break;
                }
            }
        }
    }

    Value* src = &row_data[col_index];
    Value* copy = xmalloc(sizeof(Value));
    memcpy(copy, src, sizeof(Value));

    /* Deep copy for heap-allocated data */
    if (src->type == VALUE_TEXT) {
        copy->as_text.str = xmalloc(src->as_text.len + 1);
        memcpy(copy->as_text.str, src->as_text.str, src->as_text.len);
        copy->as_text.str[src->as_text.len] = '\0';
    } else if (src->type == VALUE_BLOB) {
        copy->as_blob.blob = xmalloc(src->as_blob.len);
        memcpy(copy->as_blob.blob, src->as_blob.blob, src->as_blob.len);
    }

    return copy;
}

/* Binary operators */
static int is_numericComparison(int op) {
    return op == '=' || op == '<' || op == '>' || op == TOKEN_LTE || op == TOKEN_GTE || op == TOKEN_NEQ;
}

Value* eval_binary(Expression* expr, Value* row_data, int column_count, const char** column_names) {
    if (!expr) return value_from_null();

    Value* left = expr_eval(expr->as_binary.left, row_data, column_count, column_names);
    Value* right = expr_eval(expr->as_binary.right, row_data, column_count, column_names);
    int op = expr->as_binary.op;

    /* Handle logical operators first */
    if (op == TOKEN_AND) {
        int left_truthy = value_is_truthy(left);
        int right_truthy = value_is_truthy(right);
        value_free(left);
        value_free(right);
        return value_from_int(left_truthy && right_truthy);
    }
    if (op == TOKEN_OR) {
        int left_truthy = value_is_truthy(left);
        int right_truthy = value_is_truthy(right);
        value_free(left);
        value_free(right);
        return value_from_int(left_truthy || right_truthy);
    }

    /* Handle NULL-safe equality */
    if (op == TOKEN_IS) {
        int result = value_is_null(left) == value_is_null(right);
        value_free(left);
        value_free(right);
        return value_from_int(result);
    }
    /* IS NOT is handled as IS + NOT combination in parser */

    /* Handle comparison operators */
    if (is_numericComparison(op)) {
        /* Type coercion: if either is TEXT, convert the other */
        if (left->type == VALUE_TEXT || right->type == VALUE_TEXT) {
            /* At least one is text - convert both to text for comparison */
            const char* left_str = NULL;
            const char* right_str = NULL;
            char left_buf[32] = {0};
            char right_buf[32] = {0};

            if (left->type == VALUE_TEXT) {
                left_str = left->as_text.str;
            } else if (left->type == VALUE_INTEGER) {
                snprintf(left_buf, sizeof(left_buf), "%lld", (long long)left->as_int);
                left_str = left_buf;
            } else if (left->type == VALUE_FLOAT) {
                snprintf(left_buf, sizeof(left_buf), "%.15g", left->as_float);
                left_str = left_buf;
            }

            if (right->type == VALUE_TEXT) {
                right_str = right->as_text.str;
            } else if (right->type == VALUE_INTEGER) {
                snprintf(right_buf, sizeof(right_buf), "%lld", (long long)right->as_int);
                right_str = right_buf;
            } else if (right->type == VALUE_FLOAT) {
                snprintf(right_buf, sizeof(right_buf), "%.15g", right->as_float);
                right_str = right_buf;
            }

            int cmp = strcmp(left_str ? left_str : "", right_str ? right_str : "");
            value_free(left);
            value_free(right);

            switch (op) {
                case '=':  return value_from_int(cmp == 0);
                case TOKEN_NEQ: return value_from_int(cmp != 0);
                case '<':  return value_from_int(cmp < 0);
                case '>':  return value_from_int(cmp > 0);
                case TOKEN_LTE: return value_from_int(cmp <= 0);
                case TOKEN_GTE: return value_from_int(cmp >= 0);
                default: return value_from_int(0);
            }
        }

        /* Numeric comparison */
        double left_num = 0.0, right_num = 0.0;
        int left_valid = 0, right_valid = 0;

        if (left->type == VALUE_INTEGER) {
            left_num = (double)left->as_int;
            left_valid = 1;
        } else if (left->type == VALUE_FLOAT) {
            left_num = left->as_float;
            left_valid = 1;
        }
        if (right->type == VALUE_INTEGER) {
            right_num = (double)right->as_int;
            right_valid = 1;
        } else if (right->type == VALUE_FLOAT) {
            right_num = right->as_float;
            right_valid = 1;
        }

        int result = 0;
        if (left_valid && right_valid) {
            switch (op) {
                case '=':  result = (left_num == right_num); break;
                case TOKEN_NEQ: result = (left_num != right_num); break;
                case '<':  result = (left_num < right_num); break;
                case '>':  result = (left_num > right_num); break;
                case TOKEN_LTE: result = (left_num <= right_num); break;
                case TOKEN_GTE: result = (left_num >= right_num); break;
            }
        }

        value_free(left);
        value_free(right);
        return value_from_int(result);
    }

    /* Handle arithmetic operators */
    if (op == '+') {
        Value* result;
        if (left->type == VALUE_INTEGER && right->type == VALUE_INTEGER) {
            result = value_from_int(left->as_int + right->as_int);
        } else {
            double l = (left->type == VALUE_INTEGER) ? (double)left->as_int : left->as_float;
            double r = (right->type == VALUE_INTEGER) ? (double)right->as_int : right->as_float;
            result = value_from_float(l + r);
        }
        value_free(left);
        value_free(right);
        return result;
    }
    if (op == '-') {
        Value* result;
        if (left->type == VALUE_INTEGER && right->type == VALUE_INTEGER) {
            result = value_from_int(left->as_int - right->as_int);
        } else {
            double l = (left->type == VALUE_INTEGER) ? (double)left->as_int : left->as_float;
            double r = (right->type == VALUE_INTEGER) ? (double)right->as_int : right->as_float;
            result = value_from_float(l - r);
        }
        value_free(left);
        value_free(right);
        return result;
    }
    if (op == '*') {
        Value* result;
        if (left->type == VALUE_INTEGER && right->type == VALUE_INTEGER) {
            result = value_from_int(left->as_int * right->as_int);
        } else {
            double l = (left->type == VALUE_INTEGER) ? (double)left->as_int : left->as_float;
            double r = (right->type == VALUE_INTEGER) ? (double)right->as_int : right->as_float;
            result = value_from_float(l * r);
        }
        value_free(left);
        value_free(right);
        return result;
    }
    if (op == '/') {
        double r = (right->type == VALUE_INTEGER) ? (double)right->as_int : right->as_float;
        if (r == 0.0) {
            value_free(left);
            value_free(right);
            return value_from_null();  /* Division by zero */
        }
        Value* result;
        if (left->type == VALUE_INTEGER && right->type == VALUE_INTEGER) {
            result = value_from_int(left->as_int / (int64_t)r);
        } else {
            double l = (left->type == VALUE_INTEGER) ? (double)left->as_int : left->as_float;
            result = value_from_float(l / r);
        }
        value_free(left);
        value_free(right);
        return result;
    }

    /* Handle LIKE */
    if (op == TOKEN_LIKE) {
        Value* like_result = eval_like(expr, row_data, column_count, column_names);
        value_free(left);
        value_free(right);
        return like_result;
    }

    /* Handle IN */
    if (op == TOKEN_IN) {
        Value* in_result = eval_in(expr, row_data, column_count, column_names);
        value_free(left);
        value_free(right);
        return in_result;
    }

    /* Handle BETWEEN */
    if (op == TOKEN_BETWEEN) {
        Value* between_result = eval_between(expr, row_data, column_count, column_names);
        value_free(left);
        value_free(right);
        return between_result;
    }

    /* Unknown operator */
    value_free(left);
    value_free(right);
    return value_from_null();
}

Value* eval_unary(Expression* expr, Value* row_data, int column_count, const char** column_names) {
    if (!expr) return value_from_null();

    Value* operand = expr_eval(expr->as_unary.operand, row_data, column_count, column_names);
    int op = expr->as_unary.op;

    if (op == '!') {
        int truthy = value_is_truthy(operand);
        value_free(operand);
        return value_from_int(!truthy);
    }
    if (op == '-') {
        if (operand->type == VALUE_INTEGER) {
            int64_t val = -operand->as_int;
            value_free(operand);
            return value_from_int(val);
        } else if (operand->type == VALUE_FLOAT) {
            double val = -operand->as_float;
            value_free(operand);
            return value_from_float(val);
        }
        value_free(operand);
        return value_from_null();
    }
    if (op == TOKEN_NOT || op == TOKEN_IS) {
        /* NOT NULL check */
        value_free(operand);
        return value_from_int(0);  /* Simplified */
    }

    value_free(operand);
    return value_from_null();
}

/* Simple pattern matching for LIKE */
static int simple_like_match(const char* str, size_t str_len, const char* pattern, size_t pat_len) {
    if (pat_len == 0) return (str_len == 0);  /* Empty pattern matches empty string */

    const char* s = str;
    const char* p = pattern;

    const char* last_star = NULL;
    const char* last_star_s = NULL;

    /* Find last consecutive * for greedy matching */
    while (p < pattern + pat_len) {
        if (*p == '%') {
            last_star = p;
            last_star_s = s;
            while (p + 1 < pattern + pat_len && *(p + 1) == '%') p++;  /* Skip consecutive % */
        } else {
            if (s >= str + str_len) {
                /* No more chars in string - if pattern ends or just has %, it matches */
                while (p < pattern + pat_len && *p == '%') p++;
                return (p >= pattern + pat_len) ? 1 : 0;
            }
            if (*p == '_') {
                s++;
                p++;
            } else if (*p == *s) {
                s++;
                p++;
            } else {
                if (!last_star) return 0;
                p = last_star + 1;
                if (p >= pattern + pat_len) {
                    return 1;  /* Pattern ends after * - match anything */
                }
                /* Try to match at next position */
                while (last_star_s < str + str_len && *last_star_s != *p) last_star_s++;
                if (last_star_s >= str + str_len) return 0;
                s = last_star_s + 1;
                last_star_s++;
            }
        }
    }
    return (s >= str + str_len) ? 1 : 0;
}

Value* eval_like(Expression* expr, Value* row_data, int column_count, const char** column_names) {
    if (!expr || expr->type != EXPR_LIKE) return value_from_null();

    Value* str_val = expr_eval(expr->as_like.str, row_data, column_count, column_names);
    Value* pattern_val = expr_eval(expr->as_like.pattern, row_data, column_count, column_names);

    int result = 0;
    if (str_val->type == VALUE_TEXT && pattern_val->type == VALUE_TEXT) {
        result = simple_like_match(str_val->as_text.str, str_val->as_text.len,
                                   pattern_val->as_text.str, pattern_val->as_text.len);
        if (expr->as_like.not) result = !result;
    }

    value_free(str_val);
    value_free(pattern_val);
    return value_from_int(result);
}

Value* eval_in(Expression* expr, Value* row_data, int column_count, const char** column_names) {
    if (!expr || expr->type != EXPR_IN) return value_from_null();

    Value* value = expr_eval(expr->as_in.value, row_data, column_count, column_names);

    int result = 0;
    for (int i = 0; i < expr->as_in.count; i++) {
        Value* item = expr_eval(expr->as_in.list[i], row_data, column_count, column_names);
        if (value_compare(value, item) == 0) {
            result = 1;
            value_free(item);
            break;
        }
        value_free(item);
    }

    if (expr->as_in.not) result = !result;

    value_free(value);
    return value_from_int(result);
}

Value* eval_between(Expression* expr, Value* row_data, int column_count, const char** column_names) {
    if (!expr || expr->type != EXPR_BETWEEN) return value_from_null();

    Value* value = expr_eval(expr->as_between.value, row_data, column_count, column_names);
    Value* low = expr_eval(expr->as_between.low, row_data, column_count, column_names);
    Value* high = expr_eval(expr->as_between.high, row_data, column_count, column_names);

    int result = 0;
    if (!value_is_null(value) && !value_is_null(low) && !value_is_null(high)) {
        int ge_low = value_compare(value, low) >= 0;
        int le_high = value_compare(value, high) <= 0;
        result = ge_low && le_high;
    }

    if (expr->as_between.not) result = !result;

    value_free(value);
    value_free(low);
    value_free(high);
    return value_from_int(result);
}