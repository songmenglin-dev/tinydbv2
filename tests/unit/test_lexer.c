#include "../../tests/unit/mini_test.h"
#include "sql/lexer.h"
#include "sql/token.h"
#include <string.h>

/* Test helper to get token text */
static const char* tok_text(Token* t) {
    return t->lexeme ? t->lexeme : "";
}

/* Test basic lexer creation and tokenization */
test(lexer_create_destroy) {
    const char* sql = "SELECT * FROM users";
    Lexer* lex = lexer_create(sql, strlen(sql));
    assert_non_null(lex);
    lexer_destroy(lex);
}

test(lexer_simple_select) {
    const char* sql = "SELECT * FROM users";
    Lexer* lex = lexer_create(sql, strlen(sql));
    assert_non_null(lex);

    Token t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_SELECT);
    assert_str_eq(tok_text(&t), "SELECT");

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_STAR);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_FROM);
    assert_str_eq(tok_text(&t), "FROM");

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_IDENTIFIER);
    assert_str_eq(tok_text(&t), "users");

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_EOF);

    lexer_destroy(lex);
}

test(lexer_integer_tokens) {
    const char* sql = "123 456 0xFF";
    Lexer* lex = lexer_create(sql, strlen(sql));
    assert_non_null(lex);

    Token t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_INTEGER);
    assert_eq(t.value.integer_value, 123);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_INTEGER);
    assert_eq(t.value.integer_value, 456);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_INTEGER);
    assert_eq(t.value.integer_value, 255);  /* 0xFF = 255 */

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_EOF);

    lexer_destroy(lex);
}

test(lexer_real_tokens) {
    const char* sql = "3.14 1.5e-3";
    Lexer* lex = lexer_create(sql, strlen(sql));
    assert_non_null(lex);

    Token t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_REAL);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_REAL);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_EOF);

    lexer_destroy(lex);
}

test(lexer_string_token) {
    const char* sql = "'hello world'";
    Lexer* lex = lexer_create(sql, strlen(sql));
    assert_non_null(lex);

    Token t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_STRING);
    assert_eq(t.value.string_value.length, 11);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_EOF);

    lexer_destroy(lex);
}

test(lexer_string_escape) {
    const char* sql = "'hello''world'";
    Lexer* lex = lexer_create(sql, strlen(sql));
    assert_non_null(lex);

    Token t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_STRING);
    /* 'hello''world' should become "hello'world" which is 11 chars:
     * h e l l o ' w o r l d
     * 5 + 1 + 5 = 11
     */
    assert_eq(t.value.string_value.length, 11);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_EOF);

    lexer_destroy(lex);
}

test(lexer_operators) {
    const char* sql = "= != <> < > <= >=";
    Lexer* lex = lexer_create(sql, strlen(sql));
    assert_non_null(lex);

    Token t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_EQ);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_NEQ);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_NEQ);  /* <> also maps to TOKEN_NEQ */

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_LT);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_GT);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_LTE);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_GTE);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_EOF);

    lexer_destroy(lex);
}

test(lexer_keywords_case_insensitive) {
    const char* sql = "select SELECT Select";
    Lexer* lex = lexer_create(sql, strlen(sql));
    assert_non_null(lex);

    Token t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_SELECT);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_SELECT);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_SELECT);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_EOF);

    lexer_destroy(lex);
}

test(lexer_punctuation) {
    const char* sql = "( ), ;";
    Lexer* lex = lexer_create(sql, strlen(sql));
    assert_non_null(lex);

    Token t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_LPAREN);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_RPAREN);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_COMMA);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_SEMICOLON);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_EOF);

    lexer_destroy(lex);
}

test(lexer_complex_sql) {
    const char* sql = "SELECT id, name FROM users WHERE age > 18 ORDER BY name ASC LIMIT 10;";
    Lexer* lex = lexer_create(sql, strlen(sql));
    assert_non_null(lex);

    Token t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_SELECT);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_IDENTIFIER);  /* id */

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_COMMA);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_IDENTIFIER);  /* name */

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_FROM);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_IDENTIFIER);  /* users */

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_WHERE);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_IDENTIFIER);  /* age */

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_GT);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_INTEGER);  /* 18 */

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_ORDER);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_BY);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_IDENTIFIER);  /* name */

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_ASC);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_LIMIT);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_INTEGER);  /* 10 */

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_SEMICOLON);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_EOF);

    lexer_destroy(lex);
}

test(lexer_peek) {
    const char* sql = "SELECT * FROM t";
    Lexer* lex = lexer_create(sql, strlen(sql));
    assert_non_null(lex);

    Token peek = lexer_peek_token(lex);
    assert_eq(peek.type, TOKEN_SELECT);

    Token first = lexer_next_token(lex);
    assert_eq(first.type, TOKEN_SELECT);

    Token second = lexer_next_token(lex);
    assert_eq(second.type, TOKEN_STAR);

    lexer_destroy(lex);
}

test(lexer_placeholder) {
    const char* sql = "$1 $2 $123";
    Lexer* lex = lexer_create(sql, strlen(sql));
    assert_non_null(lex);

    Token t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_PLACEHOLDER);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_PLACEHOLDER);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_PLACEHOLDER);

    t = lexer_next_token(lex);
    assert_eq(t.type, TOKEN_EOF);

    lexer_destroy(lex);
}

/* Run all tests */
int main() {
    printf("=== Lexer Unit Tests ===\n");

    run(lexer_create_destroy);
    run(lexer_simple_select);
    run(lexer_integer_tokens);
    run(lexer_real_tokens);
    run(lexer_string_token);
    run(lexer_string_escape);
    run(lexer_operators);
    run(lexer_keywords_case_insensitive);
    run(lexer_punctuation);
    run(lexer_complex_sql);
    run(lexer_peek);
    run(lexer_placeholder);

    printf("\nAll lexer tests passed!\n");
    return 0;
}