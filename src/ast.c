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
VECTOR_DEFINE(FunctionCtx)

typedef struct {
	const char* identifier;
	size_t code_pos;
	int line;
} BackPatch;
VECTOR_DEFINE(BackPatch)

typedef struct {
	char* identifier;
	int64_t index;
} Variable;
VECTOR_DEFINE(Variable)

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
			for(size_t i = 0; i < node->fun.arguments.size; ++i)
				free(node->fun.arguments.data[i]);
			vector_deinit(&node->fun.arguments);
			ast_destroy(node->fun.body);
			break;
		case NODE_RET_STATEMENT:
			ast_destroy(node->ret.expr);
			break;
		case NODE_EXPR:
			switch(node->expr.type) {
				case NODE_EXPR_INT_LIT:
					break;
				case NODE_EXPR_STR_LIT:
					free(node->expr.str_lit.str);
					break;
				case NODE_EXPR_BIN_OP:
					ast_destroy(node->expr.bin_op.lhs);
					ast_destroy(node->expr.bin_op.rhs);
					break;
				case NODE_EXPR_VAR_LOOKUP:
					free(node->expr.var_lookup.identifier);
					break;
				case NODE_EXPR_VAR_REASSIGNMENT:
					free(node->expr.var_assignment.identifier);
					ast_destroy(node->expr.var_assignment.expr);
					break;
				case NODE_EXPR_FUN_CALL:
					for(size_t i = 0; i < node->expr.fun_call.args.size; ++i)
						ast_destroy(node->expr.fun_call.args.data[i]);
					vector_deinit(&node->expr.fun_call.args);
					free(node->expr.fun_call.identifier);
					break;
				default:
					assert(0);
			}
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
	return NULL;
}

static int is_a_redecl(const char* iden, Vector_Variable* global_vars, Vector_Variable* vars) {
	if(!vars) {
		for(size_t i = 0; i < global_vars->size; ++i)
			if(!strcmp(global_vars->data[i].identifier, iden))
				return 1;
		return 0;
	}
	for(size_t i = 0; i < vars->size; ++i)
		if(!strcmp(vars->data[i].identifier, iden))
			return 1;
	return 0;
}

