#include "vm.h"
#include <stdlib.h>
#include <assert.h>

static int stack_init(VM_Stack* stack, size_t stack_capacity) {
	stack->data = malloc(sizeof(int64_t) * stack_capacity);
	if(!stack->data)
		return 1;
	stack->capacity = stack_capacity;
	stack->sp = 0;
	return 0;
}

static void stack_deinit(VM_Stack* stack) {
	free(stack->data);
	stack->capacity = 0;
	stack->sp = 0;
}

static void stack_push(VM_Stack* stack, int64_t val) {
	assert(stack->sp < stack->capacity);
	stack->data[stack->sp++] = val;
}

static int64_t stack_pop(VM_Stack* stack) {
	assert(stack->sp);
	return stack->data[--stack->sp];
}

static int table_init(VM_GlobalsTable* table, size_t table_capacity) {
	table->data = malloc(sizeof(int64_t) * table_capacity);
	if(!table->data)
		return 1;
	table->capacity = table_capacity;
	table->size = 0;
	return 0;
}

static void table_deinit(VM_GlobalsTable* table) {
	free(table->data);
	table->capacity = 0;
	table->size = 0;
}

static int64_t table_get(VM_GlobalsTable* table, size_t i) {
	assert(i < table->size);
	return table->data[i];
}

static size_t table_put(VM_GlobalsTable* table, int64_t val) {
	assert(table->size < table->capacity);
	table->data[table->size++] = val;
	return table->size - 1;
}

int vm_init(VM* vm, size_t stack_capacity, size_t table_capacity) {
	if(stack_init(&vm->stack, stack_capacity))
		return 1;
	if(table_init(&vm->table, table_capacity)) {
		stack_deinit(&vm->stack);
		return 1;
	}
	return 0;
}

void vm_deinit(VM* vm) {
	stack_deinit(&vm->stack);
	table_deinit(&vm->table);
}

int vm_run(VM* vm, Instruction* instructions, size_t inst_size) {
	int64_t val1;
	int64_t val2;
	for(size_t pc = 0; pc < inst_size; ++pc) {
		Instruction* inst = &instructions[pc];
		switch(inst->type) {
			case INST_PUSH:
				stack_push(&vm->stack, inst->val);
				break;
			case INST_POP:
				stack_pop(&vm->stack);
				break;
			case INST_SWAP: {
				// This makes it possible to get arguments from
				// a function call. TODO: Should probably be replaced
				// with a call stack
				assert(vm->stack.sp > (size_t) inst->val);
				val1 = vm->stack.data[vm->stack.sp - 1];
				size_t other_pos = vm->stack.sp - 1 - inst->val;
				vm->stack.data[vm->stack.sp - 1] = vm->stack.data[other_pos]; 
				vm->stack.data[other_pos] = val1;
				break;
			}
			case INST_LOAD:
				int64_t val = table_get(&vm->table, (size_t) stack_pop(&vm->stack));
				stack_push(&vm->stack, val);
				break;
			case INST_STORE:
				size_t index = table_put(&vm->table, stack_pop(&vm->stack));
				stack_push(&vm->stack, (int64_t) index);
				break;
			case INST_EXIT:
				return 0;
			case INST_CALL: {
				stack_push(&vm->stack, pc + 1);
				pc = inst->val - 1;
				break;
			}
			case INST_RET: {
				size_t ret_addr = (size_t) stack_pop(&vm->stack);
				pc = ret_addr - 1;
				break;
			}
			case INST_SUM:
				val1 = stack_pop(&vm->stack);
				val2 = stack_pop(&vm->stack);
				stack_push(&vm->stack, val2 + val1);
				break;
			case INST_SUB:
				val1 = stack_pop(&vm->stack);
				val2 = stack_pop(&vm->stack);
				stack_push(&vm->stack, val2 - val1);
				break;
			case INST_MUL:
				val1 = stack_pop(&vm->stack);
				val2 = stack_pop(&vm->stack);
				stack_push(&vm->stack, val2 * val1);
				break;
			case INST_DIV:
				val1 = stack_pop(&vm->stack);
				val2 = stack_pop(&vm->stack);
				stack_push(&vm->stack, val2 / val1);
				break;
			default:
				assert(0);
		}
	}
	return 0;
}
