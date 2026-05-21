#ifndef TINYDB_LEXER_TEST_H
#define TINYDB_LEXER_TEST_H

#include "mini_test.h"
#include "../../src/sql/lexer.h"
#include "../../src/sql/token.h"

/*============================================================================
 * Lexer helper functions
 *============================================================================*/
static void test_lexer_basic_tokens(void) {
    /* Test simple keywords and identifiers */
    Lexer* lexer = lexer_create("SELECT * FROM table1", 21);
    assert_non_null(lexer);

    Token token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_SELECT);
    assert_str_eq(token_get_text(&token), "SELECT");

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_STAR);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_FROM);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_IDENTIFIER);
    assert_str_eq(token_get_text(&token), "table1");

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_EOF);

    lexer_destroy(lexer);
}

static void test_lexer_integer_tokens(void) {
    Lexer* lexer = lexer_create("42 123456789 0xFF", 17);
    assert_non_null(lexer);

    Token token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_INTEGER);
    assert_eq(token.value.integer_value, 42);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_INTEGER);
    assert_eq(token.value.integer_value, 123456789);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_INTEGER);
    assert_eq(token.value.integer_value, 255);  /* 0xFF = 255 */

    lexer_destroy(lexer);
}

static void test_lexer_real_tokens(void) {
    Lexer* lexer = lexer_create("3.14 1.5e-3 2.0E+10", 20);
    assert_non_null(lexer);

    Token token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_REAL);
    assert_double_eq(token.value.float_value, 3.14);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_REAL);
    assert_double_eq(token.value.float_value, 0.0015);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_REAL);
    assert_double_eq(token.value.float_value, 20000000000.0);

    lexer_destroy(lexer);
}

static void test_lexer_string_tokens(void) {
    Lexer* lexer = lexer_create("'hello' 'world' ''", 19);
    assert_non_null(lexer);

    Token token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_STRING);
    assert_str_eq(token.value.string_value.value, "hello");
    assert_eq(token.value.string_value.length, 5);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_STRING);
    assert_str_eq(token.value.string_value.value, "world");

    /* SQL standard: '' becomes ' */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_STRING);
    assert_str_eq(token.value.string_value.value, "'");
    assert_eq(token.value.string_value.length, 1);

    lexer_destroy(lexer);
}

static void test_lexer_operators(void) {
    Lexer* lexer = lexer_create("+ - * / % = != <> < > <= >=", 23);
    assert_non_null(lexer);

    Token token;

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_PLUS);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_MINUS);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_STAR);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_SLASH);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_PERCENT);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_EQ);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_NEQ);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_NEQ);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_LT);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_GT);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_LTE);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_GTE);

    lexer_destroy(lexer);
}

static void test_lexer_punctuation(void) {
    Lexer* lexer = lexer_create("( ) , . ;", 8);
    assert_non_null(lexer);

    Token token;

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_LPAREN);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_RPAREN);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_COMMA);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_DOT);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_SEMICOLON);

    lexer_destroy(lexer);
}

static void test_lexer_keywords(void) {
    /* Test all keywords are recognized */
    const char* sql = "SELECT INSERT UPDATE DELETE CREATE DROP TABLE INDEX FROM WHERE AND OR NOT IN LIKE BETWEEN IS NULL BEGIN COMMIT ROLLBACK ORDER BY ASC DESC LIMIT OFFSET DISTINCT INTO VALUES SET IF EXISTS PRIMARY KEY UNIQUE DEFAULT AUTOINCREMENT";
    size_t sql_len = strlen(sql);

    Lexer* lexer = lexer_create(sql, sql_len);
    assert_non_null(lexer);

    Token token;

    /* SELECT */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_SELECT);

    /* INSERT */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_INSERT);

    /* UPDATE */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_UPDATE);

    /* DELETE */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_DELETE);

    /* CREATE */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_CREATE);

    /* DROP */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_DROP);

    /* TABLE */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_TABLE);

    /* INDEX */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_INDEX);

    /* FROM */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_FROM);

    /* WHERE */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_WHERE);

    /* AND */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_AND);

    /* OR */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_OR);

    /* NOT */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_NOT);

    /* IN */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_IN);

    /* LIKE */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_LIKE);

    /* BETWEEN */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_BETWEEN);

    /* IS */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_IS);

    /* NULL */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_NULL);

    /* BEGIN */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_BEGIN);

    /* COMMIT */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_COMMIT);

    /* ROLLBACK */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_ROLLBACK);

    /* ORDER */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_ORDER);

    /* BY */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_BY);

    /* ASC */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_ASC);

    /* DESC */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_DESC);

    /* LIMIT */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_LIMIT);

    /* OFFSET */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_OFFSET);

    /* DISTINCT */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_DISTINCT);

    /* INTO */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_INTO);

    /* VALUES */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_VALUES);

    /* SET */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_SET);

    /* IF */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_IF);

    /* EXISTS */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_EXISTS);

    /* PRIMARY */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_PRIMARY);

    /* KEY */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_KEY);

    /* UNIQUE */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_UNIQUE);

    /* DEFAULT */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_DEFAULT);

    /* AUTOINCREMENT */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_AUTOINCREMENT);

    /* EOF */
    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_EOF);

    lexer_destroy(lexer);
}