static int compile_recur(Silk_Ctx* ctx, Vector_Instruction* instructions, ASTNode* node,
	Vector_FunctionCtx* functions, Vector_BackPatch* bpatches, Vector_Variable* global_vars,
	Vector_Variable* vars, Vector_Variable* scope_merge_vars) {
	int is_global = vars == NULL;
	switch(node->type) {
		case NODE_SCOPE: { // TODO: Rethink how scopes should be implemented
			Vector_Variable scope_vars;
			vector_Variable_ainit(&scope_vars, 64);
			if(scope_merge_vars) {
				memcpy(scope_vars.data, scope_merge_vars->data, sizeof(Variable) * scope_merge_vars->size);
				scope_vars.size = scope_merge_vars->size;
			}
			for(size_t i = 0; i < node->scope.n_nodes; ++i)
				if(compile_recur(ctx, instructions, node->scope.nodes[i], functions, bpatches, global_vars, &scope_vars, NULL)) {
					vector_deinit(&scope_vars);
					return 1;
				}
			vector_deinit(&scope_vars);
		}
			break;
		case NODE_EXPR:
			switch(node->expr.type) {
				case NODE_EXPR_INT_LIT:
					vector_aappend(instructions, ((Instruction){ INST_PUSH, node->expr.int_lit.num }));
					break;
				case NODE_EXPR_BIN_OP:
					if(compile_recur(ctx, instructions, node->expr.bin_op.lhs, functions, bpatches, global_vars, vars, NULL))
						return 1;
					if(compile_recur(ctx, instructions, node->expr.bin_op.rhs, functions, bpatches, global_vars, vars, NULL))
						return 1;
					switch(node->expr.bin_op.type) {
						case NODE_EXPR_SUM:
							vector_aappend(instructions, ((Instruction){ INST_SUM, 0 }));
							break;
						case NODE_EXPR_SUB:
							vector_aappend(instructions, ((Instruction){ INST_SUB, 0 }));
							break;
						case NODE_EXPR_MUL:
							vector_aappend(instructions, ((Instruction){ INST_MUL, 0 }));
							break;
						case NODE_EXPR_DIV:
							vector_aappend(instructions, ((Instruction){ INST_DIV, 0 }));
							break;
						default:
							assert(0);
					}
					break;
				case NODE_EXPR_FUN_CALL:
					for(size_t i = 0; i < node->expr.fun_call.args.size; ++i)
						if(compile_recur(ctx, instructions, node->expr.fun_call.args.data[i], functions, bpatches, global_vars, vars, NULL))
						return 1;

					vector_aappend(bpatches, ((BackPatch){ node->expr.fun_call.identifier, instructions->size, node->line }));
					vector_aappend(instructions, ((Instruction){ INST_CALL, 0 }));
					break;
				case NODE_EXPR_VAR_LOOKUP:
					if(!is_global) {
						for(size_t i = 0; i < vars->size; ++i)
							if(!strcmp(node->expr.var_lookup.identifier, vars->data[i].identifier)) {
								vector_aappend(instructions, ((Instruction){ INST_LOAD, vars->data[i].index }));
								return 0;
							}
					}
					for(size_t i = 0; i < global_vars->size; ++i)
						if(!strcmp(node->expr.var_lookup.identifier, global_vars->data[i].identifier)) {
							vector_aappend(instructions, ((Instruction){ INST_LOAD_GLOBAL, global_vars->data[i].index }));
							return 0;
						}
					if(ctx->print_errors)
						printf("%s:%d: error: Undeclared identifier \"%s\"\n",
							ctx->filename, node->line,
							node->expr.var_lookup.identifier);
					return 1;
				case NODE_EXPR_VAR_REASSIGNMENT:
					if(compile_recur(ctx, instructions, node->expr.var_assignment.expr, functions, bpatches, global_vars, vars, NULL))
						return 1;

					if(!is_global) {
						for(size_t i = 0; i < vars->size; ++i)
							if(!strcmp(node->expr.var_assignment.identifier, vars->data[i].identifier)) {
								vector_aappend(instructions, ((Instruction){ INST_STORE, vars->data[i].index }));
								vector_aappend(instructions, ((Instruction){ INST_LOAD, vars->data[i].index }));
								return 0;
							}
					}

					for(size_t i = 0; i < global_vars->size; ++i)
						if(!strcmp(node->expr.var_assignment.identifier, global_vars->data[i].identifier)) {
							vector_aappend(instructions, ((Instruction){ INST_STORE_GLOBAL, global_vars->data[i].index }));
							vector_aappend(instructions, ((Instruction){ INST_LOAD_GLOBAL, global_vars->data[i].index }));
							return 0;
						}
					if(ctx->print_errors)
						printf("%s:%d: error: Undeclared identifier \"%s\"\n",
							ctx->filename, node->line,
							node->expr.var_lookup.identifier);
					return 1;
				default:
					assert(0);
			}
			break;
		case NODE_RET_STATEMENT:
			if(compile_recur(ctx, instructions, node->ret.expr, functions, bpatches, global_vars, vars, NULL))
				return 1;
			vector_aappend(instructions, ((Instruction){ INST_RET, 0 }));
			break;
		case NODE_FUN_STATEMENT: {
			FunctionCtx* fun = lookup_fun_ctx(functions, node);
			fun->start_addr = instructions->size;
			Vector_Variable* merge_or_null = NULL;
			Vector_Variable merge;
			if(node->fun.arguments.size) {
				vector_Variable_ainit(&merge, 64);
				for(size_t i = 0; i < node->fun.arguments.size; ++i) {
					vector_aappend(&merge, ((Variable){ node->fun.arguments.data[i], i }));
					vector_aappend(instructions, ((Instruction){ INST_STORE, i }));
				}

				merge_or_null = &merge;
			}
			if(compile_recur(ctx, instructions, node->fun.body, functions, bpatches, global_vars, vars, merge_or_null)) {
				if(node->fun.arguments.size)
					vector_deinit(merge_or_null);
				return 1;
			}
			if(node->fun.arguments.size)
				vector_deinit(merge_or_null);
			break;
		}
		case NODE_VAR_STATEMENT: {
			if(is_a_redecl(node->var.identifier, global_vars, vars))
				return 1;
			if(compile_recur(ctx, instructions, node->var.expr, functions, bpatches, global_vars, vars, NULL))
				return 1;
			int64_t index = is_global ? global_vars->size : vars->size;
			vector_aappend(is_global ? global_vars : vars, ((Variable){ node->var.identifier, index }));
			vector_aappend(instructions, ((Instruction){ is_global ? INST_STORE_GLOBAL : INST_STORE, index }));
			break;
		}
		default:
			assert(0);
	}
	return 0;
}

