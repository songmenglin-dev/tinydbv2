#include "../../tests/unit/mini_test.h"
#include "sql/parser.h"
#include "sql/ast.h"
#include "sql/lexer.h"
#include "sql/token.h"
#include <string.h>

/* Test simple SELECT */
test(parser_simple_select) {
    const char* sql = "SELECT * FROM users;";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_SELECT);

    /* Check table name */
    AstSelect* sel = (AstSelect*)((char*)node - offsetof(AstSelect, base));
    assert_str_eq(sel->table_name, "users");

    /* Check columns (should be * ) */
    assert_non_null(sel->columns);
    assert_str_eq(sel->columns->name, "*");

    parser_free_ast(p, node);
    parser_destroy(p);
}

test(parser_select_with_where) {
    const char* sql = "SELECT id, name FROM users WHERE age > 18;";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_SELECT);

    AstSelect* sel = (AstSelect*)((char*)node - offsetof(AstSelect, base));
    assert_str_eq(sel->table_name, "users");

    /* Check columns */
    assert_non_null(sel->columns);
    assert_str_eq(sel->columns->name, "id");
    assert_non_null(sel->columns->next);
    assert_str_eq(sel->columns->next->name, "name");

    parser_free_ast(p, node);
    parser_destroy(p);
}

test(parser_create_table) {
    const char* sql = "CREATE TABLE users (id INTEGER, name TEXT);";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_CREATE_TABLE);

    AstCreateTable* ct = (AstCreateTable*)((char*)node - offsetof(AstCreateTable, base));
    assert_str_eq(ct->table_name, "users");
    assert_eq(ct->if_not_exists, 0);

    parser_free_ast(p, node);
    parser_destroy(p);
}

test(parser_create_table_if_not_exists) {
    const char* sql = "CREATE TABLE IF NOT EXISTS users (id INTEGER, name TEXT);";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_CREATE_TABLE);

    AstCreateTable* ct = (AstCreateTable*)((char*)node - offsetof(AstCreateTable, base));
    assert_str_eq(ct->table_name, "users");
    assert_eq(ct->if_not_exists, 1);

    parser_free_ast(p, node);
    parser_destroy(p);
}

test(parser_insert) {
    const char* sql = "INSERT INTO users VALUES (1, 'Alice');";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_INSERT);

    AstInsert* ins = (AstInsert*)((char*)node - offsetof(AstInsert, base));
    assert_str_eq(ins->table_name, "users");

    /* Verify it has one ValueList (one row) with two expressions */
    assert_non_null(ins->values);
    assert_eq(ins->values->count, 2);
    assert_non_null(ins->values->values);
    assert_non_null(ins->values->values[0]);
    assert_eq(ins->values->values[0]->type, EXPR_LITERAL_INT);
    assert_eq(ins->values->values[0]->as_int, 1);
    assert_non_null(ins->values->values[1]);
    assert_eq(ins->values->values[1]->type, EXPR_LITERAL_STRING);
    assert_str_eq(ins->values->values[1]->as_string.str, "Alice");
    /* Should have only one row (no next) */
    assert_null(ins->values->next);

    parser_free_ast(p, node);
    parser_destroy(p);
}

test(parser_insert_multi_row) {
    /* Test INSERT with multiple value tuples: INSERT INTO t VALUES (1),(2) */
    const char* sql = "INSERT INTO t VALUES (1),(2);";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_INSERT);

    AstInsert* ins = (AstInsert*)((char*)node - offsetof(AstInsert, base));
    assert_str_eq(ins->table_name, "t");

    /* Should have TWO ValueList nodes (two rows) */
    assert_non_null(ins->values);
    assert_eq(ins->values->count, 1);
    assert_non_null(ins->values->values[0]);
    assert_eq(ins->values->values[0]->type, EXPR_LITERAL_INT);
    assert_eq(ins->values->values[0]->as_int, 1);

    /* Second row */
    assert_non_null(ins->values->next);
    assert_eq(ins->values->next->count, 1);
    assert_non_null(ins->values->next->values[0]);
    assert_eq(ins->values->next->values[0]->type, EXPR_LITERAL_INT);
    assert_eq(ins->values->next->values[0]->as_int, 2);
    assert_null(ins->values->next->next);

    parser_free_ast(p, node);
    parser_destroy(p);
}

