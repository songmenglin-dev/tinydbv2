#define _DEFAULT_SOURCE
#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stddef.h>

/*============================================================================
 * Lexer structure
 *============================================================================*/
struct Lexer {
    const char* sql;
    size_t sql_len;
    size_t pos;           /* Current position in SQL string */
    int line;             /* Current line number */
    int column;           /* Current column number */
    Token current_token;
    Token peek_token;
    int peeked;           /* Whether we have a peeked token */
    char error[256];
    int error_line;
    int error_column;
};

/* Forward declarations */
static Token next_token_internal(Lexer* lexer);
static int is_alpha(char c);
static int is_digit(char c);
static int is_alphanumeric(char c);

/* Keywords map - must be alphabetically sorted for binary search */
static struct {
    const char* keyword;
    TokenType type;
} keywords[] = {
    {"AND", TOKEN_AND},
    {"AS", TOKEN_AS},
    {"ASC", TOKEN_ASC},
    {"AUTOINCREMENT", TOKEN_AUTOINCREMENT},
    {"BEGIN", TOKEN_BEGIN},
    {"BETWEEN", TOKEN_BETWEEN},
    {"BY", TOKEN_BY},
    {"COMMIT", TOKEN_COMMIT},
    {"CREATE", TOKEN_CREATE},
    {"DEFAULT", TOKEN_DEFAULT},
    {"DELETE", TOKEN_DELETE},
    {"DESC", TOKEN_DESC},
    {"DISTINCT", TOKEN_DISTINCT},
    {"DROP", TOKEN_DROP},
    {"EXISTS", TOKEN_EXISTS},
    {"FROM", TOKEN_FROM},
    {"IF", TOKEN_IF},
    {"IN", TOKEN_IN},
    {"INDEX", TOKEN_INDEX},
    {"INSERT", TOKEN_INSERT},
    {"INTO", TOKEN_INTO},
    {"IS", TOKEN_IS},
    {"KEY", TOKEN_KEY},
    {"LIKE", TOKEN_LIKE},
    {"LIMIT", TOKEN_LIMIT},
    {"NOT", TOKEN_NOT},
    {"NULL", TOKEN_NULL},
    {"OFFSET", TOKEN_OFFSET},
    {"ON", TOKEN_ON},
    {"OR", TOKEN_OR},
    {"ORDER", TOKEN_ORDER},
    {"PRIMARY", TOKEN_PRIMARY},
    {"REAL", TOKEN_REAL_KW},
    {"ROLLBACK", TOKEN_ROLLBACK},
    {"SELECT", TOKEN_SELECT},
    {"SET", TOKEN_SET},
    {"TABLE", TOKEN_TABLE},
    {"TEXT", TOKEN_TEXT_KW},
    {"TRANSACTION", TOKEN_TRANSACTION},
    {"UNIQUE", TOKEN_UNIQUE},
    {"UPDATE", TOKEN_UPDATE},
    {"VALUES", TOKEN_VALUES},
    {"WHERE", TOKEN_WHERE},
    {"INTEGER", TOKEN_INTEGER_KW},
    {"BLOB", TOKEN_BLOB_KW}
};
static const int keyword_count = sizeof(keywords) / sizeof(keywords[0]);

/*============================================================================
 * Lexer lifecycle
 *============================================================================*/
Lexer* lexer_create(const char* sql, size_t len) {
    if (!sql || len == 0) return NULL;

    Lexer* lexer = calloc(1, sizeof(Lexer));
    if (!lexer) return NULL;

    lexer->sql = sql;
    lexer->sql_len = len;
    lexer->pos = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->peeked = 0;

    return lexer;
}

void lexer_destroy(Lexer* lexer) {
    if (lexer) {
        free(lexer->current_token.lexeme);
        free(lexer->peek_token.lexeme);
        free(lexer);
    }
}

/*============================================================================
 * Character access
 *============================================================================*/
static char current_char(Lexer* lexer) {
    if (lexer->pos >= lexer->sql_len) return '\0';
    return lexer->sql[lexer->pos];
}

