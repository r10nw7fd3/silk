#include "parser.h"
#include "ast.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

int parser_init(Parser* parser, Lexer* lexer) {
	parser->lexer = lexer;
	return 0;
}

static inline void unexpected(Token* tok, TokenType expected) {
	printf("Error on line %d: Invalid token %s, expected %s\n", tok->line,
		lexer_token_type_to_str(tok->type),
		lexer_token_type_to_str(expected));
}

static int expect(Parser* parser, TokenType expected) {
	if(parser->tok.type == expected)
		return lexer_next(parser->lexer, &parser->tok);
	unexpected(&parser->tok, expected);
	return 1;
}

static int expect_silent(Parser* parser, TokenType expected) {
	if(parser->tok.type == expected)
		return lexer_next(parser->lexer, &parser->tok);
	return 1;
}

static void scope_append(ASTNode* scope, ASTNode* appendant) {
	size_t new_size = (scope->scope.n_nodes + 1) * sizeof(ASTNode*);
	ASTNode** new = realloc(scope->scope.nodes, new_size);
	assert(new);
	scope->scope.nodes = new;
	scope->scope.nodes[scope->scope.n_nodes] = appendant;
	++scope->scope.n_nodes;
}

static inline int is_bin_op(TokenType type) {
	return
		type == TOKEN_PLUS ||
		type == TOKEN_MINUS ||
		type == TOKEN_ASTERISK ||
		type == TOKEN_SLASH;
}

static int token_to_bin_op(TokenType type) {
	switch(type) {
		case TOKEN_PLUS: return NODE_EXPR_SUM;
		case TOKEN_MINUS: return NODE_EXPR_SUB;
		case TOKEN_ASTERISK: return NODE_EXPR_MUL;
		case TOKEN_SLASH: return NODE_EXPR_DIV;
		default: assert(0);
	}
}

static ASTNode* parse_expr(Parser* parser) {
	ASTNode* expr_node = ast_create_node((ASTNode){
		.type = NODE_EXPR,
	});

	TokenType lhs_type = parser->tok.type;
	int64_t num = parser->tok.num;
	char* data = parser->tok.data;
	if(
		expect_silent(parser, TOKEN_IDENTIFIER) &&
		expect_silent(parser, TOKEN_INT_LITERAL) &&
		expect(parser, TOKEN_STR_LITERAL)
	) {
		ast_destroy(expr_node);
		return NULL;
	}

	if(lhs_type == TOKEN_IDENTIFIER) {
		expr_node->expr.data = data;
		if(parser->tok.type == TOKEN_BRACKET_OPEN) {
			if(lexer_next(parser->lexer, &parser->tok)) {
				ast_destroy(expr_node);
				return NULL;
			}
			if(expect(parser, TOKEN_BRACKET_CLOSE)) {
				ast_destroy(expr_node);
				return NULL;
			}
			expr_node->expr.kind = NODE_EXPR_FUN_CALL;
		}
		else
			expr_node->expr.kind = NODE_EXPR_VAR_LOOKUP;

	}
	else if(lhs_type == TOKEN_INT_LITERAL) {
		expr_node->expr.kind = NODE_EXPR_INT_LIT;
		expr_node->expr.num = num;
	}
	else if(lhs_type == TOKEN_STR_LITERAL) {
		expr_node->expr.kind = NODE_EXPR_STR_LIT;
		expr_node->expr.data = data;
	}
	else
		assert(0);

	if(is_bin_op(parser->tok.type)) {
		ASTNode* lhs = expr_node;
		TokenType type = parser->tok.type;
		if(lexer_next(parser->lexer, &parser->tok)) {
			ast_destroy(lhs);
			return NULL;
		}
		ASTNode* rhs = parse_expr(parser);
		if(!rhs) {
			ast_destroy(lhs);
			return NULL;
		}
		expr_node = ast_create_node((ASTNode){
			.type = NODE_EXPR,
			.expr = {
				.kind = NODE_EXPR_BIN_OP,
				.op = token_to_bin_op(type),
				.lhs = lhs,
				.rhs = rhs
			}
		});
	}

	return expr_node;
}

static ASTNode* parse_return(Parser* parser) {
	if(lexer_next(parser->lexer, &parser->tok))
		return NULL;

	ASTNode* ret_node = ast_create_node((ASTNode){
		.type = NODE_RET_STATEMENT,
		.ret = {
			NULL
		}
	});

	if(!expect_silent(parser, TOKEN_SEMICOLON))
		return ret_node;

	ASTNode* expr_node = parse_expr(parser);
	if(!expr_node) {
		ast_destroy(ret_node);
		return NULL;
	}

	ret_node->ret.expr = expr_node;

	if(expect(parser, TOKEN_SEMICOLON)) {
		printf("here\n");
		ast_destroy(ret_node);
		return NULL;
	}

	return ret_node;
}

