#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

#include <stdint.h>

#define FOR_EACH_INSTRUCTION(_) \
	_(INST_PUSH) \
	_(INST_POP) \
	_(INST_SWAP) \
	_(INST_LOAD) \
	_(INST_STORE) \
	_(INST_EXIT) \
	_(INST_CALL) \
	_(INST_RET) \
	_(INST_SUM) \
	_(INST_SUB) \
	_(INST_MUL) \
	_(INST_DIV)

typedef enum {
#define ENUMERATOR(inst) inst,
FOR_EACH_INSTRUCTION(ENUMERATOR)
#undef ENUMERATOR
} InstructionType;

typedef struct {
	InstructionType type;
	int64_t val;
} Instruction;

const char* instruction_type_to_str(InstructionType type);
void instruction_print(Instruction* inst);

#endif
