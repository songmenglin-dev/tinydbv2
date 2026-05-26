#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/*============================================================================
 * Type casting helpers to silence warnings on tagged struct initializers
 *============================================================================*/
static inline AstCreateTable* cast_create_table(AstNode* n) { return (AstCreateTable*)n; }
static inline AstDropTable* cast_drop_table(AstNode* n) { return (AstDropTable*)n; }
static inline AstCreateIndex* cast_create_index(AstNode* n) { return (AstCreateIndex*)n; }
static inline AstDropIndex* cast_drop_index(AstNode* n) { return (AstDropIndex*)n; }
static inline AstInsert* cast_insert(AstNode* n) { return (AstInsert*)n; }
static inline AstUpdate* cast_update(AstNode* n) { return (AstUpdate*)n; }
static inline AstDelete* cast_delete(AstNode* n) { return (AstDelete*)n; }
static inline AstSelect* cast_select(AstNode* n) { return (AstSelect*)n; }
static inline AstTransaction* cast_transaction(AstNode* n) { return (AstTransaction*)n; }
static inline AstShowTables* cast_show_tables(AstNode* n) { return (AstShowTables*)n; }
static inline AstDescribeTable* cast_describe_table(AstNode* n) { return (AstDescribeTable*)n; }

/*============================================================================
 * Parser structure
 *============================================================================*/
struct Parser {
    Lexer* lexer;
    Token current_token;
    int has_error;
    char error[256];
    int error_line;
    int error_column;
};

/*============================================================================
 * Parser lifecycle
 *============================================================================*/
Parser* parser_create(const char* sql, size_t len) {
    if (!sql || len == 0) return NULL;

    Parser* parser = calloc(1, sizeof(Parser));
    if (!parser) return NULL;

    parser->lexer = lexer_create(sql, len);
    if (!parser->lexer) {
        free(parser);
        return NULL;
    }

    parser->has_error = 0;
    parser->error[0] = '\0';

    /* Initialize current token */
    parser->current_token = lexer_next_token(parser->lexer);

    return parser;
}

void parser_destroy(Parser* parser) {
    if (parser) {
        lexer_destroy(parser->lexer);
        free(parser);
    }
}

char* parser_error(Parser* parser) {
    if (parser && parser->has_error) {
        return parser->error;
    }
    return NULL;
}

int parser_error_line(Parser* parser) {
    return parser ? parser->error_line : 0;
}

int parser_error_column(Parser* parser) {
    return parser ? parser->error_column : 0;
}

/*============================================================================
 * Error reporting
 *============================================================================*/
static void parser_set_error(Parser* parser, const char* msg) {
    parser->has_error = 1;
    snprintf(parser->error, sizeof(parser->error), "%s", msg);
    parser->error_line = parser->current_token.line;
    parser->error_column = parser->current_token.column;
}

/*============================================================================
 * Token helpers
 *============================================================================*/
static int keyword_to_token_type(const char* kw) {
    if (strcmp(kw, "SELECT") == 0) return TOKEN_SELECT;
    if (strcmp(kw, "DISTINCT") == 0) return TOKEN_DISTINCT;
    if (strcmp(kw, "FROM") == 0) return TOKEN_FROM;
    if (strcmp(kw, "WHERE") == 0) return TOKEN_WHERE;
    if (strcmp(kw, "ORDER") == 0) return TOKEN_ORDER;
    if (strcmp(kw, "BY") == 0) return TOKEN_BY;
    if (strcmp(kw, "ASC") == 0) return TOKEN_ASC;
    if (strcmp(kw, "DESC") == 0) return TOKEN_DESC;
    if (strcmp(kw, "LIMIT") == 0) return TOKEN_LIMIT;
    if (strcmp(kw, "OFFSET") == 0) return TOKEN_OFFSET;
    if (strcmp(kw, "INTO") == 0) return TOKEN_INTO;
    if (strcmp(kw, "VALUES") == 0) return TOKEN_VALUES;
    if (strcmp(kw, "INSERT") == 0) return TOKEN_INSERT;
    if (strcmp(kw, "UPDATE") == 0) return TOKEN_UPDATE;
    if (strcmp(kw, "DELETE") == 0) return TOKEN_DELETE;
    if (strcmp(kw, "SET") == 0) return TOKEN_SET;
    if (strcmp(kw, "CREATE") == 0) return TOKEN_CREATE;
    if (strcmp(kw, "DROP") == 0) return TOKEN_DROP;
    if (strcmp(kw, "TABLE") == 0) return TOKEN_TABLE;
    if (strcmp(kw, "INDEX") == 0) return TOKEN_INDEX;
    if (strcmp(kw, "AND") == 0) return TOKEN_AND;
    if (strcmp(kw, "OR") == 0) return TOKEN_OR;
    if (strcmp(kw, "NOT") == 0) return TOKEN_NOT;
    if (strcmp(kw, "NULL") == 0) return TOKEN_NULL;
    if (strcmp(kw, "LIKE") == 0) return TOKEN_LIKE;
    if (strcmp(kw, "BETWEEN") == 0) return TOKEN_BETWEEN;
    if (strcmp(kw, "IN") == 0) return TOKEN_IN;
    if (strcmp(kw, "IS") == 0) return TOKEN_IS;
    if (strcmp(kw, "BEGIN") == 0) return TOKEN_BEGIN;
    if (strcmp(kw, "COMMIT") == 0) return TOKEN_COMMIT;
    if (strcmp(kw, "ROLLBACK") == 0) return TOKEN_ROLLBACK;
    if (strcmp(kw, "PRIMARY") == 0) return TOKEN_PRIMARY;
    if (strcmp(kw, "KEY") == 0) return TOKEN_KEY;
    if (strcmp(kw, "IF") == 0) return TOKEN_IF;
    if (strcmp(kw, "EXISTS") == 0) return TOKEN_EXISTS;
    if (strcmp(kw, "AS") == 0) return TOKEN_AS;
    if (strcmp(kw, "ON") == 0) return TOKEN_ON;
    if (strcmp(kw, "UNIQUE") == 0) return TOKEN_UNIQUE;
    if (strcmp(kw, "DEFAULT") == 0) return TOKEN_DEFAULT;
    if (strcmp(kw, "AUTOINCREMENT") == 0) return TOKEN_AUTOINCREMENT;
    if (strcmp(kw, "SHOW") == 0) return TOKEN_SHOW;
    if (strcmp(kw, "DESCRIBE") == 0) return TOKEN_DESCRIBE;
    return -1;
}

