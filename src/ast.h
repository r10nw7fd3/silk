#ifndef _AST_H_
#define _AST_H_

#include <stddef.h>
#include <stdint.h>
#include <silk.h>

#include "instruction.h"
#include "vector.h"

#define FOR_EACH_NODE(_) \
	_(NODE_SCOPE) \
	_(NODE_EXPR) \
	_(NODE_FUN_STATEMENT) \
	_(NODE_RET_STATEMENT) \
	_(NODE_VAR_STATEMENT)

typedef enum {
#define ENUMERATOR(node) node,
	FOR_EACH_NODE(ENUMERATOR)
#undef ENUMERATOR
} ASTNodeType;

typedef char* str_t;
#ifndef VECTOR_DEFINED_str_t
#define VECTOR_DEFINED_str_t
VECTOR_DEFINE(str_t)
#endif

typedef struct ASTNode ASTNode;
typedef ASTNode* ASTNode_ptr_t;
#ifndef VECTOR_DEFINED_ASTNode_ptr_t
#define VECTOR_DEFINED_ASTNode_ptr_t
VECTOR_DEFINE(ASTNode_ptr_t)
#endif

struct ASTNode {
	ASTNodeType type;
	int line;
	union {
		struct {
			size_t n_nodes;
			ASTNode** nodes;
		} scope;
		struct {
			enum {
				NODE_EXPR_INT_LIT,
				NODE_EXPR_STR_LIT,
				NODE_EXPR_BIN_OP,
				NODE_EXPR_VAR_LOOKUP,
				NODE_EXPR_VAR_REASSIGNMENT,
				NODE_EXPR_FUN_CALL
			} type;
			union {
				struct {
					int64_t num;
				} int_lit;
				struct {
					char* str;
				} str_lit;
				struct {
					enum {
						NODE_EXPR_SUM = '+',
						NODE_EXPR_SUB = '-',
						NODE_EXPR_MUL = '*',
						NODE_EXPR_DIV = '/'
					} type;
					ASTNode* lhs;
					ASTNode* rhs;
				} bin_op;
				struct {
					char* identifier;
				} var_lookup;
				struct {
					char* identifier;
					ASTNode* expr;
				} var_assignment;
				struct {
					char* identifier;
					Vector_ASTNode_ptr_t args;
				} fun_call;
			};
		} expr;
		struct {
			char* identifier;
			Vector_str_t arguments;
			ASTNode* body;
		} fun;
		struct {
			ASTNode* expr;
		} ret;
		struct {
			char* identifier;
			ASTNode* expr;
		} var;
	};
};

#ifndef VECTOR_DEFINED_Instruction
#define VECTOR_DEFINED_Instruction
VECTOR_DEFINE(Instruction)
#endif

ASTNode* ast_create_node(ASTNode node);

void ast_destroy(ASTNode* node);

int ast_compile(Silk_Ctx* ctx, Vector_Instruction* instructions, ASTNode* node);

const char* ast_node_type_to_str(ASTNodeType node);
void ast_print_node(ASTNode* root, int indent);

#endif