static void test_lexer_identifiers(void) {
    /* Test identifiers including those that look like keywords but aren't */
    Lexer* lexer = lexer_create("my_table row_id some_function", 31);
    assert_non_null(lexer);

    Token token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_IDENTIFIER);
    assert_str_eq(token_get_text(&token), "my_table");

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_IDENTIFIER);
    assert_str_eq(token_get_text(&token), "row_id");

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_IDENTIFIER);
    assert_str_eq(token_get_text(&token), "some_function");

    lexer_destroy(lexer);
}

static void test_lexer_line_column_tracking(void) {
    /* Test that line and column are tracked correctly */
    Lexer* lexer = lexer_create("SELECT\nFROM\nWHERE", 20);
    assert_non_null(lexer);

    Token token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_SELECT);
    assert_eq(token.line, 1);
    assert_eq(token.column, 1);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_FROM);
    assert_eq(token.line, 2);
    assert_eq(token.column, 1);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_WHERE);
    assert_eq(token.line, 3);
    assert_eq(token.column, 1);

    lexer_destroy(lexer);
}

static void test_lexer_error_handling(void) {
    /* Test lexer with invalid input */
    Lexer* lexer = lexer_create("SELECT @invalid", 14);
    assert_non_null(lexer);

    Token token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_SELECT);

    token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_ERROR);

    lexer_destroy(lexer);
}

static void test_lexer_peek(void) {
    /* Test lexer peek functionality */
    Lexer* lexer = lexer_create("SELECT * FROM", 14);
    assert_non_null(lexer);

    Token peeked = lexer_peek_token(lexer);
    assert_eq(peeked.type, TOKEN_SELECT);

    /* Peek again - should be the same */
    peeked = lexer_peek_token(lexer);
    assert_eq(peeked.type, TOKEN_SELECT);

    /* Now advance and peek should still work */
    Token token = lexer_next_token(lexer);
    assert_eq(token.type, TOKEN_SELECT);

    peeked = lexer_peek_token(lexer);
    assert_eq(peeked.type, TOKEN_STAR);

    lexer_destroy(lexer);
}

/*============================================================================
 * Test registration
 *============================================================================*/
#define REGISTER_LEXER_TESTS() \
    register_test(test_lexer_basic_tokens, "lexer_basic_tokens"); \
    register_test(test_lexer_integer_tokens, "lexer_integer_tokens"); \
    register_test(test_lexer_real_tokens, "lexer_real_tokens"); \
    register_test(test_lexer_string_tokens, "lexer_string_tokens"); \
    register_test(test_lexer_operators, "lexer_operators"); \
    register_test(test_lexer_punctuation, "lexer_punctuation"); \
    register_test(test_lexer_keywords, "lexer_keywords"); \
    register_test(test_lexer_identifiers, "lexer_identifiers"); \
    register_test(test_lexer_line_column_tracking, "lexer_line_column_tracking"); \
    register_test(test_lexer_error_handling, "lexer_error_handling"); \
    register_test(test_lexer_peek, "lexer_peek")

#endif /* TINYDB_LEXER_TEST_H */