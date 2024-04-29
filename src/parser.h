#ifndef _PARSER_H_
#define _PARSER_H_

#include "lexer.h"

typedef struct {
	Lexer* lexer;
	Token tok;
} Parser;

int parser_init(Parser* parser, Lexer* lexer);
int parser_parse(Parser* parser);

#endif