static int check_keyword(Parser* parser, const char* kw) {
    // Check if current token is the expected keyword token type
    int expected_type = keyword_to_token_type(kw);
    if (expected_type >= 0 && (TokenType)expected_type == parser->current_token.type) {
        return 1;
    }
    // Also handle IDENTIFIER case for compatibility
    if (parser->current_token.type != TOKEN_IDENTIFIER) return 0;
    const char* text = token_get_text(&parser->current_token);
    if (!text) return 0;
    size_t len = strlen(text);
    char* upper = malloc(len + 1);
    if (!upper) return 0;
    for (size_t i = 0; i < len; i++) {
        upper[i] = (char)toupper((unsigned char)text[i]);
    }
    upper[len] = '\0';
    int result = strcmp(upper, kw) == 0;
    free(upper);
    return result;
}

static void advance(Parser* parser) {
    if (!parser->has_error) {
        parser->current_token = lexer_next_token(parser->lexer);
    }
}

static void expect_token(Parser* parser, TokenType type, const char* expected) {
    if (parser->current_token.type != type) {
        parser_set_error(parser, expected);
    } else {
        advance(parser);
    }
}

static char* parse_identifier(Parser* parser) {
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        parser_set_error(parser, "Expected identifier");
        return NULL;
    }
    char* id = strdup(token_get_text(&parser->current_token));
    advance(parser);
    return id;
}

/*============================================================================
 * Expression parsing
 *============================================================================*/
static Expression* parse_expression(Parser* parser);
static Expression* parse_binary_expr(Parser* parser, int min_prec);

/* Forward declaration for keyword expectation */
static void expect_keyword(Parser* parser, const char* kw, const char* msg);

static Expression* parse_literal(Parser* parser) {
    Expression* expr = NULL;

    switch (parser->current_token.type) {
        case TOKEN_INTEGER: {
            expr = expr_create(EXPR_LITERAL_INT, sizeof(Expression));
            expr->as_int = parser->current_token.value.integer_value;
            advance(parser);
            break;
        }
        case TOKEN_REAL: {
            expr = expr_create(EXPR_LITERAL_FLOAT, sizeof(Expression));
            expr->as_float = parser->current_token.value.float_value;
            advance(parser);
            break;
        }
        case TOKEN_STRING: {
            expr = expr_create(EXPR_LITERAL_STRING, sizeof(Expression));
            expr->as_string.str = strndup(
                parser->current_token.value.string_value.value,
                parser->current_token.value.string_value.length
            );
            expr->as_string.len = parser->current_token.value.string_value.length;
            advance(parser);
            break;
        }
        case TOKEN_NULL: {
            expr = expr_create(EXPR_LITERAL_NULL, sizeof(Expression));
            advance(parser);
            break;
        }
        case TOKEN_IDENTIFIER: {
            char* name = parse_identifier(parser);
            if (!name) return NULL;

            /* Check if it's a column reference (could be table.column) */
            if (parser->current_token.type == TOKEN_DOT) {
                /* Qualified column name */
                char* table_name = name;
                advance(parser);
                char* col_name = parse_identifier(parser);
                if (!col_name) {
                    free(table_name);
                    return NULL;
                }

                expr = expr_create(EXPR_COLUMN, sizeof(Expression));
                expr->as_column.col_name = col_name;
                expr->as_column.table_name = table_name;
                expr->as_column.col_index = -1;
            } else {
                /* Simple column reference */
                expr = expr_create(EXPR_COLUMN, sizeof(Expression));
                expr->as_column.col_name = name;
                expr->as_column.col_index = -1; /* Will be resolved in analyzer */
            }
            break;
        }
        case TOKEN_LPAREN: {
            advance(parser);
            expr = parse_expression(parser);
            if (parser->current_token.type == TOKEN_RPAREN) {
                advance(parser);
            } else {
                parser_set_error(parser, "Expected ')'");
            }
            break;
        }
        case TOKEN_MINUS: {
            advance(parser);
            Expression* operand = parse_literal(parser);
            if (!operand) return NULL;
            expr = expr_create(EXPR_UNARY, sizeof(Expression));
            expr->as_unary.op = TOKEN_MINUS;
            expr->as_unary.operand = operand;
            break;
        }
        case TOKEN_PLUS: {
            advance(parser);
            Expression* operand = parse_literal(parser);
            if (!operand) return NULL;
            expr = expr_create(EXPR_UNARY, sizeof(Expression));
            expr->as_unary.op = TOKEN_PLUS;
            expr->as_unary.operand = operand;
            break;
        }
        default:
            parser_set_error(parser, "Unexpected token in expression");
            return NULL;
    }

    return expr;
}

