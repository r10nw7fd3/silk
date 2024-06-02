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

static inline void unexpected(Parser* parser, TokenType expected) {
	if(parser->lexer->ctx->print_errors)
		printf("%s:%d: error: Invalid token %s, expected %s\n",
			parser->lexer->ctx->filename, parser->tok.line,
			lexer_token_type_to_str(parser->tok.type),
			lexer_token_type_to_str(expected));
}

static int expect(Parser* parser, TokenType expected) {
	if(parser->tok.type == expected)
		return lexer_next(parser->lexer, &parser->tok);
	unexpected(parser, expected);
	return 1;
}

static int expect_silent(Parser* parser, TokenType expected) {
	if(parser->tok.type == expected)
		return lexer_next(parser->lexer, &parser->tok);
	return 1;
}

static inline void invalid(Parser* parser) {
	if(parser->lexer->ctx->print_errors) {
		printf("%s:%d: error: Invalid token ", parser->lexer->ctx->filename,
			parser->tok.line);
		lexer_print_token(&parser->tok);
	}
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
		.line = parser->tok.line
	});

	TokenType lhs_type = parser->tok.type;
	int64_t num = parser->tok.num;
	char* data = parser->tok.data;
	if(
		expect_silent(parser, TOKEN_IDENTIFIER) &&
		expect_silent(parser, TOKEN_INT_LITERAL) &&
		expect(parser, TOKEN_STR_LITERAL)
	) {
		free(expr_node);
		return NULL;
	}

	if(lhs_type == TOKEN_IDENTIFIER) {
		if(parser->tok.type == TOKEN_BRACKET_OPEN) {
			if(lexer_next(parser->lexer, &parser->tok)) {
				free(data);
				free(expr_node);
				return NULL;
			}

			vector_ASTNode_ptr_t_ainit(&expr_node->expr.fun_call.args, 64);
			while(parser->tok.type != TOKEN_BRACKET_CLOSE) {
				ASTNode* arg = parse_expr(parser);
				if(!arg) {
					for(size_t i = 0; i < expr_node->expr.fun_call.args.size; ++i)
						ast_destroy(expr_node->expr.fun_call.args.data[i]);
					vector_deinit(&expr_node->expr.fun_call.args);
					free(data);
					free(expr_node);
					return NULL;
				}

				vector_aappend(&expr_node->expr.fun_call.args, arg);

				if(parser->tok.type == TOKEN_COMMA) {
					if(lexer_next(parser->lexer, &parser->tok)) {
						for(size_t i = 0; i < expr_node->expr.fun_call.args.size; ++i)
							ast_destroy(expr_node->expr.fun_call.args.data[i]);
						vector_deinit(&expr_node->expr.fun_call.args);
						free(data);
						free(expr_node);
						return NULL;
					}
				}
			}

			if(expect(parser, TOKEN_BRACKET_CLOSE)) {
				for(size_t i = 0; i < expr_node->expr.fun_call.args.size; ++i)
					ast_destroy(expr_node->expr.fun_call.args.data[i]);
				vector_deinit(&expr_node->expr.fun_call.args);
				free(data);
				free(expr_node);
				return NULL;
			}
			expr_node->expr.type = NODE_EXPR_FUN_CALL;
			expr_node->expr.fun_call.identifier = data;
		}
		else {
			expr_node->expr.type = NODE_EXPR_VAR_LOOKUP;
			expr_node->expr.var_lookup.identifier = data;
		}

	}
	else if(lhs_type == TOKEN_INT_LITERAL) {
		expr_node->expr.type = NODE_EXPR_INT_LIT;
		expr_node->expr.int_lit.num = num;
	}
	else if(lhs_type == TOKEN_STR_LITERAL) {
		expr_node->expr.type = NODE_EXPR_STR_LIT;
		expr_node->expr.str_lit.str = data;
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
				.type = NODE_EXPR_BIN_OP,
				.bin_op = {
					.type = token_to_bin_op(type),
					.lhs = lhs,
					.rhs = rhs
				}
			}
		});
	}

	return expr_node;
}

static ASTNode* parse_return(Parser* parser) {
	int line = parser->tok.line;
	if(lexer_next(parser->lexer, &parser->tok))
		return NULL;

	ASTNode* ret_node = ast_create_node((ASTNode){
		.type = NODE_RET_STATEMENT,
		.line = line,
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
		ast_destroy(ret_node);
		return NULL;
	}

	return ret_node;
}

static ASTNode* parse_var(Parser* parser) {
	int line = parser->tok.line;
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
		.line = line,
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
		.line = parser->tok.line,
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
				unexpected(parser, TOKEN_CURLY_CLOSE);
				ast_destroy(scope_node);
				return NULL;
			default:
				invalid(parser);
				ast_destroy(scope_node);
				return NULL;
		}
	}
}

static ASTNode* parse_function(Parser* parser) {
	int line = parser->tok.line;
	if(lexer_next(parser->lexer, &parser->tok))
		return NULL;

	char* identifier = parser->tok.data;
	if(expect(parser, TOKEN_IDENTIFIER))
		return NULL;

	if(expect(parser, TOKEN_BRACKET_OPEN))
		return NULL;

	Vector_str_t arguments;
	vector_str_t_ainit(&arguments, 64);

	while(parser->tok.type != TOKEN_BRACKET_CLOSE) {
		char* arg = parser->tok.data;
		if(expect(parser, TOKEN_IDENTIFIER)) {
			for(size_t i = 0; i < arguments.size; ++i)
				free(arguments.data[i]);
			vector_deinit(&arguments);
			free(identifier);
			return NULL;
		}

		vector_aappend(&arguments, arg);

		if(parser->tok.type == TOKEN_COMMA)
			expect(parser, TOKEN_COMMA);
	}

	if(expect(parser, TOKEN_BRACKET_CLOSE)) {
		for(size_t i = 0; i < arguments.size; ++i)
			free(arguments.data[i]);
		vector_deinit(&arguments);
		free(identifier);
		return NULL;
	}

	ASTNode* body = parse_scope(parser);
	if(!body) {
		for(size_t i = 0; i < arguments.size; ++i)
			free(arguments.data[i]);
		vector_deinit(&arguments);
		free(identifier);
		return NULL;
	}

	ASTNode* fun_node = ast_create_node((ASTNode){
		.type = NODE_FUN_STATEMENT,
		.line = line,
		.fun = {
			.identifier = identifier,
			.arguments = arguments,
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

	if(lexer_next(parser->lexer, &parser->tok)) {
		ast_destroy(root);
		return NULL;
	}

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
					root = NULL;
					goto out;
				}
				scope_append(root, expr_node);
				break;
			}
			case TOKEN_SEMICOLON:
				if(lexer_next(parser->lexer, &parser->tok)) {
					ast_destroy(root);
					root = NULL;
					goto out;
				}
				break;
			case TOKEN_FUNCTION: {
				ASTNode* fun_node = parse_function(parser);
				if(!fun_node) {
					ast_destroy(root);
					root = NULL;
					goto out;
				}
				scope_append(root, fun_node);
				break;
			}
			case TOKEN_VAR: {
				ASTNode* var_node = parse_var(parser);
				if(!var_node) {
					ast_destroy(root);
					root = NULL;
					goto out;
				}
				scope_append(root, var_node);
				break;
			}
			default:
				invalid(parser);
				ast_destroy(root);
				root = NULL;
				goto out;
		}
	}

out:
	if(!root) {
		lexer_destroy_token(&parser->tok);
		return NULL;
	}

	if(parser->lexer->ctx->print_ast)
		ast_print_node(root, 1);

	return root;
}
