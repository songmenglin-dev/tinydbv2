#include "expression.h"
#include "token.h"
#include "../util/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <ctype.h>
#include <errno.h>
#include <strings.h>
#include <strings.h>  /* for strcasecmp */

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
/* Aggregate function state (passed via row_data context) */
typedef struct AggState {
    int64_t count;
    double sum;
    double min;
    double max;
    int has_value;
} AggState;

/* Evaluate a function call expression (COUNT, SUM, AVG, MAX, MIN, etc.) */
static Value* eval_func(Expression* expr, Value* row_data, int column_count, const char** column_names) {
    if (!expr || expr->type != EXPR_FUNC) return value_from_null();

    const char* name = expr->as_func.name;
    if (!name) return value_from_null();

    /* Case-insensitive function name comparison */
    int is_count = (strcasecmp(name, "COUNT") == 0);
    int is_sum = (strcasecmp(name, "SUM") == 0);
    int is_avg = (strcasecmp(name, "AVG") == 0);
    int is_min = (strcasecmp(name, "MIN") == 0);
    int is_max = (strcasecmp(name, "MAX") == 0);
    int is_coalesce = (strcasecmp(name, "COALESCE") == 0);

    /* COALESCE: return first non-null argument */
    if (is_coalesce) {
        for (int i = 0; i < expr->as_func.arg_count; i++) {
            Value* arg = expr_eval(expr->as_func.args[i], row_data, column_count, column_names);
            if (!value_is_null(arg)) {
                /* Transfer ownership - caller will free result */
                return arg;
            }
            value_free(arg);
        }
        return value_from_null();
    }

    /* Handle COUNT(*) separately - no argument evaluation needed */
    if (is_count && expr->as_func.arg_count == 0) {
        /* COUNT(*) - count all rows, return 1 as a sentinel for the caller to aggregate */
        return value_from_int(1);
    }

    /* For aggregate functions, evaluate the first argument */
    if ((is_count || is_sum || is_avg || is_min || is_max) && expr->as_func.arg_count > 0) {
        Value* arg = expr_eval(expr->as_func.args[0], row_data, column_count, column_names);

        /* COUNT(column) - ignore nulls */
        if (is_count) {
            int result = value_is_null(arg) ? 0 : 1;
            value_free(arg);
            return value_from_int(result);  /* 1 if non-null, 0 if null */
        }

        /* SUM/AVG/MIN/MAX - require numeric values */
        if (value_is_null(arg)) {
            value_free(arg);
            return value_from_null();
        }

        double num_val = 0.0;
        int valid = 0;

        if (arg->type == VALUE_INTEGER) {
            num_val = (double)arg->as_int;
            valid = 1;
        } else if (arg->type == VALUE_FLOAT) {
            num_val = arg->as_float;
            valid = 1;
        }

        value_free(arg);

        if (!valid) return value_from_null();

        if (is_sum) return value_from_float(num_val);         /* SUM: return raw value for caller to accumulate */
        if (is_avg) return value_from_float(num_val);         /* AVG: same pattern */
        if (is_min) return value_from_float(num_val);        /* MIN: same */
        if (is_max) return value_from_float(num_val);         /* MAX: same */
    }

    /* Unknown function */
    return value_from_null();
}

/* Evaluate a CASE WHEN expression */
static Value* eval_case(Expression* expr, Value* row_data, int column_count, const char** column_names) {
    if (!expr || expr->type != EXPR_CASE) return value_from_null();

    Expression* cond = expr->as_case.cond;
    Expression* then = expr->as_case.then;
    Expression* else_ = expr->as_case.else_;

    /* Evaluate condition */
    if (cond) {
        Value* cond_val = expr_eval(cond, row_data, column_count, column_names);
        int is_true = value_is_truthy(cond_val);
        value_free(cond_val);

        if (is_true && then) {
            return expr_eval(then, row_data, column_count, column_names);
        }
    }

    /* Fall through to ELSE */
    if (else_) {
        return expr_eval(else_, row_data, column_count, column_names);
    }

    return value_from_null();
}

