#ifndef _LEXER_H_
#define _LEXER_H_

#include <stdint.h>
#include <silk.h>

#define FOR_EACH_TOKEN(_) \
	_(TOKEN_EOF) \
	_(TOKEN_BRACKET_OPEN) \
	_(TOKEN_BRACKET_CLOSE) \
	_(TOKEN_CURLY_OPEN) \
	_(TOKEN_CURLY_CLOSE) \
	_(TOKEN_SEMICOLON) \
	_(TOKEN_COMMA) \
	_(TOKEN_PLUS) \
	_(TOKEN_MINUS) \
	_(TOKEN_ASTERISK) \
	_(TOKEN_SLASH) \
	_(TOKEN_EQ_SIGN) \
	_(TOKEN_INT_LITERAL) \
	_(TOKEN_STR_LITERAL) \
	_(TOKEN_IDENTIFIER) \
	_(TOKEN_FUNCTION) \
	_(TOKEN_RETURN) \
	_(TOKEN_VAR)

typedef enum {
#define ENUMERATOR(tok) tok,
FOR_EACH_TOKEN(ENUMERATOR)
#undef ENUMERATOR
} TokenType;

typedef struct {
	TokenType type;
	int line;
	union {
		char whitespace_char;
		char* data;
		int64_t num;
	};
} Token;

typedef struct {
	Silk_Ctx* ctx;
	const char* data;
	const char* end;
	int line;
} Lexer;

int lexer_init(Lexer* lexer, Silk_Ctx* ctx, const char* data, const char* end);
int lexer_next(Lexer* lexer, Token* tok);
const char* lexer_token_type_to_str(TokenType type);
void lexer_print_token(Token* tok);
void lexer_destroy_token(Token* tok);

#endif