static char peek_char(Lexer* lexer) {
    if (lexer->pos + 1 >= lexer->sql_len) return '\0';
    return lexer->sql[lexer->pos + 1];
}

static void advance(Lexer* lexer) {
    if (lexer->pos < lexer->sql_len) {
        if (lexer->sql[lexer->pos] == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
        lexer->pos++;
    }
}

/*============================================================================
 * Character classification
 *============================================================================*/
static int is_alpha(char c) {
    return isalpha((unsigned char)c);
}

static int is_digit(char c) {
    return isdigit((unsigned char)c);
}

static int is_alphanumeric(char c) {
    return is_alpha(c) || is_digit(c);
}

static int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int is_identifier_start(char c) {
    return is_alpha(c) || c == '_';
}

static int is_identifier_char(char c) {
    return is_alphanumeric(c) || c == '_';
}

/*============================================================================
 * Keyword lookup (binary search)
 *============================================================================*/
static TokenType lookup_keyword(const char* text, size_t len) {
    int low = 0;
    int high = keyword_count - 1;

    while (low <= high) {
        int mid = (low + high) / 2;
        size_t kw_len = strlen(keywords[mid].keyword);
        size_t min_len = len < kw_len ? len : kw_len;
        int cmp = strncasecmp(text, keywords[mid].keyword, min_len);

        if (cmp == 0) {
            /* Prefixes match - shorter string comes first */
            if (len < kw_len) {
                cmp = -1;
            } else if (len > kw_len) {
                cmp = 1;
            }
        }

        if (cmp == 0) {
            return keywords[mid].type;
        } else if (cmp < 0) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    return TOKEN_IDENTIFIER;
}

/*============================================================================
 * Error reporting
 *============================================================================*/
static void set_error(Lexer* lexer, const char* msg) {
    snprintf(lexer->error, sizeof(lexer->error), "%s", msg);
    lexer->error_line = lexer->line;
    lexer->error_column = lexer->column;
}

const char* lexer_error(Lexer* lexer) {
    if (lexer && lexer->error[0]) return lexer->error;
    return NULL;
}

int lexer_error_line(Lexer* lexer) {
    return lexer ? lexer->error_line : 0;
}

int lexer_error_column(Lexer* lexer) {
    return lexer ? lexer->error_column : 0;
}

/*============================================================================
 * Token creation helpers
 *============================================================================*/
static Token make_token(Lexer* lexer, TokenType type, const char* start, size_t len) {
    Token token;
    memset(&token, 0, sizeof(token));
    token.type = type;
    token.line = lexer->line;
    token.column = lexer->column - (int)len;
    token.lexeme = NULL;
    token.lexeme_len = len;

    if (start && len > 0) {
        token.lexeme = malloc(len + 1);
        if (token.lexeme) {
            memcpy(token.lexeme, start, len);
            token.lexeme[len] = '\0';
        }
    }

    return token;
}

static Token make_error_token(Lexer* lexer, const char* msg) {
    Token token = make_token(lexer, TOKEN_ERROR, NULL, 0);
    set_error(lexer, msg);
    return token;
}

/*============================================================================
 * Skip whitespace
 *============================================================================*/
static void skip_whitespace(Lexer* lexer) {
    while (is_whitespace(current_char(lexer))) {
        advance(lexer);
    }
}

/*============================================================================
 * Scan identifier or keyword
 *============================================================================*/
static Token scan_identifier(Lexer* lexer) {
    size_t start = lexer->pos;

    /* identifier_start: [a-zA-Z_] */
    if (!is_identifier_start(current_char(lexer))) {
        return make_error_token(lexer, "Invalid identifier start");
    }

    advance(lexer);

    /* identifier_chars: [a-zA-Z0-9_]* */
    while (is_identifier_char(current_char(lexer))) {
        advance(lexer);
    }

    size_t len = lexer->pos - start;
    const char* text = lexer->sql + start;

    /* Uppercase for keyword lookup (SQL is case-insensitive for keywords) */
    char* upper = malloc(len + 1);
    if (!upper) return make_error_token(lexer, "Out of memory");
    for (size_t i = 0; i < len; i++) {
        upper[i] = (char)toupper((unsigned char)text[i]);
    }
    upper[len] = '\0';

    TokenType type = lookup_keyword(upper, len);
    free(upper);

    return make_token(lexer, type, text, len);
}

/*============================================================================
 * Scan number (integer or real)
 *============================================================================*/
static Token scan_number(Lexer* lexer) {
    size_t start = lexer->pos;
    int start_column = lexer->column;

    /* Check for hex prefix */
    int is_hex = 0;
    if (current_char(lexer) == '0' && (peek_char(lexer) == 'x' || peek_char(lexer) == 'X')) {
        is_hex = 1;
        advance(lexer); /* skip '0' */
        advance(lexer); /* skip 'x' */
    }

    /* Scan digits */
    int has_dot = 0;
    int has_e = 0;

    if (is_hex) {
        /* Hex number */
        while (isxdigit((unsigned char)current_char(lexer))) {
            advance(lexer);
        }
    } else {
        /* Decimal or real */
        /* Leading digits */
        while (is_digit(current_char(lexer))) {
            advance(lexer);
        }

        /* Decimal point */
        if (current_char(lexer) == '.' && is_digit(peek_char(lexer))) {
            has_dot = 1;
            advance(lexer);
            while (is_digit(current_char(lexer))) {
                advance(lexer);
            }
        }

        /* Exponent (for both decimal and real numbers) */
        if (current_char(lexer) == 'e' || current_char(lexer) == 'E') {
            has_e = 1;
            advance(lexer);
            if (current_char(lexer) == '+' || current_char(lexer) == '-') {
                advance(lexer);
            }
            while (is_digit(current_char(lexer))) {
                advance(lexer);
            }
        }
    }

    size_t len = lexer->pos - start;
    const char* text = lexer->sql + start;
    lexer->column = start_column;

    Token token = make_token(lexer, has_dot || has_e ? TOKEN_REAL : TOKEN_INTEGER, text, len);

    /* Parse the numeric value */
    if (token.lexeme) {
        if (is_hex) {
            /* Parse hexadecimal */
            int64_t val = 0;
            const char* p = token.lexeme + 2; /* skip '0x' */
            while (*p) {
                char c = *p;
                int digit = 0;
                if (c >= '0' && c <= '9') digit = c - '0';
                else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
                else break;
                val = (val << 4) | digit;
                p++;
            }
            token.value.integer_value = val;
        } else if (has_dot || has_e) {
            /* Parse double */
            token.value.float_value = atof(token.lexeme);
        } else {
            /* Parse integer */
            token.value.integer_value = atoll(token.lexeme);
        }
    }

    return token;
}

/*============================================================================
 * Scan string (single-quoted SQL string)
 *============================================================================*/
static Token scan_string(Lexer* lexer) {
    size_t start = lexer->pos;
    int start_column = lexer->column;

    char quote = current_char(lexer); /* should be ' */
    advance(lexer); /* skip opening quote */

    /* Scan string content, handling '' escape sequence */
    size_t content_len = 0;
    size_t capacity = 16;
    char* content = malloc(capacity);
    if (!content) return make_error_token(lexer, "Out of memory");

    while (1) {
        if (current_char(lexer) == '\0') {
            free(content);
            lexer->column = start_column;
            return make_error_token(lexer, "Unterminated string");
        }

        if (current_char(lexer) == quote) {
            /* Check for '' escape sequence */
            if (peek_char(lexer) == quote) {
                /* Escaped quote - add single quote to result */
                if (content_len + 1 >= capacity) {
                    capacity *= 2;
                    char* new_content = realloc(content, capacity);
                    if (!new_content) {
                        free(content);
                        return make_error_token(lexer, "Out of memory");
                    }
                    content = new_content;
                }
                content[content_len++] = '\'';
                advance(lexer); /* skip first quote */
                advance(lexer); /* skip second quote */
            } else {
                /* End of string */
                advance(lexer); /* skip closing quote */
                break;
            }
        } else {
            /* Regular character */
            if (content_len + 1 >= capacity) {
                capacity *= 2;
                char* new_content = realloc(content, capacity);
                if (!new_content) {
                    free(content);
                    return make_error_token(lexer, "Out of memory");
                }
                content = new_content;
            }
            content[content_len++] = current_char(lexer);
            advance(lexer);
        }
    }

    lexer->column = start_column;

    Token token = make_token(lexer, TOKEN_STRING, lexer->sql + start, lexer->pos - start);
    token.value.string_value.value = content;
    token.value.string_value.length = content_len;

    return token;
}

/*============================================================================
 * Scan operators
 *============================================================================*/
static Token scan_operator(Lexer* lexer) {
    char c = current_char(lexer);

    switch (c) {
        case '+': advance(lexer); return make_token(lexer, TOKEN_PLUS, "+", 1);
        case '-': advance(lexer); return make_token(lexer, TOKEN_MINUS, "-", 1);
        case '*': advance(lexer); return make_token(lexer, TOKEN_STAR, "*", 1);
        case '/': advance(lexer); return make_token(lexer, TOKEN_SLASH, "/", 1);
        case '%': advance(lexer); return make_token(lexer, TOKEN_PERCENT, "%", 1);
        case '(': advance(lexer); return make_token(lexer, TOKEN_LPAREN, "(", 1);
        case ')': advance(lexer); return make_token(lexer, TOKEN_RPAREN, ")", 1);
        case ',': advance(lexer); return make_token(lexer, TOKEN_COMMA, ",", 1);
        case '.': advance(lexer); return make_token(lexer, TOKEN_DOT, ".", 1);
        case ';': advance(lexer); return make_token(lexer, TOKEN_SEMICOLON, ";", 1);
        case '?': advance(lexer); return make_token(lexer, TOKEN_QUESTION, "?", 1);

        case '=': advance(lexer); return make_token(lexer, TOKEN_EQ, "=", 1);

        case '<':
            advance(lexer);
            if (current_char(lexer) == '=') {
                advance(lexer);
                return make_token(lexer, TOKEN_LTE, "<=", 2);
            } else if (current_char(lexer) == '>') {
                advance(lexer);
                return make_token(lexer, TOKEN_NEQ, "<>", 2);
            } else {
                return make_token(lexer, TOKEN_LT, "<", 1);
            }

        case '>':
            advance(lexer);
            if (current_char(lexer) == '=') {
                advance(lexer);
                return make_token(lexer, TOKEN_GTE, ">=", 2);
            } else {
                return make_token(lexer, TOKEN_GT, ">", 1);
            }

        case '!':
            advance(lexer);
            if (current_char(lexer) == '=') {
                advance(lexer);
                return make_token(lexer, TOKEN_NEQ, "!=", 2);
            } else {
                return make_error_token(lexer, "Unexpected character '!'");
            }

        default:
            advance(lexer);
            return make_error_token(lexer, "Unexpected character");
    }
}

/*============================================================================
 * Main token scanning
 *============================================================================*/
static Token next_token_internal(Lexer* lexer) {
    skip_whitespace(lexer);

    if (lexer->pos >= lexer->sql_len) {
        return make_token(lexer, TOKEN_EOF, "", 0);
    }

    char c = current_char(lexer);

    /* Identifier or keyword */
    if (is_identifier_start(c)) {
        return scan_identifier(lexer);
    }

    /* Number */
    if (is_digit(c) || (c == '0' && isxdigit(peek_char(lexer)))) {
        return scan_number(lexer);
    }

    /* String */
    if (c == '\'' || c == '"') {
        return scan_string(lexer);
    }

    /* Placeholder $1, $2, etc. */
    if (c == '$') {
        size_t start = lexer->pos;
        advance(lexer);
        if (is_digit(current_char(lexer))) {
            while (is_digit(current_char(lexer))) {
                advance(lexer);
            }
            return make_token(lexer, TOKEN_PLACEHOLDER, lexer->sql + start, lexer->pos - start);
        }
        return make_error_token(lexer, "Invalid placeholder");
    }

    /* Operators and punctuation */
    return scan_operator(lexer);
}

/*============================================================================
 * Public token interface
 *============================================================================*/
Token lexer_next_token(Lexer* lexer) {
    if (!lexer) {
        Token t = {0};
        t.type = TOKEN_ERROR;
        return t;
    }

    /* Return peeked token if available */
    if (lexer->peeked) {
        lexer->peeked = 0;
        return lexer->peek_token;
    }

    /* Free previous current token lexeme */
    free(lexer->current_token.lexeme);
    lexer->current_token = next_token_internal(lexer);
    return lexer->current_token;
}

Token lexer_peek_token(Lexer* lexer) {
    if (!lexer) {
        Token t = {0};
        t.type = TOKEN_ERROR;
        return t;
    }

    if (!lexer->peeked) {
        lexer->peek_token = next_token_internal(lexer);
        lexer->peeked = 1;
    }

    return lexer->peek_token;
}

void lexer_advance(Lexer* lexer) {
    (void)lexer_next_token(lexer); /* Ignore the return, just advance */
}

/*============================================================================
 * Token type name (for debugging)
 *============================================================================*/
const char* token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_INTEGER: return "INTEGER";
        case TOKEN_REAL: return "REAL";
        case TOKEN_STRING: return "STRING";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_PERCENT: return "PERCENT";
        case TOKEN_EQ: return "EQ";
        case TOKEN_NEQ: return "NEQ";
        case TOKEN_LT: return "LT";
        case TOKEN_GT: return "GT";
        case TOKEN_LTE: return "LTE";
        case TOKEN_GTE: return "GTE";
        case TOKEN_LIKE_OP: return "LIKE";
        case TOKEN_IN_OP: return "IN";
        case TOKEN_BETWEEN_OP: return "BETWEEN";
        case TOKEN_SELECT: return "SELECT";
        case TOKEN_FROM: return "FROM";
        case TOKEN_WHERE: return "WHERE";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_NOT: return "NOT";
        case TOKEN_IN: return "IN";
        case TOKEN_LIKE: return "LIKE";
        case TOKEN_BETWEEN: return "BETWEEN";
        case TOKEN_IS: return "IS";
        case TOKEN_NULL: return "NULL";
        case TOKEN_INSERT: return "INSERT";
        case TOKEN_INTO: return "INTO";
        case TOKEN_VALUES: return "VALUES";
        case TOKEN_UPDATE: return "UPDATE";
        case TOKEN_SET: return "SET";
        case TOKEN_DELETE: return "DELETE";
        case TOKEN_CREATE: return "CREATE";
        case TOKEN_DROP: return "DROP";
        case TOKEN_TABLE: return "TABLE";
        case TOKEN_INDEX: return "INDEX";
        case TOKEN_IF: return "IF";
        case TOKEN_EXISTS: return "EXISTS";
        case TOKEN_BEGIN: return "BEGIN";
        case TOKEN_COMMIT: return "COMMIT";
        case TOKEN_ROLLBACK: return "ROLLBACK";
        case TOKEN_TRANSACTION: return "TRANSACTION";
        case TOKEN_ORDER: return "ORDER";
        case TOKEN_BY: return "BY";
        case TOKEN_ASC: return "ASC";
        case TOKEN_DESC: return "DESC";
        case TOKEN_LIMIT: return "LIMIT";
        case TOKEN_OFFSET: return "OFFSET";
        case TOKEN_DISTINCT: return "DISTINCT";
        case TOKEN_AS: return "AS";
        case TOKEN_ON: return "ON";
        case TOKEN_PRIMARY: return "PRIMARY";
        case TOKEN_KEY: return "KEY";
        case TOKEN_UNIQUE: return "UNIQUE";
        case TOKEN_DEFAULT: return "DEFAULT";
        case TOKEN_AUTOINCREMENT: return "AUTOINCREMENT";
        case TOKEN_INTEGER_KW: return "INTEGER";
        case TOKEN_REAL_KW: return "REAL";
        case TOKEN_TEXT_KW: return "TEXT";
        case TOKEN_BLOB_KW: return "BLOB";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_DOT: return "DOT";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_QUESTION: return "QUESTION";
        case TOKEN_PLACEHOLDER: return "PLACEHOLDER";
        default: return "UNKNOWN";
    }
}

const char* token_get_text(Token* token) {
    return token->lexeme ? token->lexeme : "";
}