static Expression* parse_unary_not(Parser* parser) {
    if (check_keyword(parser, "NOT")) {
        advance(parser);
        Expression* operand = parse_unary_not(parser);
        if (!operand) return NULL;
        Expression* expr = expr_create(EXPR_UNARY, sizeof(Expression));
        expr->as_unary.op = TOKEN_NOT;
        expr->as_unary.operand = operand;
        return expr;
    }
    return parse_literal(parser);
}

/* Operator precedence table */
typedef struct {
    TokenType type;
    int precedence;
    int right_assoc;
} OpInfo;

static int get_precedence(TokenType type) {
    switch (type) {
        case TOKEN_OR: return 4;
        case TOKEN_AND: return 5;
        case TOKEN_NOT: return 6;
        case TOKEN_EQ:
        case TOKEN_NEQ:
        case TOKEN_LT:
        case TOKEN_GT:
        case TOKEN_LTE:
        case TOKEN_GTE: return 7;
        case TOKEN_LIKE_OP:
        case TOKEN_IN_OP:
        case TOKEN_BETWEEN_OP: return 8;
        case TOKEN_PLUS:
        case TOKEN_MINUS: return 9;
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT: return 10;
        default: return 0;
    }
}

static int is_right_assoc(TokenType type) {
    (void)type;
    return 0; /* All left associative for now */
}

static Expression* parse_binary_expr(Parser* parser, int min_prec) {
    (void)min_prec;  /* Reserved for future precedence enhancements */
    Expression* left = parse_unary_not(parser);
    if (!left || parser->has_error) return left;

    while (1) {
        TokenType op = parser->current_token.type;
        int prec = get_precedence(op);

        /* Handle LIKE, IN, BETWEEN as binary operators */
        if (op == TOKEN_LIKE || op == TOKEN_IN || op == TOKEN_BETWEEN) {
            /* These are special - treat them as binary */
            if (op == TOKEN_LIKE) {
                advance(parser);
                Expression* pattern = parse_binary_expr(parser, prec + 1);
                if (!pattern) return NULL;
                Expression* expr = expr_create(EXPR_LIKE, sizeof(Expression));
                expr->as_like.str = left;
                expr->as_like.pattern = pattern;
                expr->as_like.escape = 0;
                expr->as_like.not = 0;
                left = expr;
            } else if (op == TOKEN_IN) {
                advance(parser);
                if (parser->current_token.type == TOKEN_LPAREN) {
                    advance(parser);
                    /* Parse list */
                    Expression** list = NULL;
                    int count = 0;
                    int capacity = 4;

                    if (parser->current_token.type != TOKEN_RPAREN) {
                        list = malloc(sizeof(Expression*) * capacity);
                        if (!list) return NULL;

                        while (1) {
                            if (count >= capacity) {
                                capacity *= 2;
                                Expression** new_list = realloc(list, sizeof(Expression*) * capacity);
                                if (!new_list) {
                                    free(list);
                                    return NULL;
                                }
                                list = new_list;
                            }
                            list[count] = parse_expression(parser);
                            if (!list[count]) {
                                for (int i = 0; i < count; i++) expr_unref(list[i]);
                                free(list);
                                return NULL;
                            }
                            count++;

                            if (parser->current_token.type == TOKEN_COMMA) {
                                advance(parser);
                            } else {
                                break;
                            }
                        }

                        expect_token(parser, TOKEN_RPAREN, "Expected ')'");
                    } else {
                        list = NULL;
                        expect_token(parser, TOKEN_RPAREN, "Expected ')'");
                    }

                    Expression* expr = expr_create(EXPR_IN, sizeof(Expression));
                    expr->as_in.value = left;
                    expr->as_in.list = list;
                    expr->as_in.count = count;
                    expr->as_in.not = 0;
                    left = expr;
                }
            } else if (op == TOKEN_BETWEEN) {
                advance(parser);
                Expression* low = parse_binary_expr(parser, prec + 1);
                if (!low) return NULL;
                if (!check_keyword(parser, "AND")) {
                    parser_set_error(parser, "Expected AND in BETWEEN expression");
                    expr_unref(low);
                    return NULL;
                }
                advance(parser);
                Expression* high = parse_binary_expr(parser, prec + 1);
                if (!high) {
                    expr_unref(low);
                    return NULL;
                }

                Expression* expr = expr_create(EXPR_BETWEEN, sizeof(Expression));
                expr->as_between.value = left;
                expr->as_between.low = low;
                expr->as_between.high = high;
                expr->as_between.not = 0;
                left = expr;
            }
            continue;
        }

        if (prec == 0) break;

        /* Precedence climbing: stop if operator precedence is lower than min_prec */
        if (prec < min_prec) break;

        /* Check for IS [NOT] NULL special case */
        if (op == TOKEN_IS) {
            advance(parser);
            int is_not = 0;
            if (check_keyword(parser, "NOT")) {
                advance(parser);
                is_not = 1;
            }
            if (check_keyword(parser, "NULL")) {
                advance(parser);
                Expression* expr = expr_create(EXPR_BINARY, sizeof(Expression));
                expr->as_binary.left = left;
                expr->as_binary.op = is_not ? TOKEN_NEQ : TOKEN_EQ;
                expr->as_binary.right = expr_create(EXPR_LITERAL_NULL, sizeof(Expression));
                left = expr;
                continue;
            } else {
                parser_set_error(parser, "Expected NULL after IS");
                return NULL;
            }
        }

        /* Handle NOT at expression level */
        if (op == TOKEN_NOT) {
            /* NOT has lower precedence than binary operators when followed by expression */
            break;
        }

        advance(parser); /* consume operator */

        Expression* right = parse_binary_expr(parser, prec + (is_right_assoc(op) ? 0 : 1));
        if (!right) {
            expr_unref(left);
            return NULL;
        }

        Expression* binary = expr_create(EXPR_BINARY, sizeof(Expression));
        binary->as_binary.left = left;
        binary->as_binary.op = op;
        binary->as_binary.right = right;
        left = binary;
    }

    return left;
}

