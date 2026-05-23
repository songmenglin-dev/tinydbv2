#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*============================================================================
 * Memory allocation helpers
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
 * AST node allocation
 *============================================================================*/
AstNode* ast_create(AstNodeType type, size_t size) {
    AstNode* node = xmalloc(size);
    memset(node, 0, size);
    node->type = type;
    node->refcount = 1;
    return node;
}

void ast_ref(AstNode* node) {
    if (node) {
        node->refcount++;
    }
}

void ast_unref(AstNode* node) {
    if (!node) return;

    node->refcount--;
    if (node->refcount <= 0) {
        ast_free(node);
    }
}

static void free_column_def(ColumnDef* col) {
    if (!col) return;
    free(col->name);
    if (col->default_value) expr_unref(col->default_value);
    free_column_def(col->next);
    free(col);
}

static void free_column_list(ColumnList* col) {
    if (!col) return;
    free(col->name);
    free(col->alias);
    free_column_list(col->next);
    free(col);
}

static void free_value_list(ValueList* vl) {
    if (!vl) return;
    for (int i = 0; i < vl->count; i++) {
        if (vl->values[i]) expr_unref(vl->values[i]);
    }
    free(vl->values);
    free_value_list(vl->next);
    free(vl);
}

static void free_order_by(OrderByItem* ob) {
    if (!ob) return;
    free(ob->column_name);
    free_order_by(ob->next);
    free(ob);
}

static void free_set_clause(SetClause* sc) {
    if (!sc) return;
    free(sc->column_name);
    if (sc->value) expr_unref(sc->value);
    free_set_clause(sc->next);
    free(sc);
}

void ast_free(AstNode* node) {
    if (!node) return;

    switch (node->type) {
        case AST_CREATE_TABLE: {
            AstCreateTable* stmt = AST_CAST(AstCreateTable, node);
            free(stmt->table_name);
            free_column_def(stmt->columns);
            break;
        }
        case AST_DROP_TABLE: {
            AstDropTable* stmt = AST_CAST(AstDropTable, node);
            free(stmt->table_name);
            break;
        }
        case AST_CREATE_INDEX: {
            AstCreateIndex* stmt = AST_CAST(AstCreateIndex, node);
            free(stmt->index_name);
            free(stmt->table_name);
            free(stmt->column_name);
            break;
        }
        case AST_DROP_INDEX: {
            AstDropIndex* stmt = AST_CAST(AstDropIndex, node);
            free(stmt->index_name);
            break;
        }
        case AST_INSERT: {
            AstInsert* stmt = AST_CAST(AstInsert, node);
            free(stmt->table_name);
            free_column_list(stmt->columns);
            free_value_list(stmt->values);
            break;
        }
        case AST_UPDATE: {
            AstUpdate* stmt = AST_CAST(AstUpdate, node);
            free(stmt->table_name);
            free_set_clause(stmt->set_clauses);
            if (stmt->where) expr_unref(stmt->where);
            break;
        }
        case AST_DELETE: {
            AstDelete* stmt = AST_CAST(AstDelete, node);
            free(stmt->table_name);
            if (stmt->where) expr_unref(stmt->where);
            break;
        }
        case AST_SELECT: {
            AstSelect* stmt = AST_CAST(AstSelect, node);
            free_column_list(stmt->columns);
            free(stmt->table_name);
            free(stmt->alias);
            if (stmt->where) expr_unref(stmt->where);
            free_order_by(stmt->order_by);
            if (stmt->limit) expr_unref(stmt->limit);
            if (stmt->offset) expr_unref(stmt->offset);
            break;
        }
        case AST_TRANSACTION:
            /* No additional allocation */
            break;
        case AST_SHOW_TABLES:
            /* No additional allocation */
            break;
        case AST_DESCRIBE_TABLE: {
            AstDescribeTable* stmt = AST_CAST(AstDescribeTable, node);
            free(stmt->table_name);
            break;
        }
    }

    free(node);
}

/*============================================================================
 * Expression allocation
 *============================================================================*/
Expression* expr_create(ExprType type, size_t size) {
    Expression* e = xmalloc(size);
    memset(e, 0, size);
    e->type = type;
    e->refcount = 1;
    return e;
}

void expr_ref(Expression* e) {
    if (e) e->refcount++;
}

void expr_unref(Expression* e) {
    if (!e) return;

    e->refcount--;
    if (e->refcount <= 0) {
        switch (e->type) {
            case EXPR_LITERAL_STRING:
                free(e->as_string.str);
                break;
            case EXPR_BINARY:
                if (e->as_binary.left) expr_unref(e->as_binary.left);
                if (e->as_binary.right) expr_unref(e->as_binary.right);
                break;
            case EXPR_UNARY:
                if (e->as_unary.operand) expr_unref(e->as_unary.operand);
                break;
            case EXPR_FUNC:
                for (int i = 0; i < e->as_func.arg_count; i++) {
                    if (e->as_func.args[i]) expr_unref(e->as_func.args[i]);
                }
                free(e->as_func.args);
                free(e->as_func.name);
                break;
            case EXPR_CASE:
                if (e->as_case.cond) expr_unref(e->as_case.cond);
                if (e->as_case.then) expr_unref(e->as_case.then);
                if (e->as_case.else_) expr_unref(e->as_case.else_);
                break;
            case EXPR_IN:
                if (e->as_in.value) expr_unref(e->as_in.value);
                for (int i = 0; i < e->as_in.count; i++) {
                    if (e->as_in.list[i]) expr_unref(e->as_in.list[i]);
                }
                free(e->as_in.list);
                break;
            case EXPR_BETWEEN:
                if (e->as_between.value) expr_unref(e->as_between.value);
                if (e->as_between.low) expr_unref(e->as_between.low);
                if (e->as_between.high) expr_unref(e->as_between.high);
                break;
            case EXPR_LIKE:
                if (e->as_like.str) expr_unref(e->as_like.str);
                if (e->as_like.pattern) expr_unref(e->as_like.pattern);
                break;
            default:
                break;
        }
        free(e);
    }
}

Expression* expr_copy(Expression* e) {
    if (!e) return NULL;

    Expression* copy = xmalloc(sizeof(Expression));
    memcpy(copy, e, sizeof(Expression));
    copy->refcount = 1;

    /* Handle deep copy for reference types */
    switch (e->type) {
        case EXPR_LITERAL_STRING:
            copy->as_string.str = malloc(e->as_string.len);
            memcpy(copy->as_string.str, e->as_string.str, e->as_string.len);
            break;
        case EXPR_BINARY:
            if (e->as_binary.left) {
                copy->as_binary.left = expr_copy(e->as_binary.left);
            }
            if (e->as_binary.right) {
                copy->as_binary.right = expr_copy(e->as_binary.right);
            }
            break;
        case EXPR_UNARY:
            if (e->as_unary.operand) {
                copy->as_unary.operand = expr_copy(e->as_unary.operand);
            }
            break;
        default:
            break;
    }

    return copy;
}