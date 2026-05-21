#ifndef TINYDB_EXPRESSION_H
#define TINYDB_EXPRESSION_H

#include "../../include/types.h"
#include "ast.h"
#include <stddef.h>

/*============================================================================
 * Value struct for storing evaluated expression results
 *============================================================================*/
typedef struct Value {
    ValueType type;
    union {
        int64_t as_int;
        double as_float;
        struct { char* str; size_t len; } as_text;
        struct { void* blob; size_t len; } as_blob;
    };
} Value;

/*============================================================================
 * Value creation functions
 *============================================================================*/

/* Create an integer value */
Value* value_from_int(int64_t val);

/* Create a float value */
Value* value_from_float(double val);

/* Create a text value (copies the string) */
Value* value_from_text(const char* str, size_t len);

/* Create a null value */
Value* value_from_null(void);

/* Create a blob value */
Value* value_from_blob(const void* blob, size_t len);

/* Free a value */
void value_free(Value* val);

/*============================================================================
 * Value comparison
 *============================================================================*/

/* Compare two values, returns -1, 0, or 1 */
int value_compare(const Value* a, const Value* b);

/* Check if value is null */
int value_is_null(const Value* val);

/* Check if value is truthy (non-null and non-zero for int, non-empty for text) */
int value_is_truthy(const Value* val);

/*============================================================================
 * Expression evaluation
 *============================================================================*/

/* Evaluate an expression in the context of a row
 * Args:
 *   expr - the expression to evaluate
 *   row_data - pointer to row data (column values as Value array)
 *   column_count - number of columns in the row
 *   column_names - array of column names (for COLUMN expressions)
 *
 * Returns a Value that must be freed with value_free()
 */
Value* expr_eval(Expression* expr, Value* row_data, int column_count, const char** column_names);

/* Evaluate a literal expression */
Value* eval_literal(Expression* expr);

/* Evaluate a column reference */
Value* eval_column(Expression* expr, Value* row_data, int column_count, const char** column_names);

/* Evaluate a binary expression */
Value* eval_binary(Expression* expr, Value* row_data, int column_count, const char** column_names);

/* Evaluate a unary expression */
Value* eval_unary(Expression* expr, Value* row_data, int column_count, const char** column_names);

/* Evaluate a LIKE expression */
Value* eval_like(Expression* expr, Value* row_data, int column_count, const char** column_names);

/* Evaluate an IN expression */
Value* eval_in(Expression* expr, Value* row_data, int column_count, const char** column_names);

/* Evaluate a BETWEEN expression */
Value* eval_between(Expression* expr, Value* row_data, int column_count, const char** column_names);

#endif /* TINYDB_EXPRESSION_H */