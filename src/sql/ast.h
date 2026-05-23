#ifndef TINYDB_AST_H
#define TINYDB_AST_H

#include "token.h"
#include "../../include/types.h"
#include <stddef.h>

/*============================================================================
 * AST Node Types
 *============================================================================*/
typedef enum {
    /* Statements */
    AST_CREATE_TABLE,
    AST_DROP_TABLE,
    AST_CREATE_INDEX,
    AST_DROP_INDEX,
    AST_INSERT,
    AST_UPDATE,
    AST_DELETE,
    AST_SELECT,
    AST_TRANSACTION,
    AST_SHOW_TABLES,
    AST_DESCRIBE_TABLE
} AstNodeType;

/*============================================================================
 * Forward declarations
 *============================================================================*/
typedef struct Expression Expression;
typedef struct AstNode AstNode;

/*============================================================================
 * Expression types (from api-design.md)
 *============================================================================*/
typedef enum {
    EXPR_LITERAL_INT,
    EXPR_LITERAL_FLOAT,
    EXPR_LITERAL_STRING,
    EXPR_LITERAL_NULL,
    EXPR_COLUMN,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_FUNC,
    EXPR_CASE,
    EXPR_SUBQUERY,
    EXPR_IN,
    EXPR_BETWEEN,
    EXPR_LIKE
} ExprType;

/*============================================================================
 * Expression with reference counting
 *============================================================================*/
struct Expression {
    ExprType type;
    int refcount;
    union {
        int64_t as_int;                                       /* EXPR_LITERAL_INT */
        double as_float;                                      /* EXPR_LITERAL_FLOAT */
        struct { char* str; size_t len; } as_string;         /* EXPR_LITERAL_STRING */
        struct { int col_index; char* col_name; } as_column; /* EXPR_COLUMN */
        struct { Expression* left; int op; Expression* right; } as_binary; /* EXPR_BINARY */
        struct { int op; Expression* operand; } as_unary;     /* EXPR_UNARY */
        struct { char* name; Expression** args; int arg_count; } as_func;  /* EXPR_FUNC */
        struct { Expression* cond; Expression* then; Expression* else_; } as_case;     /* EXPR_CASE */
        struct { AstNode* query; } as_subquery;              /* EXPR_SUBQUERY */
        struct { Expression* value; Expression** list; int count; int not; } as_in;     /* EXPR_IN */
        struct { Expression* value; Expression* low; Expression* high; int not; } as_between; /* EXPR_BETWEEN */
        struct { Expression* str; Expression* pattern; int escape; int not; } as_like; /* EXPR_LIKE */
    };
};

/*============================================================================
 * Column type is defined in include/types.h (ColumnType)
 *============================================================================*/

/*============================================================================
 * Column definition (for CREATE TABLE)
 *============================================================================*/
typedef struct ColumnDef {
    char* name;
    ColumnType type;
    int not_null;
    int primary_key;
    int autoincrement;
    Expression* default_value;
    struct ColumnDef* next;
} ColumnDef;

/*============================================================================
 * Column list (for SELECT)
 *============================================================================*/
typedef struct ColumnList {
    char* name;
    char* alias;
    struct ColumnList* next;
} ColumnList;

/*============================================================================
 * Value list (for INSERT)
 *============================================================================*/
typedef struct ValueList {
    Expression** values;
    int count;
    struct ValueList* next;
} ValueList;

/*============================================================================
 * ORDER BY item
 *============================================================================*/
typedef struct OrderByItem {
    char* column_name;
    int descending;
    struct OrderByItem* next;
} OrderByItem;

/*============================================================================
 * Set clause (for UPDATE)
 *============================================================================*/
typedef struct SetClause {
    char* column_name;
    Expression* value;
    struct SetClause* next;
} SetClause;

/*============================================================================
 * Base AST node
 *============================================================================*/
struct AstNode {
    AstNodeType type;
    int refcount;
};

/*============================================================================
 * CREATE TABLE statement
 *============================================================================*/
typedef struct {
    AstNode base;
    char* table_name;
    int if_not_exists;
    ColumnDef* columns;
} AstCreateTable;

/*============================================================================
 * DROP TABLE statement
 *============================================================================*/
typedef struct {
    AstNode base;
    char* table_name;
    int if_exists;
} AstDropTable;

/*============================================================================
 * CREATE INDEX statement
 *============================================================================*/
typedef struct {
    AstNode base;
    char* index_name;
    char* table_name;
    char* column_name;
    int unique;
} AstCreateIndex;

/*============================================================================
 * DROP INDEX statement
 *============================================================================*/
typedef struct {
    AstNode base;
    char* index_name;
} AstDropIndex;

/*============================================================================
 * INSERT statement
 *============================================================================*/
typedef struct {
    AstNode base;
    char* table_name;
    ColumnList* columns;
    ValueList* values;
} AstInsert;

/*============================================================================
 * UPDATE statement
 *============================================================================*/
typedef struct {
    AstNode base;
    char* table_name;
    SetClause* set_clauses;
    Expression* where;
} AstUpdate;

/*============================================================================
 * DELETE statement
 *============================================================================*/
typedef struct {
    AstNode base;
    char* table_name;
    Expression* where;
} AstDelete;

/*============================================================================
 * SELECT statement
 *============================================================================*/
typedef struct {
    AstNode base;
    ColumnList* columns;
    int is_distinct;
    char* table_name;
    char* alias;
    Expression* where;
    OrderByItem* order_by;
    Expression* limit;
    Expression* offset;
} AstSelect;

/*============================================================================
 * Transaction statement
 *============================================================================*/
typedef enum {
    TX_BEGIN,
    TX_COMMIT,
    TX_ROLLBACK
} TransactionType;

typedef struct {
    AstNode base;
    TransactionType transaction_type;
} AstTransaction;

/*============================================================================
 * SHOW TABLES statement
 *============================================================================*/
typedef struct {
    AstNode base;
} AstShowTables;

/*============================================================================
 * DESCRIBE TABLE statement
 *============================================================================*/
typedef struct {
    AstNode base;
    char* table_name;
} AstDescribeTable;

/*============================================================================
 * AST node allocation
 *============================================================================*/
AstNode* ast_create(AstNodeType type, size_t size);
void ast_free(AstNode* node);
void ast_ref(AstNode* node);
void ast_unref(AstNode* node);

/*============================================================================
 * Expression allocation and management
 *============================================================================*/
Expression* expr_create(ExprType type, size_t size);
void expr_ref(Expression* e);
void expr_unref(Expression* e);
Expression* expr_copy(Expression* e);

/*============================================================================
 * Helper to cast AST nodes
 *============================================================================*/
#define AST_CAST(type, node) ((type*)((char*)(node) - offsetof(type, base)))

#endif /* TINYDB_AST_H */