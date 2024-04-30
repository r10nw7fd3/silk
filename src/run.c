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

int silk_run_file(const char* filename) {
	char* file;
	size_t size;
	int fd = map_file(filename, &file, &size);
	if(fd == -1)
		return 1;

	int ret = silk_run(file, file + size);
	close(fd);

	return ret;
}

int silk_run_string(const char* js_data) {
	size_t len = strlen(js_data);
	return silk_run(js_data, js_data + len);
}

#define PRINT_TOKENS 0

#if PRINT_TOKENS
int silk_run(const char* js_data, const char* js_data_end) {
	Lexer lexer;
	lexer_init(&lexer, js_data, js_data_end);
	Token tok;
	lexer_next(&lexer, &tok);
	while(tok.type != TOKEN_EOF) {
		lexer_print_token(&tok);
		lexer_next(&lexer, &tok);
	}
	return 0;
}
#else
int silk_run(const char* js_data, const char* js_data_end) {
	Lexer lexer;
	if(lexer_init(&lexer, js_data, js_data_end))
		return 1;

	Parser parser;
	if(parser_init(&parser, &lexer))
		return 1;

	ASTNode* root = parser_parse(&parser);
	if(!root)
		return 1;

	Instruction* insts = NULL;
	size_t size = 0;
	if(ast_compile(&insts, 1024, &size, root)) {
		ast_destroy(root);
		return 1;
	}

	VM vm;
	if(vm_init(&vm, 64, 64)) {
		ast_destroy(root);
		free(insts);
	}

	for(size_t i = 0; i < size; ++i)
		instruction_print(&insts[i]);

	if(vm_run(&vm, insts, size)) {
		puts("vm_run() failed");
		vm_deinit(&vm);
		ast_destroy(root);
		free(insts);
		return 1;
	}

	vm_deinit(&vm);
	ast_destroy(root);
	free(insts);

	return 0;
}

#endif
