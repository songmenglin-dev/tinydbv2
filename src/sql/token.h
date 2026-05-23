#ifndef TINYDB_TOKEN_H
#define TINYDB_TOKEN_H

#include <stddef.h>
#include <stdint.h>

/*============================================================================
 * Token types for SQL lexer
 *============================================================================*/
typedef enum {
    /* End/Error */
    TOKEN_EOF = 0,
    TOKEN_ERROR,

    /* Literals */
    TOKEN_IDENTIFIER,       /* table_name, column1 */
    TOKEN_INTEGER,         /* 123, 0xFF */
    TOKEN_REAL,            /* 3.14, 1e-5 */
    TOKEN_STRING,          /* 'hello' */

    /* Operators */
    TOKEN_PLUS,            /* + */
    TOKEN_MINUS,          /* - */
    TOKEN_STAR,            /* * */
    TOKEN_SLASH,          /* / */
    TOKEN_PERCENT,         /* % */
    TOKEN_EQ,              /* = */
    TOKEN_NEQ,             /* != or <> */
    TOKEN_LT,              /* < */
    TOKEN_GT,              /* > */
    TOKEN_LTE,             /* <= */
    TOKEN_GTE,             /* >= */
    TOKEN_DOUBLE_EQ,       /* == */
    TOKEN_LIKE_OP,         /* LIKE */
    TOKEN_IN_OP,          /* IN */
    TOKEN_BETWEEN_OP,      /* BETWEEN */

    /* Keywords */
    TOKEN_SELECT,
    TOKEN_FROM,
    TOKEN_WHERE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_IN,
    TOKEN_LIKE,
    TOKEN_BETWEEN,
    TOKEN_IS,
    TOKEN_NULL,

    TOKEN_INSERT,
    TOKEN_INTO,
    TOKEN_VALUES,
    TOKEN_UPDATE,
    TOKEN_SET,

    TOKEN_DELETE,
    TOKEN_CREATE,
    TOKEN_DROP,
    TOKEN_TABLE,
    TOKEN_INDEX,
    TOKEN_IF,
    TOKEN_EXISTS,

    TOKEN_BEGIN,
    TOKEN_COMMIT,
    TOKEN_ROLLBACK,
    TOKEN_TRANSACTION,

    TOKEN_ORDER,
    TOKEN_BY,
    TOKEN_ASC,
    TOKEN_DESC,
    TOKEN_LIMIT,
    TOKEN_OFFSET,
    TOKEN_DISTINCT,
    TOKEN_AS,
    TOKEN_ON,
    TOKEN_PRIMARY,
    TOKEN_KEY,
    TOKEN_UNIQUE,
    TOKEN_DEFAULT,
    TOKEN_AUTOINCREMENT,
    TOKEN_SHOW,
    TOKEN_DESCRIBE,

    /* Data types */
    TOKEN_INTEGER_KW,
    TOKEN_REAL_KW,
    TOKEN_TEXT_KW,
    TOKEN_BLOB_KW,

    /* Special tokens */
    TOKEN_LPAREN,           /* ( */
    TOKEN_RPAREN,           /* ) */
    TOKEN_COMMA,            /* , */
    TOKEN_DOT,              /* . */
    TOKEN_SEMICOLON,        /* ; */
    TOKEN_QUESTION,        /* ? */
    TOKEN_PLACEHOLDER       /* $1, $2 */
} TokenType;

/*============================================================================
 * Token structure
 *============================================================================*/
typedef struct {
    TokenType type;
    char* lexeme;           /* Original text */
    size_t lexeme_len;
    int line;
    int column;

    union {
        int64_t integer_value;
        double float_value;
        struct {
            char* value;
            size_t length;
        } string_value;
    } value;
} Token;

/*============================================================================
 * Token helpers
 *============================================================================*/
const char* token_type_name(TokenType type);
const char* token_get_text(Token* token);

#endif /* TINYDB_TOKEN_H */