test(parser_update) {
    const char* sql = "UPDATE users SET name = 'Bob' WHERE id = 1;";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_UPDATE);

    AstUpdate* upd = (AstUpdate*)((char*)node - offsetof(AstUpdate, base));
    assert_str_eq(upd->table_name, "users");
    assert_non_null(upd->where);

    parser_free_ast(p, node);
    parser_destroy(p);
}

test(parser_delete) {
    const char* sql = "DELETE FROM users WHERE id = 1;";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_DELETE);

    AstDelete* del = (AstDelete*)((char*)node - offsetof(AstDelete, base));
    assert_str_eq(del->table_name, "users");
    assert_non_null(del->where);

    parser_free_ast(p, node);
    parser_destroy(p);
}

test(parser_drop_table) {
    const char* sql = "DROP TABLE users;";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_DROP_TABLE);

    AstDropTable* dt = (AstDropTable*)((char*)node - offsetof(AstDropTable, base));
    assert_str_eq(dt->table_name, "users");

    parser_free_ast(p, node);
    parser_destroy(p);
}

test(parser_begin) {
    const char* sql = "BEGIN;";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_TRANSACTION);

    AstTransaction* tx = (AstTransaction*)((char*)node - offsetof(AstTransaction, base));
    assert_eq(tx->transaction_type, TX_BEGIN);

    parser_free_ast(p, node);
    parser_destroy(p);
}

test(parser_commit) {
    const char* sql = "COMMIT;";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_TRANSACTION);

    AstTransaction* tx = (AstTransaction*)((char*)node - offsetof(AstTransaction, base));
    assert_eq(tx->transaction_type, TX_COMMIT);

    parser_free_ast(p, node);
    parser_destroy(p);
}

test(parser_rollback) {
    const char* sql = "ROLLBACK;";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_TRANSACTION);

    AstTransaction* tx = (AstTransaction*)((char*)node - offsetof(AstTransaction, base));
    assert_eq(tx->transaction_type, TX_ROLLBACK);

    parser_free_ast(p, node);
    parser_destroy(p);
}

test(parser_create_index) {
    const char* sql = "CREATE INDEX idx ON users (name);";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_CREATE_INDEX);

    AstCreateIndex* ci = (AstCreateIndex*)((char*)node - offsetof(AstCreateIndex, base));
    assert_str_eq(ci->index_name, "idx");
    assert_str_eq(ci->table_name, "users");
    assert_str_eq(ci->column_name, "name");

    parser_free_ast(p, node);
    parser_destroy(p);
}

test(parser_drop_index) {
    const char* sql = "DROP INDEX idx;";
    Parser* p = parser_create(sql, strlen(sql));
    assert_non_null(p);

    AstNode* node = parser_parse(p);
    assert_non_null(node);
    assert_eq(node->type, AST_DROP_INDEX);

    AstDropIndex* di = (AstDropIndex*)((char*)node - offsetof(AstDropIndex, base));
    assert_str_eq(di->index_name, "idx");

    parser_free_ast(p, node);
    parser_destroy(p);
}

/* Run all tests */
int main() {
    printf("=== Parser Unit Tests ===\n");

    run(parser_simple_select);
    run(parser_select_with_where);
    run(parser_create_table);
    run(parser_create_table_if_not_exists);
    run(parser_insert);
    run(parser_update);
    run(parser_delete);
    run(parser_drop_table);
    run(parser_begin);
    run(parser_commit);
    run(parser_rollback);
    run(parser_create_index);
    run(parser_drop_index);

    printf("\nAll parser tests passed!\n");
    return 0;
}