static Expression* parse_expression(Parser* parser) {
    return parse_binary_expr(parser, 0);
}

/*============================================================================
 * Statement parsing
 *============================================================================*/
static AstNode* parse_create_table(Parser* parser);
static AstNode* parse_drop_table(Parser* parser);
static AstNode* parse_create_index(Parser* parser);
static AstNode* parse_drop_index(Parser* parser);
static AstNode* parse_insert(Parser* parser);
static AstNode* parse_update(Parser* parser);
static AstNode* parse_delete(Parser* parser);
static AstNode* parse_select(Parser* parser);
static AstNode* parse_transaction(Parser* parser);
static AstNode* parse_show_tables(Parser* parser);
static AstNode* parse_describe_table(Parser* parser);

static AstNode* parse_statement(Parser* parser) {
    switch (parser->current_token.type) {
        case TOKEN_CREATE:
            advance(parser);
            /* Check for TABLE or INDEX keyword */
            if (parser->current_token.type == TOKEN_TABLE) {
                advance(parser);
                return parse_create_table(parser);
            } else if (parser->current_token.type == TOKEN_INDEX) {
                advance(parser);
                return parse_create_index(parser);
            }
            parser_set_error(parser, "Expected TABLE or INDEX after CREATE");
            return NULL;

        case TOKEN_DROP:
            advance(parser);
            if (parser->current_token.type == TOKEN_TABLE) {
                advance(parser);
                return parse_drop_table(parser);
            } else if (parser->current_token.type == TOKEN_INDEX) {
                advance(parser);
                return parse_drop_index(parser);
            }
            parser_set_error(parser, "Expected TABLE or INDEX after DROP");
            return NULL;

        case TOKEN_INSERT:
            return parse_insert(parser);

        case TOKEN_UPDATE:
            return parse_update(parser);

        case TOKEN_DELETE:
            return parse_delete(parser);

        case TOKEN_SELECT:
            return parse_select(parser);

        case TOKEN_SHOW:
            return parse_show_tables(parser);

        case TOKEN_DESCRIBE:
        case TOKEN_DESC:
            return parse_describe_table(parser);

        case TOKEN_BEGIN:
        case TOKEN_COMMIT:
        case TOKEN_ROLLBACK:
            return parse_transaction(parser);

        default:
            parser_set_error(parser, "Unexpected token at start of statement");
            return NULL;
    }
}

static ColumnType parse_column_type(Parser* parser) {
    if (parser->current_token.type == TOKEN_INTEGER_KW) {
        advance(parser);
        return COL_TYPE_INTEGER;
    }
    if (parser->current_token.type == TOKEN_REAL_KW || check_keyword(parser, "REAL")) {
        advance(parser);
        return COL_TYPE_FLOAT;
    }
    if (parser->current_token.type == TOKEN_TEXT_KW || check_keyword(parser, "TEXT")) {
        advance(parser);
        return COL_TYPE_TEXT;
    }
    if (parser->current_token.type == TOKEN_BLOB_KW || check_keyword(parser, "BLOB")) {
        advance(parser);
        return COL_TYPE_BLOB;
    }
    /* Fallback: check if identifier is a type keyword (handles case-insensitive matching) */
    if (parser->current_token.type == TOKEN_IDENTIFIER) {
        const char* text = token_get_text(&parser->current_token);
        if (text && strcasecmp(text, "INTEGER") == 0) {
            advance(parser);
            return COL_TYPE_INTEGER;
        }
        if (text && strcasecmp(text, "INT") == 0) {
            advance(parser);
            return COL_TYPE_INTEGER;
        }
        if (text && strcasecmp(text, "FLOAT") == 0) {
            advance(parser);
            return COL_TYPE_FLOAT;
        }
        if (text && strcasecmp(text, "TEXT") == 0) {
            advance(parser);
            return COL_TYPE_TEXT;
        }
        if (text && strcasecmp(text, "REAL") == 0) {
            advance(parser);
            return COL_TYPE_FLOAT;
        }
        if (text && strcasecmp(text, "BLOB") == 0) {
            advance(parser);
            return COL_TYPE_BLOB;
        }
    }
    return COL_TYPE_TEXT; /* Default */
}

