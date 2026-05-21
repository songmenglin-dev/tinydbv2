#ifndef TINYDB_LEXER_H
#define TINYDB_LEXER_H

#include "token.h"

/*============================================================================
 * Lexer (Tokenizer) for SQL
 *============================================================================*/
typedef struct Lexer Lexer;

/* Lexer lifecycle */
Lexer* lexer_create(const char* sql, size_t len);
void lexer_destroy(Lexer* lexer);

/* Token iteration */
Token lexer_next_token(Lexer* lexer);
Token lexer_peek_token(Lexer* lexer);
void lexer_advance(Lexer* lexer);

/* Error reporting */
const char* lexer_error(Lexer* lexer);
int lexer_error_line(Lexer* lexer);
int lexer_error_column(Lexer* lexer);

#endif /* TINYDB_LEXER_H */