#include "instruction.h"
#include <stdio.h>

const char* instruction_type_to_str(InstructionType type) {
	switch(type) {
#define ENUMERATOR(inst) case inst: return &#inst[5];
FOR_EACH_INSTRUCTION(ENUMERATOR)
#undef ENUMERATOR
		default: return "(invalid instruction)";
	}
}

void instruction_print(Instruction* inst) {
	printf("%s ", instruction_type_to_str(inst->type));
	switch(inst->type) {
		case INST_PUSH:
		case INST_SWAP:
		case INST_CALL:
		case INST_LOAD:
		case INST_STORE:
		case INST_LOAD_GLOBAL:
		case INST_STORE_GLOBAL:
			printf("%ld", inst->val);
		default:
			break;
	}
	putchar('\n');
}
