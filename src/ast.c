#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

ASTNode* ast_create_node(ASTNode node) {
	ASTNode* new = malloc(sizeof(ASTNode));
	assert(new);
	*new = node;
	return new;
}

void ast_destroy(ASTNode* node) {
	if(!node)
		return;

	switch(node->type) {
		case NODE_SCOPE:
			for(size_t i = 0; i < node->scope.n_nodes; ++i)
				ast_destroy(node->scope.nodes[i]);
			free(node->scope.nodes);
			break;
		case NODE_FUN_STATEMENT:
			free(node->fun.identifier);
			ast_destroy(node->fun.body);
			break;
		case NODE_RET_STATEMENT:
			ast_destroy(node->ret.expr);
			break;
		case NODE_EXPR:
			if(node->expr.kind == NODE_EXPR_INT_LIT) {
			}
			else if(node->expr.kind == NODE_EXPR_BIN_OP) {
				ast_destroy(node->expr.lhs);
				ast_destroy(node->expr.rhs);
			}
			else if(
				node->expr.kind == NODE_EXPR_VAR_LOOKUP ||
				node->expr.kind == NODE_EXPR_FUN_CALL ||
				node->expr.kind == NODE_EXPR_STR_LIT
			)
				free(node->expr.data);
			else
				assert(0);
			break;
		case NODE_VAR_STATEMENT:
			free(node->var.identifier);
			ast_destroy(node->var.expr);
			break;
		default:
			assert(0);
	}
	free(node);
}

const char* ast_node_type_to_str(ASTNodeType type) {
	switch(type) {
#define ENUMERATOR(typ3) case typ3: return #typ3 + 5;
		FOR_EACH_NODE(ENUMERATOR)
		default:
			return "(invalid node)";
	}
}

static void print_indent(int indent) {
	if(!indent)
		return;
	while(--indent) {
		putchar(' ');
		putchar(' ');
	}
}

void ast_print_node(ASTNode* node, int indent) {
	if(!node)
		return;

	print_indent(indent);
	printf("%s ", ast_node_type_to_str(node->type));
	switch(node->type) {
		case NODE_SCOPE:
			putchar('\n');
			for(size_t i = 0; i < node->scope.n_nodes; ++i)
				ast_print_node(node->scope.nodes[i], indent + 1);
			break;
		case NODE_FUN_STATEMENT:
			puts(node->fun.identifier);
			ast_print_node(node->fun.body, indent + 1);
			break;
		case NODE_RET_STATEMENT:
			putchar('\n');
			ast_print_node(node->ret.expr, indent + 1);
			break;
		case NODE_EXPR:
			if(node->expr.kind == NODE_EXPR_INT_LIT)
				printf("%ld\n", node->expr.num);
			else if(node->expr.kind == NODE_EXPR_STR_LIT)
				printf("\"%s\"\n", node->expr.data);
			else if(node->expr.kind == NODE_EXPR_BIN_OP) {
				printf("%c\n", node->expr.op);
				ast_print_node(node->expr.lhs, indent + 1);
				ast_print_node(node->expr.rhs, indent + 1);
			}
			else if(node->expr.kind == NODE_EXPR_VAR_LOOKUP ||
				node->expr.kind == NODE_EXPR_FUN_CALL)
				printf("%s%s\n", node->expr.data,
					node->expr.kind == NODE_EXPR_VAR_LOOKUP ? "" : "()");
			else
				assert(0);
			break;
		case NODE_VAR_STATEMENT:
			puts(node->var.identifier);
			ast_print_node(node->var.expr, indent + 1);
			break;
		default:
			assert(0);
	}
}
