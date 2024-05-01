#include "vm.h"
#include <stdlib.h>
#include <assert.h>

typedef struct {
	int64_t* data;
	size_t capacity;
} VM_LocalsTable;

typedef struct {
	VM_LocalsTable locals;
} VM_CallFrame;

static inline int stack_init(VM_Stack* stack, size_t stack_capacity) {
	assert(sizeof(int64_t) >= sizeof(void*));
	stack->data = malloc(sizeof(int64_t) * stack_capacity);
	if(!stack->data)
		return 1;
	stack->capacity = stack_capacity;
	stack->sp = 0;
	return 0;
}

static inline void stack_deinit(VM_Stack* stack) {
	free(stack->data);
	stack->capacity = 0;
	stack->sp = 0;
}

static inline void stack_push(VM_Stack* stack, int64_t val) {
	assert(stack->sp < stack->capacity);
	stack->data[stack->sp++] = val;
}

static inline int64_t stack_pop(VM_Stack* stack) {
	assert(stack->sp);
	return stack->data[--stack->sp];
}

static inline int64_t stack_peek(VM_Stack* stack) {
	assert(stack->sp);
	return stack->data[stack->sp - 1];
}

static inline int table_init(VM_LocalsTable* table, size_t table_capacity) {
	table->data = malloc(sizeof(int64_t) * table_capacity);
	if(!table->data)
		return 1;
	table->capacity = table_capacity;
	return 0;
}

static inline void table_deinit(VM_LocalsTable* table) {
	free(table->data);
	table->capacity = 0;
}

static inline int64_t table_get(VM_LocalsTable* table, size_t i) {
	assert(i < table->capacity);
	return table->data[i];
}

static inline void table_put(VM_LocalsTable* table, size_t i, int64_t val) {
	assert(i < table->capacity);
	table->data[i] = val;
}

int vm_init(VM* vm, size_t stack_capacity, size_t table_capacity) {
	if(stack_init(&vm->operand_stack, stack_capacity))
		return 1;
	if(stack_init(&vm->call_stack, stack_capacity)) {
		stack_deinit(&vm->operand_stack);
		return 1;
	}
	vm->table_capacity = table_capacity;
	return 0;
}

void vm_deinit(VM* vm) {
	stack_deinit(&vm->operand_stack);
	stack_deinit(&vm->call_stack);
}

static inline VM_CallFrame* cf_create(size_t table_capacity) {
	VM_CallFrame* cf = malloc(sizeof(VM_CallFrame));
	assert(!cf || !table_init(&cf->locals, table_capacity));
	return cf;
}

static inline void cf_destroy(VM_CallFrame* cf) {
	table_deinit(&cf->locals);
	free(cf);
}

int vm_run(VM* vm, Instruction* instructions, size_t inst_size) {
	VM_CallFrame* global_cf = cf_create(vm->table_capacity);
	stack_push(&vm->call_stack, (int64_t) global_cf);

	int64_t val1;
	int64_t val2;
	for(size_t pc = 0; pc < inst_size; ++pc) {
		Instruction* inst = &instructions[pc];
		VM_CallFrame* current_cf = (VM_CallFrame*) stack_peek(&vm->call_stack);
		switch(inst->type) {
			case INST_PUSH:
				stack_push(&vm->operand_stack, inst->val);
				break;
			case INST_POP:
				stack_pop(&vm->operand_stack);
				break;
			case INST_SWAP: {
				assert(vm->operand_stack.sp > (size_t) inst->val);
				val1 = vm->operand_stack.data[vm->operand_stack.sp - 1];
				size_t other_pos = vm->operand_stack.sp - 1 - inst->val;
				vm->operand_stack.data[vm->operand_stack.sp - 1] = vm->operand_stack.data[other_pos]; 
				vm->operand_stack.data[other_pos] = val1;
				break;
			}
			case INST_LOAD:
				val1 = table_get(&current_cf->locals, inst->val);
				stack_push(&vm->operand_stack, val1);
				break;
			case INST_STORE:
				val1 = stack_pop(&vm->operand_stack);
				table_put(&current_cf->locals, inst->val, val1);
				break;
			case INST_EXIT:
				goto quit;
			case INST_CALL: {
				VM_CallFrame* new_cf = cf_create(vm->table_capacity);
				stack_push(&vm->call_stack, (int64_t) new_cf);
				stack_push(&vm->operand_stack, pc + 1);
				pc = inst->val - 1;
				break;
			}
			case INST_RET: {
				VM_CallFrame* old_cf = (VM_CallFrame*) stack_pop(&vm->call_stack);
				cf_destroy(old_cf);
				size_t ret_addr = (size_t) stack_pop(&vm->operand_stack);
				pc = ret_addr - 1;
				break;
			}
			case INST_SUM:
				val1 = stack_pop(&vm->operand_stack);
				val2 = stack_pop(&vm->operand_stack);
				stack_push(&vm->operand_stack, val2 + val1);
				break;
			case INST_SUB:
				val1 = stack_pop(&vm->operand_stack);
				val2 = stack_pop(&vm->operand_stack);
				stack_push(&vm->operand_stack, val2 - val1);
				break;
			case INST_MUL:
				val1 = stack_pop(&vm->operand_stack);
				val2 = stack_pop(&vm->operand_stack);
				stack_push(&vm->operand_stack, val2 * val1);
				break;
			case INST_DIV:
				val1 = stack_pop(&vm->operand_stack);
				val2 = stack_pop(&vm->operand_stack);
				stack_push(&vm->operand_stack, val2 / val1);
				break;
			default:
				assert(0);
		}
	}
quit:
	while(vm->call_stack.sp) {
		VM_CallFrame* cf = (VM_CallFrame*) stack_pop(&vm->call_stack);
		cf_destroy(cf);
	}
	return 0;
}
