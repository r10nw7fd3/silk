#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "instruction.h"

typedef struct {
	ASTNode* node;
	size_t start_addr;
	int64_t ra_index;
} FunctionCtx;
VECTOR_DEFINE(FunctionCtx);

typedef struct {
	size_t code_pos;
	const char* identifier;
} BackPatch;
VECTOR_DEFINE(BackPatch);

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

static inline FunctionCtx* lookup_fun_ctx(Vector_FunctionCtx* funcs, ASTNode* node) {
	for(size_t i = 0; i < funcs->size; ++i)
		if(funcs->data[i].node == node)
			return &funcs->data[i];
	assert(0);
}

static inline FunctionCtx* lookup_fun_ctx_by_name(Vector_FunctionCtx* funcs, const char* name) {
	for(size_t i = 0; i < funcs->size; ++i)
		if(!strcmp(funcs->data[i].node->fun.identifier, name))
			return &funcs->data[i];
	assert(0);
}

static int compile_recur(Vector_Instruction* instructions, ASTNode* node,
	Vector_FunctionCtx* functions, Vector_BackPatch* bpatches) {
	switch(node->type) {
		case NODE_SCOPE:
			for(size_t i = 0; i < node->scope.n_nodes; ++i)
				if(compile_recur(instructions, node->scope.nodes[i], functions, bpatches))
					return 1;
			break;
		case NODE_EXPR:
			if(node->expr.kind == NODE_EXPR_INT_LIT)
				vector_aappend(*instructions, ((Instruction){ INST_PUSH, node->expr.num }));
			else if(node->expr.kind == NODE_EXPR_BIN_OP) {
				if(compile_recur(instructions, node->expr.lhs, functions, bpatches))
					return 1;
				if(compile_recur(instructions, node->expr.rhs, functions, bpatches))
					return 1;
				switch(node->expr.op) {
					case NODE_EXPR_SUM:
						vector_aappend(*instructions, ((Instruction){ INST_SUM, 0 }));
						break;
					case NODE_EXPR_SUB:
						vector_aappend(*instructions, ((Instruction){ INST_SUB, 0 }));
						break;
					case NODE_EXPR_MUL:
						vector_aappend(*instructions, ((Instruction){ INST_MUL, 0 }));
						break;
					case NODE_EXPR_DIV:
						vector_aappend(*instructions, ((Instruction){ INST_DIV, 0 }));
						break;
					default:
						assert(0);
				}
			}
			else if(node->expr.kind == NODE_EXPR_FUN_CALL) {
				vector_aappend(*bpatches, ((BackPatch){ instructions->size, node->expr.data }));
				vector_aappend(*instructions, ((Instruction){ INST_CALL, 0 }));
			}
			else
				assert(0);
			break;
		case NODE_RET_STATEMENT:
			compile_recur(instructions, node->ret.expr, functions, bpatches);
			vector_aappend(*instructions, ((Instruction){ INST_RET, 0 }));
			break;
		case NODE_FUN_STATEMENT:
			FunctionCtx* fun = lookup_fun_ctx(functions, node);
			fun->start_addr = instructions->size;
			compile_recur(instructions, node->fun.body, functions, bpatches);
			break;
		default:
			assert(0);
	}
	return 0;
}

int ast_compile(Vector_Instruction* instructions, ASTNode* node) {
	Vector_FunctionCtx functions;
	vector_ainit(functions, 64);

	Vector_BackPatch bpatches;
	vector_ainit(bpatches, 64);

	assert(node->type == NODE_SCOPE);
	for(size_t i = 0; i < node->scope.n_nodes; ++i) {
		if(node->scope.nodes[i]->type == NODE_FUN_STATEMENT) {
			vector_aappend(functions, ((FunctionCtx){ node->scope.nodes[i], 0, 0 }));
			continue;
		}
		compile_recur(instructions, node->scope.nodes[i], &functions, &bpatches);
	}

	vector_aappend(*instructions, ((Instruction){ INST_EXIT, 0 }));

	for(size_t i = 0; i < functions.size; ++i)
		compile_recur(instructions, functions.data[i].node, &functions, &bpatches);

	for(size_t i = 0; i < bpatches.size; ++i) {
		FunctionCtx* ctx = lookup_fun_ctx_by_name(&functions, bpatches.data[i].identifier);
		instructions->data[bpatches.data[i].code_pos].val = ctx->start_addr;
	}

	vector_deinit(functions);
	vector_deinit(bpatches);

	return 0;
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
