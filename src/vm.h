#ifndef _VM_H_
#define _VM_H_

#include <stdint.h>
#include <stddef.h>
#include "instruction.h"

typedef struct {
	int64_t* data;
	size_t capacity;
	size_t sp;
} VM_Stack;

typedef struct {
	int64_t* data;
	size_t capacity;
	size_t size;
} VM_GlobalsTable;

typedef struct {
	VM_Stack stack;
	VM_GlobalsTable table;
} VM;

int vm_init(VM* vm, size_t stack_capacity, size_t table_capacity);
void vm_deinit(VM* vm);

int vm_run(VM* vm, Instruction* instructions, size_t inst_size);

#endif
