#ifndef _AST_H_
#define _AST_H_

#include <stddef.h>
#include <stdint.h>

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

typedef struct ASTNode ASTNode;
struct ASTNode {
	ASTNodeType type;
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
				NODE_EXPR_FUN_CALL
			} kind;
			union {
				int64_t num;
				char* data;
				struct {
					enum {
						NODE_EXPR_SUM = '+',
						NODE_EXPR_SUB = '-',
						NODE_EXPR_MUL = '*',
						NODE_EXPR_DIV = '/'
					} op;
					ASTNode* lhs;
					ASTNode* rhs;
				};
			};
		} expr;
		struct {
			char* identifier;
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

ASTNode* ast_node_create(ASTNode node);

void ast_destroy(ASTNode* node);

const char* ast_node_type_to_str(ASTNodeType node);
void ast_print_node(ASTNode* root, int indent);

#endif