static ColumnDef* parse_column_def(Parser* parser) {
    char* name = parse_identifier(parser);
    if (!name) return NULL;

    ColumnDef* col = calloc(1, sizeof(ColumnDef));
    if (!col) {
        free(name);
        return NULL;
    }
    col->name = name;
    col->type = parse_column_type(parser);
    col->not_null = 0;
    col->primary_key = 0;
    col->autoincrement = 0;

    /* Parse optional constraints */
    while (1) {
        if (check_keyword(parser, "NOT")) {
            /* Peek ahead to see if NOT is followed by NULL */
            Token peek = lexer_peek_token(parser->lexer);
            if (peek.type == TOKEN_NULL ||
                (peek.type == TOKEN_IDENTIFIER &&
                 strcasecmp(token_get_text(&peek), "NULL") == 0)) {
                /* NOT NULL */
                advance(parser); /* NOT */
                advance(parser); /* NULL */
                col->not_null = 1;
                continue;
            }
            /* NOT without NULL - this keyword is used elsewhere, break and let caller handle */
        }
        if (check_keyword(parser, "PRIMARY")) {
            advance(parser);
            if (check_keyword(parser, "KEY")) {
                advance(parser);
                col->primary_key = 1;
            }
        } else if (check_keyword(parser, "AUTOINCREMENT")) {
            advance(parser);
            col->autoincrement = 1;
        } else if (check_keyword(parser, "DEFAULT")) {
            advance(parser);
            col->default_value = parse_expression(parser);
        } else {
            break;
        }
    }

    return col;
}

static AstNode* parse_create_table(Parser* parser) {
    /* TABLE was already consumed in parse_statement */
    int if_not_exists = 0;
    if (parser->current_token.type == TOKEN_IF) {
        advance(parser);
        if (parser->current_token.type == TOKEN_NOT) {
            advance(parser);
            if (parser->current_token.type == TOKEN_EXISTS) {
                advance(parser);
                if_not_exists = 1;
            } else {
                parser_set_error(parser, "Expected EXISTS after IF NOT");
                return NULL;
            }
        } else {
            parser_set_error(parser, "Expected NOT after IF");
            return NULL;
        }
    }

    char* table_name = parse_identifier(parser);
    if (!table_name) return NULL;

    expect_token(parser, TOKEN_LPAREN, "Expected '(' after table name");

    /* Parse column definitions */
    ColumnDef* columns = NULL;
    ColumnDef** tail = &columns;

    while (parser->current_token.type != TOKEN_RPAREN && !parser->has_error) {
        ColumnDef* col = parse_column_def(parser);
        if (!col) {
            /* Error */
            while (columns) {
                ColumnDef* next = columns->next;
                free(columns);
                columns = next;
            }
            free(table_name);
            return NULL;
        }
        *tail = col;
        tail = &col->next;

        if (parser->current_token.type == TOKEN_COMMA) {
            advance(parser);
        } else {
            break;
        }
    }

    expect_token(parser, TOKEN_RPAREN, "Expected ')' after column definitions");
    expect_token(parser, TOKEN_SEMICOLON, "Expected ';' after CREATE TABLE");

    AstNode* node = ast_create(AST_CREATE_TABLE, sizeof(AstCreateTable));
    cast_create_table(node)->table_name = table_name;
    cast_create_table(node)->if_not_exists = if_not_exists;
    cast_create_table(node)->columns = columns;
    return node;
}

static AstNode* parse_drop_table(Parser* parser) {
    /* TABLE was already consumed in parse_statement */
    int if_exists = 0;
    if (parser->current_token.type == TOKEN_IF) {
        advance(parser);
        if (parser->current_token.type == TOKEN_EXISTS) {
            advance(parser);
            if_exists = 1;
        } else {
            parser_set_error(parser, "Expected EXISTS after IF");
            return NULL;
        }
    }

    char* table_name = parse_identifier(parser);
    if (!table_name) return NULL;

    expect_token(parser, TOKEN_SEMICOLON, "Expected ';' after DROP TABLE");

    AstNode* node = ast_create(AST_DROP_TABLE, sizeof(AstDropTable));
    cast_drop_table(node)->table_name = table_name;
    cast_drop_table(node)->if_exists = if_exists;
    return node;
}

static AstNode* parse_create_index(Parser* parser) {
    /* INDEX was already consumed in parse_statement */
    char* index_name = NULL;
    int unique = 0;

    if (check_keyword(parser, "UNIQUE")) {
        advance(parser);
        unique = 1;
    }

    index_name = parse_identifier(parser);
    if (!index_name) return NULL;

    if (check_keyword(parser, "ON")) {
        advance(parser);
    } else {
        parser_set_error(parser, "Expected ON in CREATE INDEX");
        free(index_name);
        return NULL;
    }

    char* table_name = parse_identifier(parser);
    if (!table_name) {
        free(index_name);
        return NULL;
    }

    expect_token(parser, TOKEN_LPAREN, "Expected '(' in CREATE INDEX");

    char* column_name = parse_identifier(parser);
    if (!column_name) {
        free(index_name);
        free(table_name);
        return NULL;
    }

    expect_token(parser, TOKEN_RPAREN, "Expected ')' after column name");
    expect_token(parser, TOKEN_SEMICOLON, "Expected ';' after CREATE INDEX");

    AstNode* node = ast_create(AST_CREATE_INDEX, sizeof(AstCreateIndex));
    cast_create_index(node)->index_name = index_name;
    cast_create_index(node)->table_name = table_name;
    cast_create_index(node)->column_name = column_name;
    cast_create_index(node)->unique = unique;
    return node;
}

