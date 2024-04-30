#include "instruction.h"
#include <stdio.h>

const char* instruction_type_to_str(InstructionType type) {
	switch(type) {
#define ENUMERATOR(inst) case inst: return #inst + 5;
FOR_EACH_INSTRUCTION(ENUMERATOR)
#undef ENUMERATOR
		default: return "(invalid instruction)";
	}
}

void instruction_print(Instruction* inst) {
	printf("%s ", instruction_type_to_str(inst->type));
	if(inst->type == INST_PUSH || inst->type == INST_SWAP ||
		inst->type == INST_CALL || inst->type == INST_LOAD ||
		inst->type == INST_STORE)
		printf("%ld", inst->val);
	putchar('\n');
}
