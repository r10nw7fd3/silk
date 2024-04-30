#ifndef _PARSER_H_
#define _PARSER_H_

#include "lexer.h"
#include "ast.h"

typedef struct {
	Lexer* lexer;
	Token tok;
} Parser;

int parser_init(Parser* parser, Lexer* lexer);
ASTNode* parser_parse(Parser* parser);

#endif
