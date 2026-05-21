#ifndef TINYDB_PARSER_H
#define TINYDB_PARSER_H

#include "lexer.h"
#include "ast.h"
#include <stdbool.h>

/*============================================================================
 * SQL Parser
 *============================================================================*/
typedef struct Parser Parser;

/* Parser lifecycle */
Parser* parser_create(const char* sql, size_t len);
void parser_destroy(Parser* parser);

/* Parse SQL string into AST */
AstNode* parser_parse(Parser* parser);
char* parser_error(Parser* parser);
int parser_error_line(Parser* parser);
int parser_error_column(Parser* parser);

/* Free AST allocated by parser */
void parser_free_ast(Parser* parser, AstNode* node);

#endif /* TINYDB_PARSER_H */