static AstNode* parse_drop_index(Parser* parser) {
    /* INDEX was already consumed in parse_statement */
    char* index_name = parse_identifier(parser);
    if (!index_name) return NULL;

    expect_token(parser, TOKEN_SEMICOLON, "Expected ';' after DROP INDEX");

    AstNode* node = ast_create(AST_DROP_INDEX, sizeof(AstDropIndex));
    cast_drop_index(node)->index_name = index_name;
    return node;
}

static Expression** parse_expression_list(Parser* parser, int* count) {
    Expression** exprs = NULL;
    int capacity = 4;
    int n = 0;

    *count = 0;

    if (parser->current_token.type == TOKEN_RPAREN) {
        return exprs;
    }

    exprs = malloc(sizeof(Expression*) * capacity);
    if (!exprs) return NULL;

    while (1) {
        if (n >= capacity) {
            capacity *= 2;
            Expression** new_exprs = realloc(exprs, sizeof(Expression*) * capacity);
            if (!new_exprs) {
                for (int i = 0; i < n; i++) expr_unref(exprs[i]);
                free(exprs);
                return NULL;
            }
            exprs = new_exprs;
        }

        exprs[n] = parse_expression(parser);
        if (!exprs[n]) {
            for (int i = 0; i < n; i++) expr_unref(exprs[i]);
            free(exprs);
            return NULL;
        }
        n++;

        if (parser->current_token.type == TOKEN_COMMA) {
            advance(parser);
        } else {
            break;
        }
    }

    *count = n;
    return exprs;
}

static AstNode* parse_insert(Parser* parser) {
    advance(parser); /* INSERT */

    expect_keyword(parser, "INTO", "Expected INTO after INSERT");

    char* table_name = parse_identifier(parser);
    if (!table_name) return NULL;

    ColumnList* columns = NULL;
    ColumnList** col_tail = &columns;

    /* Optional column list */
    if (parser->current_token.type == TOKEN_LPAREN) {
        advance(parser);

        while (parser->current_token.type != TOKEN_RPAREN && !parser->has_error) {
            char* col_name = parse_identifier(parser);
            if (!col_name) {
                free(table_name);
                return NULL;
            }

            ColumnList* col = calloc(1, sizeof(ColumnList));
            col->name = col_name;
            *col_tail = col;
            col_tail = &col->next;

            if (parser->current_token.type == TOKEN_COMMA) {
                advance(parser);
            } else {
                break;
            }
        }

        expect_token(parser, TOKEN_RPAREN, "Expected ')' after column list");
    }

    expect_keyword(parser, "VALUES", "Expected VALUES after INSERT");

    /* Parse value lists */
    ValueList* values = NULL;
    ValueList** val_tail = &values;

    while (!parser->has_error) {
        expect_token(parser, TOKEN_LPAREN, "Expected '(' before VALUES");

        int count = 0;
        Expression** exprs = parse_expression_list(parser, &count);
        if (!exprs) {
            free(table_name);
            /* Free columns */
            while (columns) {
                ColumnList* next = columns->next;
                free(columns->name);
                free(columns);
                columns = next;
            }
            return NULL;
        }

        ValueList* vl = calloc(1, sizeof(ValueList));
        vl->values = exprs;
        vl->count = count;
        *val_tail = vl;
        val_tail = &vl->next;

        expect_token(parser, TOKEN_RPAREN, "Expected ')' after VALUES");

        if (parser->current_token.type == TOKEN_COMMA) {
            advance(parser);
        } else {
            break;
        }
    }

    expect_token(parser, TOKEN_SEMICOLON, "Expected ';' after INSERT");

    AstNode* node = ast_create(AST_INSERT, sizeof(AstInsert));
    cast_insert(node)->table_name = table_name;
    cast_insert(node)->columns = columns;
    cast_insert(node)->values = values;
    return node;
}

static AstNode* parse_update(Parser* parser) {
    advance(parser); /* UPDATE */

    char* table_name = parse_identifier(parser);
    if (!table_name) return NULL;

    expect_keyword(parser, "SET", "Expected SET after UPDATE table name");

    /* Parse SET clauses */
    SetClause* clauses = NULL;
    SetClause** tail = &clauses;

    while (!parser->has_error) {
        char* col_name = parse_identifier(parser);
        if (!col_name) {
            /* Free accumulated clauses */
            while (clauses) {
                SetClause* next = clauses->next;
                free(clauses->column_name);
                expr_unref(clauses->value);
                free(clauses);
                clauses = next;
            }
            free(table_name);
            return NULL;
        }

        expect_token(parser, TOKEN_EQ, "Expected '=' in SET clause");

        Expression* value = parse_expression(parser);
        if (!value) {
            /* Free accumulated clauses */
            while (clauses) {
                SetClause* next = clauses->next;
                free(clauses->column_name);
                expr_unref(clauses->value);
                free(clauses);
                clauses = next;
            }
            free(col_name);
            free(table_name);
            return NULL;
        }

        SetClause* sc = calloc(1, sizeof(SetClause));
        sc->column_name = col_name;
        sc->value = value;
        *tail = sc;
        tail = &sc->next;

        if (parser->current_token.type == TOKEN_COMMA) {
            advance(parser);
        } else {
            break;
        }
    }

    /* WHERE is required */
    Expression* where = NULL;
    if (check_keyword(parser, "WHERE")) {
        advance(parser);
        where = parse_expression(parser);
        if (!where) {
            free(table_name);
            return NULL;
        }
    } else {
        parser_set_error(parser, "UPDATE requires WHERE clause");
        free(table_name);
        return NULL;
    }

    expect_token(parser, TOKEN_SEMICOLON, "Expected ';' after UPDATE");

    AstNode* node = ast_create(AST_UPDATE, sizeof(AstUpdate));
    cast_update(node)->table_name = table_name;
    cast_update(node)->set_clauses = clauses;
    cast_update(node)->where = where;
    return node;
}