/* Evaluate a subquery expression (scalar subquery in SELECT column, WHERE, etc.) */
static Value* eval_subquery(Expression* expr, Value* row_data, int column_count, const char** column_names) {
    (void)row_data;
    (void)column_count;
    (void)column_names;

    if (!expr || expr->type != EXPR_SUBQUERY) return value_from_null();

    AstNode* query = expr->as_subquery.query;
    if (!query) return value_from_null();

    /* Only support SELECT subqueries for now */
    if (query->type != AST_SELECT) return value_from_null();

    /* Delegate to executor for subquery execution */
    extern Value* executor_evaluate_subquery(void* exec_ctx, AstNode* query);
    return executor_evaluate_subquery(NULL, query);
}

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
            return eval_func(expr, row_data, column_count, column_names);
        case EXPR_CASE:
            return eval_case(expr, row_data, column_count, column_names);
        case EXPR_SUBQUERY:
            return eval_subquery(expr, row_data, column_count, column_names);
    }
    return value_from_null();
}

Value* eval_literal(Expression* expr) {
    return expr_eval(expr, NULL, 0, NULL);
}

Value* eval_column(Expression* expr, Value* row_data, int column_count, const char** column_names) {
    if (!expr || !row_data || !column_names) return value_from_null();

    int col_index = expr->as_column.col_index;

    /* If col_index is invalid or name is provided, try to find by name */
    if (col_index < 0 || col_index >= column_count || expr->as_column.col_name) {
        /* Name-based lookup */
        if (expr->as_column.col_name) {
            for (int i = 0; i < column_count; i++) {
                if (column_names[i] && strcmp(expr->as_column.col_name, column_names[i]) == 0) {
                    col_index = i;
                    break;
                }
            }
        }
        /* If still invalid, return NULL */
        if (col_index < 0 || col_index >= column_count) {
            return value_from_null();
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
    return op == '=' || op == TOKEN_EQ || op == '<' || op == '>' || op == TOKEN_LTE || op == TOKEN_GTE || op == TOKEN_NEQ;
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
                case TOKEN_EQ: result = (left_num == right_num); break;
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

/*============================================================================
 * Expression serialization
 *============================================================================*/

/* Internal helper: append to buffer with bounds checking, returns bytes written */
static size_t expr_snprintf(char* buf, size_t buf_size, size_t offset, const char* fmt, ...) {
    if (!buf || buf_size == 0) return 0;
    if (offset >= buf_size) return 0;
    size_t avail = buf_size - offset;
    /* avail == 1: only room for '\0'. Write it directly and return 0 to advance offset.
     * Otherwise vsnprintf would write '\0' at same position forever. */
    if (avail == 1) {
        buf[offset] = '\0';
        return 0;
    }
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf + offset, avail, fmt, args);
    va_end(args);
    if (n < 0) return 0;
    /* vsnprintf returns chars that WOULD be written (excl. null) if unlimited.
     * avail=1: handled above (returns 0)
     * avail>=2: vsnprintf wrote min(n, avail-1) chars + '\0', return min(n, avail-1)
     * Note: when n >= avail, output was truncated; we wrote avail-1 chars. */
    if ((size_t)n >= avail) return avail - 1;
    return (size_t)n;
}

/* Forward declaration for recursive calls */
static size_t expr_to_sql_recursive(Expression* expr, char* buf, size_t buf_size, size_t offset);

/* Serialize binary expression */
static size_t expr_binary_to_sql(Expression* expr, char* buf, size_t buf_size, size_t offset) {
    const char* op_str = " ";
    if (expr->as_binary.op == TOKEN_PLUS)      op_str = " + ";
    else if (expr->as_binary.op == TOKEN_MINUS) op_str = " - ";
    else if (expr->as_binary.op == TOKEN_STAR)  op_str = " * ";
    else if (expr->as_binary.op == TOKEN_SLASH) op_str = " / ";
    else if (expr->as_binary.op == TOKEN_PERCENT) op_str = " % ";
    else if (expr->as_binary.op == '=')          op_str = " = ";
    else if (expr->as_binary.op == TOKEN_EQ)     op_str = " = ";
    else if (expr->as_binary.op == TOKEN_NEQ)    op_str = " <> ";
    else if (expr->as_binary.op == '<')         op_str = " < ";
    else if (expr->as_binary.op == '>')         op_str = " > ";
    else if (expr->as_binary.op == TOKEN_LTE)   op_str = " <= ";
    else if (expr->as_binary.op == TOKEN_GTE)   op_str = " >= ";
    else if (expr->as_binary.op == TOKEN_LIKE_OP) op_str = " LIKE ";
    else if (expr->as_binary.op == TOKEN_AND)  op_str = " AND ";
    else if (expr->as_binary.op == TOKEN_OR)    op_str = " OR ";
    offset += expr_snprintf(buf, buf_size, offset, "(");
    offset = expr_to_sql_recursive(expr->as_binary.left, buf, buf_size, offset);
    offset += expr_snprintf(buf, buf_size, offset, "%s", op_str);
    offset = expr_to_sql_recursive(expr->as_binary.right, buf, buf_size, offset);
    offset += expr_snprintf(buf, buf_size, offset, ")");
    return offset;
}

/* Serialize unary expression */
static size_t expr_unary_to_sql(Expression* expr, char* buf, size_t buf_size, size_t offset) {
    const char* op_str;
    switch (expr->as_unary.op) {
        case TOKEN_MINUS: op_str = "-"; break;
        case TOKEN_PLUS:  op_str = "+"; break;
        case '!':         op_str = "NOT "; break;
        case TOKEN_NOT:   op_str = "NOT "; break;
        default:          op_str = "";  break;
    }
    offset += expr_snprintf(buf, buf_size, offset, "(%s", op_str);
    offset = expr_to_sql_recursive(expr->as_unary.operand, buf, buf_size, offset);
    offset += expr_snprintf(buf, buf_size, offset, ")");
    return offset;
}

/* Serialize function expression */
static size_t expr_func_to_sql(Expression* expr, char* buf, size_t buf_size, size_t offset) {
    offset += expr_snprintf(buf, buf_size, offset, "%s(", expr->as_func.name);
    for (int i = 0; i < expr->as_func.arg_count; i++) {
        if (i > 0) offset += expr_snprintf(buf, buf_size, offset, ", ");
        offset = expr_to_sql_recursive(expr->as_func.args[i], buf, buf_size, offset);
    }
    offset += expr_snprintf(buf, buf_size, offset, ")");
    return offset;
}

/* Serialize CASE expression */
static size_t expr_case_to_sql(Expression* expr, char* buf, size_t buf_size, size_t offset) {
    offset += expr_snprintf(buf, buf_size, offset, "(CASE ");
    if (expr->as_case.cond) {
        offset = expr_to_sql_recursive(expr->as_case.cond, buf, buf_size, offset);
    }
    offset += expr_snprintf(buf, buf_size, offset, " WHEN ");
    if (expr->as_case.then) {
        offset = expr_to_sql_recursive(expr->as_case.then, buf, buf_size, offset);
    }
    offset += expr_snprintf(buf, buf_size, offset, " ELSE ");
    if (expr->as_case.else_) {
        offset = expr_to_sql_recursive(expr->as_case.else_, buf, buf_size, offset);
    }
    offset += expr_snprintf(buf, buf_size, offset, " END)");
    return offset;
}

/* Serialize IN expression */
static size_t expr_in_to_sql(Expression* expr, char* buf, size_t buf_size, size_t offset) {
    offset = expr_to_sql_recursive(expr->as_in.value, buf, buf_size, offset);
    if (expr->as_in.not) {
        offset += expr_snprintf(buf, buf_size, offset, " NOT IN (");
    } else {
        offset += expr_snprintf(buf, buf_size, offset, " IN (");
    }
    for (int i = 0; i < expr->as_in.count; i++) {
        if (i > 0) offset += expr_snprintf(buf, buf_size, offset, ", ");
        offset = expr_to_sql_recursive(expr->as_in.list[i], buf, buf_size, offset);
    }
    offset += expr_snprintf(buf, buf_size, offset, "))");
    return offset;
}

/* Serialize BETWEEN expression */
static size_t expr_between_to_sql(Expression* expr, char* buf, size_t buf_size, size_t offset) {
    offset = expr_to_sql_recursive(expr->as_between.value, buf, buf_size, offset);
    if (expr->as_between.not) {
        offset += expr_snprintf(buf, buf_size, offset, " NOT BETWEEN ");
    } else {
        offset += expr_snprintf(buf, buf_size, offset, " BETWEEN ");
    }
    offset = expr_to_sql_recursive(expr->as_between.low, buf, buf_size, offset);
    offset += expr_snprintf(buf, buf_size, offset, " AND ");
    offset = expr_to_sql_recursive(expr->as_between.high, buf, buf_size, offset);
    return offset;
}

/* Serialize LIKE expression */
static size_t expr_like_to_sql(Expression* expr, char* buf, size_t buf_size, size_t offset) {
    offset = expr_to_sql_recursive(expr->as_like.str, buf, buf_size, offset);
    if (expr->as_like.not) {
        offset += expr_snprintf(buf, buf_size, offset, " NOT LIKE ");
    } else {
        offset += expr_snprintf(buf, buf_size, offset, " LIKE ");
    }
    offset = expr_to_sql_recursive(expr->as_like.pattern, buf, buf_size, offset);
    return offset;
}

static size_t expr_to_sql_recursive(Expression* expr, char* buf, size_t buf_size, size_t offset) {
    if (!buf || buf_size == 0) return offset;
    if (!expr) return offset;

    switch (expr->type) {
        case EXPR_LITERAL_INT:
            offset += expr_snprintf(buf, buf_size, offset, "%" PRId64, expr->as_int);
            break;

        case EXPR_LITERAL_FLOAT: {
            char float_buf[64];
            snprintf(float_buf, sizeof(float_buf), "%g", expr->as_float);
            offset += expr_snprintf(buf, buf_size, offset, "%s", float_buf);
            break;
        }

        case EXPR_LITERAL_STRING:
            offset += expr_snprintf(buf, buf_size, offset, "'%.*s'",
                                    (int)expr->as_string.len, expr->as_string.str);
            break;

        case EXPR_LITERAL_NULL:
            offset += expr_snprintf(buf, buf_size, offset, "NULL");
            break;

        case EXPR_COLUMN:
            if (expr->as_column.table_name) {
                offset += expr_snprintf(buf, buf_size, offset, "%s.%s",
                                        expr->as_column.table_name, expr->as_column.col_name);
            } else {
                offset += expr_snprintf(buf, buf_size, offset, "%s", expr->as_column.col_name);
            }
            break;

        case EXPR_BINARY:
            offset = expr_binary_to_sql(expr, buf, buf_size, offset);
            break;

        case EXPR_UNARY:
            offset = expr_unary_to_sql(expr, buf, buf_size, offset);
            break;

        case EXPR_FUNC:
            offset = expr_func_to_sql(expr, buf, buf_size, offset);
            break;

        case EXPR_CASE:
            offset = expr_case_to_sql(expr, buf, buf_size, offset);
            break;

        case EXPR_IN:
            offset = expr_in_to_sql(expr, buf, buf_size, offset);
            break;

        case EXPR_BETWEEN:
            offset = expr_between_to_sql(expr, buf, buf_size, offset);
            break;

        case EXPR_LIKE:
            offset = expr_like_to_sql(expr, buf, buf_size, offset);
            break;

        case EXPR_SUBQUERY:
            offset += expr_snprintf(buf, buf_size, offset, "(subquery)");
            break;

        default:
            offset += expr_snprintf(buf, buf_size, offset, "(expr)");
            break;
    }

    return offset;
}

size_t expr_to_sql_string(Expression* expr, char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) return 0;
    buf[0] = '\0';
    if (!expr) return 0;

    size_t written = expr_to_sql_recursive(expr, buf, buf_size, 0);
    /* Ensure null termination */
    if (written < buf_size) {
        buf[written] = '\0';
    } else if (buf_size > 0) {
        buf[buf_size - 1] = '\0';
    }
    return written;
}
