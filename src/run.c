#include <silk.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "vm.h"

static int map_file(const char* filename, char** mem, size_t* file_size) {
	int fd = open(filename, O_RDONLY);
	if(fd == -1)
		return -1;

	struct stat st;
	if(fstat(fd, &st) == -1)
		goto error;
	*file_size = st.st_size;

	*mem = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if(*mem == MAP_FAILED)
		goto error;

	return fd;

error:
	close(fd);
	return -1;
}

int silk_run_file(Silk_Ctx* ctx, const char* filename) {
	char* file;
	size_t size;
	int fd = map_file(filename, &file, &size);
	if(fd == -1)
		return 1;

	if(!ctx->filename)
		ctx->filename = filename;

	int ret = silk_run(ctx, file, file + size);
	close(fd);

	return ret;
}

int silk_run_string(Silk_Ctx* ctx, const char* js_data) {
	size_t len = strlen(js_data);
	return silk_run(ctx, js_data, js_data + len);
}

static inline int intlen(size_t i) {
	int len = 1;
	while(i > 9) {
		++len;
		i /= 10;
	}
	return len;
}

int silk_run(Silk_Ctx* ctx, const char* js_data, const char* js_data_end) {
	if(!ctx->filename)
		ctx->filename = "(unnamed)";

	Lexer lexer;
	if(lexer_init(&lexer, ctx, js_data, js_data_end))
		return 1;

	Parser parser;
	if(parser_init(&parser, &lexer))
		return 1;

	ASTNode* root = parser_parse(&parser);
	if(!root)
		return 1;

	Vector_Instruction insts;
	vector_ainit(insts, 64);
	if(ast_compile(&insts, root)) {
		ast_destroy(root);
		return 1;
	}

	VM vm;
	if(vm_init(&vm, 64, 64)) {
		ast_destroy(root);
		vector_deinit(insts);
	}

	if(ctx->print_bytecode) {
		for(size_t i = 0; i < insts.size; ++i) {
			printf("%*zu: ", intlen(insts.size), i);
			instruction_print(&insts.data[i]);
		}
	}

	if(vm_run(&vm, insts.data, insts.size)) {
		vm_deinit(&vm);
		ast_destroy(root);
		vector_deinit(insts);
		return 1;
	}

	if(ctx->print_stack_on_exit) {
		puts("-----");
		size_t sz = vm.operand_stack.sp;
		for(size_t i = 0; i < vm.operand_stack.sp; ++i)
			printf("%ld\n", vm.operand_stack.data[sz - i - 1]);
		puts("-----");
	}

	vm_deinit(&vm);
	ast_destroy(root);
	vector_deinit(insts);

	return 0;
}
