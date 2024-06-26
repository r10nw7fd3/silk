#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int lexer_init(Lexer* lexer, Silk_Ctx* ctx, const char* data, const char* end) {
	lexer->ctx = ctx;
	lexer->data = data;
	lexer->end = end;
	lexer->line = 1;
	return 0;
}

static inline int is_valid_identifier(char c) {
	return
		c != '(' && c != ')' &&
		c != '{' && c != '}' &&
		c != ';' &&
		c != '+' &&
		c != '*' && c != '/' &&
		c != '.' && c != ',';
}

static inline char* copy_str_to_heap(const char* str) {
	size_t len = strlen(str);
	char* new = malloc(len + 1);
	assert(new);
	return memcpy(new, str, len + 1);
}

int lexer_next(Lexer* lexer, Token* tok) {
again:
	tok->line = lexer->line;
#define DATA_SIZE 128
	char data[DATA_SIZE];
	if(lexer->data >= lexer->end) {
		tok->type = TOKEN_EOF;
		return 0;
	}
	if(isspace(*lexer->data)) {
		if(*lexer->data == '\n')
			++lexer->line;
		++lexer->data;
		goto again;
	}

#define SINGLE_CHAR_TOKEN(c, toktype) \
	do { \
		if(*lexer->data == c) { \
			tok->type = toktype; \
			goto inc_ret; \
		} \
	} \
	while(0)

	SINGLE_CHAR_TOKEN('(', TOKEN_BRACKET_OPEN);
	SINGLE_CHAR_TOKEN(')', TOKEN_BRACKET_CLOSE);
	SINGLE_CHAR_TOKEN('{', TOKEN_CURLY_OPEN);
	SINGLE_CHAR_TOKEN('}', TOKEN_CURLY_CLOSE);
	SINGLE_CHAR_TOKEN(';', TOKEN_SEMICOLON);
	SINGLE_CHAR_TOKEN(',', TOKEN_COMMA);
	SINGLE_CHAR_TOKEN('+', TOKEN_PLUS);
	SINGLE_CHAR_TOKEN('-', TOKEN_MINUS);
	SINGLE_CHAR_TOKEN('*', TOKEN_ASTERISK);
	SINGLE_CHAR_TOKEN('/', TOKEN_SLASH);
	SINGLE_CHAR_TOKEN('=', TOKEN_EQ_SIGN);
	if(isdigit(*lexer->data)) {
		int64_t num;
		for(num = 0; isdigit(*lexer->data) && lexer->data < lexer->end;
			++lexer->data) {
			num *= 10;
			num += *lexer->data - '0';
		}
		tok->type = TOKEN_INT_LITERAL;
		tok->num = num;
		goto ret;
	}
	if(*lexer->data == '"') {
		++lexer->data;
		size_t i = 0;
		for(i = 0; i < DATA_SIZE && lexer->data < lexer->end &&
			*lexer->data != '"'; ++i, ++lexer->data)
			data[i] = *lexer->data;

		data[ i < DATA_SIZE ? i : DATA_SIZE - 1 ] = 0;
		if(lexer->data < lexer->end)
			++lexer->data;

		tok->type = TOKEN_STR_LITERAL;
		tok->data = copy_str_to_heap(data);
		goto ret;
	}
	else {
		size_t i;
		for(i = 0; !isspace(*lexer->data) && is_valid_identifier(*lexer->data)
			&& lexer->data < lexer->end && i < DATA_SIZE; ++i, ++lexer->data)
			data[i] = *lexer->data;

		data[ i < DATA_SIZE ? i : DATA_SIZE - 1 ] = 0;
		if(!strcmp(data, "function")) {
			tok->type = TOKEN_FUNCTION;
			goto ret;
		}
		if(!strcmp(data, "return")) {
			tok->type = TOKEN_RETURN;
			goto ret;
		}
		if(!strcmp(data, "var")) {
			tok->type = TOKEN_VAR;
			goto ret;
		}
		else {
			tok->type = TOKEN_IDENTIFIER;
			tok->data = copy_str_to_heap(data);
			goto ret;
		}
	}

inc_ret:
	++lexer->data;
ret:
	if(lexer->ctx->print_tokens)
		lexer_print_token(tok);
	return 0;
}

const char* lexer_token_type_to_str(TokenType type) {
	switch(type) {
#define ENUMERATOR(tok) case tok: return &#tok[6];
		FOR_EACH_TOKEN(ENUMERATOR)
		default:
			return "(invalid token)";
	}
}

void lexer_print_token(Token* tok) {
	printf("%s ", lexer_token_type_to_str(tok->type));
	if(tok->type == TOKEN_IDENTIFIER)
		printf("%s", tok->data);
	else if(tok->type == TOKEN_INT_LITERAL)
		printf("%ld", tok->num);
	else if(tok->type == TOKEN_STR_LITERAL)
		printf("%s", tok->data);
	putchar('\n');
}

void lexer_destroy_token(Token* tok) {
	switch(tok->type) {
		case TOKEN_STR_LITERAL:
		case TOKEN_IDENTIFIER:
			free(tok->data);
			tok->data = NULL;
		default:
			break;
	}
}