static AstNode* parse_delete(Parser* parser) {
    advance(parser); /* DELETE */

    expect_keyword(parser, "FROM", "Expected FROM after DELETE");

    char* table_name = parse_identifier(parser);
    if (!table_name) return NULL;

    /* WHERE is required */
    Expression* where = NULL;
    if (check_keyword(parser, "WHERE")) {
        advance(parser);
        where = parse_expression(parser);
        if (!where) {
            free(table_name);
            return NULL;
        }
    } else {
        parser_set_error(parser, "DELETE requires WHERE clause");
        free(table_name);
        return NULL;
    }

    expect_token(parser, TOKEN_SEMICOLON, "Expected ';' after DELETE");

    AstNode* node = ast_create(AST_DELETE, sizeof(AstDelete));
    cast_delete(node)->table_name = table_name;
    cast_delete(node)->where = where;
    return node;
}

static ColumnList* parse_column_list(Parser* parser) {
    ColumnList* list = NULL;
    ColumnList** tail = &list;

    while (1) {
        if (parser->current_token.type == TOKEN_STAR) {
            /* SELECT * - special case */
            ColumnList* col = calloc(1, sizeof(ColumnList));
            col->name = strdup("*");
            *tail = col;
            tail = &col->next;
            advance(parser);
            break;
        }

        /* Check for literal values (numbers) */
        if (parser->current_token.type == TOKEN_INTEGER || parser->current_token.type == TOKEN_STRING) {
            ColumnList* col = calloc(1, sizeof(ColumnList));
            col->name = strdup(token_get_text(&parser->current_token));
            *tail = col;
            tail = &col->next;
            advance(parser);
            break;
        }

        char* name = parse_identifier(parser);
        if (!name) {
            /* Free what we have */
            while (list) {
                ColumnList* next = list->next;
                free(list->name);
                free(list);
                list = next;
            }
            return NULL;
        }

        ColumnList* col = calloc(1, sizeof(ColumnList));
        col->name = name;

        /* Check for alias */
        if (check_keyword(parser, "AS") || parser->current_token.type == TOKEN_AS) {
            advance(parser);
            char* alias = parse_identifier(parser);
            if (alias) {
                col->alias = alias;
            }
        }

        *tail = col;
        tail = &col->next;

        if (parser->current_token.type == TOKEN_COMMA) {
            advance(parser);
        } else {
            break;
        }
    }

    return list;
}

static OrderByItem* parse_order_by(Parser* parser) {
    OrderByItem* list = NULL;
    OrderByItem** tail = &list;

    while (!parser->has_error) {
        char* name = parse_identifier(parser);
        if (!name) {
            /* Free what we have */
            return NULL;
        }

        int descending = 0;
        if (check_keyword(parser, "DESC")) {
            descending = 1;
            advance(parser);
        } else if (check_keyword(parser, "ASC")) {
            descending = 0;
            advance(parser);
        }

        OrderByItem* item = calloc(1, sizeof(OrderByItem));
        item->column_name = name;
        item->descending = descending;
        *tail = item;
        tail = &item->next;

        if (parser->current_token.type == TOKEN_COMMA) {
            advance(parser);
        } else {
            break;
        }
    }

    return list;
}