int ast_compile(Silk_Ctx* ctx, Vector_Instruction* instructions, ASTNode* node) {
	int ret = 0;

	Vector_FunctionCtx functions;
	vector_FunctionCtx_ainit(&functions, 64);

	Vector_BackPatch bpatches;
	vector_BackPatch_ainit(&bpatches, 64);

	Vector_Variable global_vars;
	vector_Variable_ainit(&global_vars, 64);

	assert(node->type == NODE_SCOPE);
	for(size_t i = 0; i < node->scope.n_nodes; ++i) {
		if(node->scope.nodes[i]->type == NODE_FUN_STATEMENT) {
			vector_aappend(&functions, ((FunctionCtx){ node->scope.nodes[i], 0, 0 }));
			continue;
		}
		if(compile_recur(ctx, instructions, node->scope.nodes[i], &functions, &bpatches, &global_vars, NULL, NULL)) {
			ret = 1;
			goto quit;
		}
	}

	vector_aappend(instructions, ((Instruction){ INST_EXIT, 0 }));

	for(size_t i = 0; i < functions.size; ++i) {
		Vector_Variable scope_vars;
		vector_Variable_ainit(&scope_vars, 64);
		if(compile_recur(ctx, instructions, functions.data[i].node, &functions, &bpatches, &global_vars, &scope_vars, NULL)) {
			vector_deinit(&scope_vars);
			ret = 1;
			goto quit;
		}
		vector_deinit(&scope_vars);
	}

	for(size_t i = 0; i < bpatches.size; ++i) {
		FunctionCtx* fun_ctx = lookup_fun_ctx_by_name(&functions, bpatches.data[i].identifier);
		if(!fun_ctx) {
			if(ctx->print_errors)
				printf("%s:%d: error: Undeclared identifier \"%s\"\n",
				ctx->filename, bpatches.data[i].line,
				bpatches.data[i].identifier);
			ret = 1;
			goto quit;
		}
		instructions->data[bpatches.data[i].code_pos].val = fun_ctx->start_addr;
	}

quit:
	vector_deinit(&functions);
	vector_deinit(&bpatches);
	vector_deinit(&global_vars);

	return ret;
}

const char* ast_node_type_to_str(ASTNodeType type) {
	switch(type) {
#define ENUMERATOR(typ3) case typ3: return &#typ3[5];
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
			printf("%s(", node->fun.identifier);
			for(size_t i = 0; i < node->fun.arguments.size; ++i)
				printf("%s%s", node->fun.arguments.data[i],
					i == node->fun.arguments.size - 1 ? "" : ", ");
			printf(")\n");
			ast_print_node(node->fun.body, indent + 1);
			break;
		case NODE_RET_STATEMENT:
			putchar('\n');
			ast_print_node(node->ret.expr, indent + 1);
			break;
		case NODE_EXPR:
			switch(node->expr.type) {
				case NODE_EXPR_INT_LIT:
					printf("%ld\n", node->expr.int_lit.num);
					break;
				case NODE_EXPR_STR_LIT:
					printf("\"%s\"\n", node->expr.str_lit.str);
					break;
				case NODE_EXPR_BIN_OP:
					printf("%c\n", node->expr.bin_op.type);
					ast_print_node(node->expr.bin_op.lhs, indent + 1);
					ast_print_node(node->expr.bin_op.rhs, indent + 1);
					break;
				case NODE_EXPR_VAR_LOOKUP:
					printf("%s\n", node->expr.var_lookup.identifier);
					break;
				case NODE_EXPR_VAR_REASSIGNMENT:
					printf("%s =\n", node->expr.var_assignment.identifier);
					ast_print_node(node->expr.var_assignment.expr, indent + 1);
					break;
				case NODE_EXPR_FUN_CALL:
					printf("%s()\n", node->expr.fun_call.identifier);
					for(size_t i = 0; i < node->expr.fun_call.args.size; ++i)
						ast_print_node(node->expr.fun_call.args.data[i], indent + 1);
					break;
				default:
					assert(0);
			}
			break;
		case NODE_VAR_STATEMENT:
			printf("%s =\n", node->var.identifier);
			ast_print_node(node->var.expr, indent + 1);
			break;
		default:
			assert(0);
	}
}