static ASTNode* parse_var(Parser* parser) {
	if(lexer_next(parser->lexer, &parser->tok))
		return NULL;

	char* identifier = parser->tok.data;
	if(expect(parser, TOKEN_IDENTIFIER))
		return NULL;

	if(expect(parser, TOKEN_EQ_SIGN)) {
		free(identifier);
		return NULL;
	}

	ASTNode* expr_node = parse_expr(parser);
	if(!expr_node) {
		free(identifier);
		return NULL;
	}

	ASTNode* var_node = ast_create_node((ASTNode){
		.type = NODE_VAR_STATEMENT,
		.var = {
			.identifier = identifier,
			.expr = expr_node,
		}
	});
	return var_node;
}

static ASTNode* parse_scope(Parser* parser) {
	ASTNode* scope_node = ast_create_node((ASTNode){
		.type = NODE_SCOPE,
		.scope = {
			0,
			NULL
		}
	});
	if(expect(parser, TOKEN_CURLY_OPEN)) {
		ast_destroy(scope_node);
		return NULL;
	}

	for(;;) {
		switch(parser->tok.type) {
			case TOKEN_RETURN: {
				ASTNode* ret_node = parse_return(parser);
				if(!ret_node) {
					ast_destroy(scope_node);
					return NULL;
				}
				scope_append(scope_node, ret_node);
				break;
			}
			case TOKEN_CURLY_CLOSE:
				if(lexer_next(parser->lexer, &parser->tok)) {
					ast_destroy(scope_node);
					return NULL;
				}
				return scope_node;
			case TOKEN_IDENTIFIER:
			case TOKEN_INT_LITERAL:
			case TOKEN_STR_LITERAL: {
				ASTNode* expr_node = parse_expr(parser);
				if(!expr_node) {
					ast_destroy(scope_node);
					return NULL;
				}
				scope_append(scope_node, expr_node);
				break;
			}
			case TOKEN_VAR: {
				ASTNode* var_node = parse_var(parser);
				if(!var_node) {
					ast_destroy(scope_node);
					return NULL;
				}
				scope_append(scope_node, var_node);
				break;
			}
			case TOKEN_SEMICOLON:
				if(lexer_next(parser->lexer, &parser->tok)) {
					ast_destroy(scope_node);
					return NULL;
				}
				break;
			case TOKEN_EOF:
				unexpected(&parser->tok, TOKEN_CURLY_CLOSE);
				ast_destroy(scope_node);
				return NULL;
			default:
				printf("Error on line %d: Invalid token ", parser->tok.line);
				lexer_print_token(&parser->tok);
				ast_destroy(scope_node);
				return NULL;
		}
	}
}

static ASTNode* parse_function(Parser* parser) {
	if(lexer_next(parser->lexer, &parser->tok))
		return NULL;

	char* identifier = parser->tok.data;
	if(expect(parser, TOKEN_IDENTIFIER))
		return NULL;

	if(
		expect(parser, TOKEN_BRACKET_OPEN) ||
		expect(parser, TOKEN_BRACKET_CLOSE)
	) {
		free(identifier);
		return NULL;
	}

	ASTNode* body = parse_scope(parser);
	if(!body) {
		free(identifier);
		return NULL;
	}

	ASTNode* fun_node = ast_create_node((ASTNode){
		.type = NODE_FUN_STATEMENT,
		.fun = {
			.identifier = identifier,
			.body = body
		}
	});
	return fun_node;
}

ASTNode* parser_parse(Parser* parser) {
	ASTNode* root = ast_create_node((ASTNode){
		.type = NODE_SCOPE,
		.scope = {
			0,
			NULL
		}
	});

	if(lexer_next(parser->lexer, &parser->tok))
		return NULL;

	for(;;) {
		switch(parser->tok.type) {
			case TOKEN_EOF:
				goto out;
			case TOKEN_IDENTIFIER:
			case TOKEN_INT_LITERAL:
			case TOKEN_STR_LITERAL: {
				ASTNode* expr_node = parse_expr(parser);
				if(!expr_node) {
					ast_destroy(root);
					return NULL;
				}
				scope_append(root, expr_node);
				break;
			}
			case TOKEN_SEMICOLON:
				if(lexer_next(parser->lexer, &parser->tok)) {
					ast_destroy(root);
					return NULL;
				}
				break;
			case TOKEN_FUNCTION: {
				ASTNode* fun_node = parse_function(parser);
				if(!fun_node) {
					ast_destroy(root);
					return NULL;
				}
				scope_append(root, fun_node);
				break;
			}
			case TOKEN_VAR: {
				ASTNode* var_node = parse_var(parser);
				if(!var_node) {
					ast_destroy(root);
					return NULL;
				}
				scope_append(root, var_node);
				break;
			}
			default:
				printf("Error on line %d: Invalid token ", parser->tok.line);
				lexer_print_token(&parser->tok);
				ast_destroy(root);
				return NULL;
		}
	}

out:
	if(!root)
		return NULL;

	ast_print_node(root, 1);

	return root;
}