static AstNode* parse_select(Parser* parser) {
    advance(parser); /* SELECT */

    int is_distinct = 0;
    if (check_keyword(parser, "DISTINCT")) {
        is_distinct = 1;
        advance(parser);
    }

    /* Parse column list */
    ColumnList* columns = parse_column_list(parser);
    if (!columns) {
        return NULL;
    }

    /* Check for SELECT without FROM (scalar expression like SELECT 1) */
    if (parser->current_token.type != TOKEN_FROM) {
        /* This is a scalar SELECT - create a special AST node */
        AstSelect* select = calloc(1, sizeof(AstSelect));
        select->base.type = AST_SELECT;
        select->columns = columns;
        select->table_name = NULL;  /* No table for scalar SELECT */
        select->where = NULL;
        select->order_by = NULL;
        select->limit = NULL;
        select->offset = NULL;
        select->is_distinct = is_distinct;
        return &select->base;
    }

    /* Has FROM clause - continue with normal parsing */
    advance(parser);

    char* table_name = parse_identifier(parser);
    if (!table_name) {
        return NULL;
    }

    /* Optional alias - look ahead to determine if this identifier is an alias */
    char* alias = NULL;
    if (parser->current_token.type == TOKEN_IDENTIFIER) {
        Token peek = lexer_peek_token(parser->lexer);
        /* Alias is present if next token is AS or if next token is an identifier and
         * not followed by a keyword that would end the FROM clause */
        if (check_keyword(parser, "AS")) {
            /* Explicit AS keyword: SELECT * FROM t AS alias */
            advance(parser); /* AS */
            alias = parse_identifier(parser);
        } else if (peek.type == TOKEN_IDENTIFIER || peek.type == TOKEN_AS) {
            /* Implicit alias: SELECT * FROM t alias - peek ahead to confirm */
            /* Check if next token is a keyword that would end the FROM clause */
            int is_end_keyword = (peek.type == TOKEN_WHERE ||
                                  peek.type == TOKEN_ORDER ||
                                  peek.type == TOKEN_LIMIT ||
                                  peek.type == TOKEN_OFFSET ||
                                  peek.type == TOKEN_SEMICOLON ||
                                  peek.type == TOKEN_COMMA ||
                                  peek.type == TOKEN_EOF);
            if (!is_end_keyword) {
                /* This identifier is an alias */
                alias = parse_identifier(parser);
            }
        }
    }

    /* WHERE clause (optional) */
    Expression* where = NULL;
    if (check_keyword(parser, "WHERE")) {
        advance(parser);
        where = parse_expression(parser);
        if (!where) {
            free(table_name);
            return NULL;
        }
    }

    /* ORDER BY clause (optional) */
    OrderByItem* order_by = NULL;
    if (check_keyword(parser, "ORDER")) {
        advance(parser);
        if (check_keyword(parser, "BY")) {
            advance(parser);
            order_by = parse_order_by(parser);
            if (!order_by) {
                free(table_name);
                return NULL;
            }
        }
    }

    /* LIMIT clause (optional) */
    Expression* limit = NULL;
    if (check_keyword(parser, "LIMIT")) {
        advance(parser);
        limit = parse_expression(parser);
        if (!limit) {
            free(table_name);
            return NULL;
        }
    }

    /* OFFSET clause (optional) */
    Expression* offset = NULL;
    if (check_keyword(parser, "OFFSET")) {
        advance(parser);
        offset = parse_expression(parser);
        if (!offset) {
            free(table_name);
            return NULL;
        }
    }

    expect_token(parser, TOKEN_SEMICOLON, "Expected ';' after SELECT");

    AstNode* node = ast_create(AST_SELECT, sizeof(AstSelect));
    cast_select(node)->columns = columns;
    cast_select(node)->is_distinct = is_distinct;
    cast_select(node)->table_name = table_name;
    cast_select(node)->alias = alias;
    cast_select(node)->where = where;
    cast_select(node)->order_by = order_by;
    cast_select(node)->limit = limit;
    cast_select(node)->offset = offset;
    return node;
}

static AstNode* parse_transaction(Parser* parser) {
    TransactionType tx_type;

    switch (parser->current_token.type) {
        case TOKEN_BEGIN:
            tx_type = TX_BEGIN;
            break;
        case TOKEN_COMMIT:
            tx_type = TX_COMMIT;
            break;
        case TOKEN_ROLLBACK:
            tx_type = TX_ROLLBACK;
            break;
        default:
            parser_set_error(parser, "Invalid transaction statement");
            return NULL;
    }

    advance(parser);

    /* Handle optional TRANSACTION keyword after BEGIN */
    if (tx_type == TX_BEGIN && check_keyword(parser, "TRANSACTION")) {
        advance(parser);
    }

    expect_token(parser, TOKEN_SEMICOLON, "Expected ';' after transaction statement");

    AstNode* node = ast_create(AST_TRANSACTION, sizeof(AstTransaction));
    cast_transaction(node)->transaction_type = tx_type;
    return node;
}

/*============================================================================
 * SHOW TABLES and DESCRIBE TABLE parsing
 *============================================================================*/
static AstNode* parse_show_tables(Parser* parser) {
    advance(parser); /* SHOW or DESCRIBE was already consumed */

    expect_keyword(parser, "TABLES", "Expected TABLES after SHOW");

    AstNode* node = ast_create(AST_SHOW_TABLES, sizeof(AstShowTables));
    return node;
}

static AstNode* parse_describe_table(Parser* parser) {
    advance(parser); /* DESCRIBE was already consumed */

    char* table_name = parse_identifier(parser);
    if (!table_name) return NULL;

    AstNode* node = ast_create(AST_DESCRIBE_TABLE, sizeof(AstDescribeTable));
    cast_describe_table(node)->table_name = table_name;
    return node;
}

/*============================================================================
 * Main parse entry point
 *============================================================================*/
AstNode* parser_parse(Parser* parser) {
    if (!parser || parser->has_error) return NULL;

    AstNode* node = parse_statement(parser);
    return node;
}

void parser_free_ast(Parser* parser, AstNode* node) {
    (void)parser;
    if (node) ast_free(node);
}

/* Helper to expect keyword */
static void expect_keyword(Parser* parser, const char* kw, const char* msg) {
    if (!check_keyword(parser, kw)) {
        parser_set_error(parser, msg);
    } else {
        advance(parser);
    }
}