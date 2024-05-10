#include <silk.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
	if(argc < 2) {
		printf("Usage: %s [-t|-a|-b|-s|-e] <file.js>\n", argv[0]);
		return 1;
	}

	Silk_Ctx ctx;
	silk_ctx_init(&ctx);
	ctx.print_tokens = 0;
	ctx.print_ast = 0;
	ctx.print_bytecode = 0;
	ctx.print_stack_on_exit = 0;
	ctx.print_errors = 0;

	int i;
	for(i = 1; i < argc - 1; ++i) {
		if(!strcmp(argv[i], "-t"))
			ctx.print_tokens = 1;
		else if(!strcmp(argv[i], "-a"))
			ctx.print_ast = 1;
		else if(!strcmp(argv[i], "-b"))
			ctx.print_bytecode = 1;
		else if(!strcmp(argv[i], "-s"))
			ctx.print_stack_on_exit = 1;
		else if(!strcmp(argv[i], "-e"))
			ctx.print_errors = 1;
		else {
			printf("Unknown argument \"%s\"\n", argv[i]);
			return 1;
		}
	}

	const char* filename = argv[i];
	int ret = silk_run_file(&ctx, filename);
	printf("silk_run_file() returned %d\n", ret);

	silk_ctx_deinit(&ctx);
	return ret;
}
