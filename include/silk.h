#ifndef _SILK_H_
#define _SILK_H_

#define SILK_API __attribute__((visibility("default")))

typedef struct {
	const char* filename;
	char print_tokens;
	char print_ast;
	char print_bytecode;
	char print_stack_on_exit;
	char print_errors;
} Silk_Ctx;

SILK_API int silk_ctx_init(Silk_Ctx* ctx);
SILK_API void silk_ctx_deinit(Silk_Ctx* ctx);

SILK_API int silk_run_file(Silk_Ctx* ctx, const char* filename);
SILK_API int silk_run_string(Silk_Ctx* ctx, const char* js_data);
SILK_API int silk_run(Silk_Ctx* ctx, const char* js_data, const char* js_data_end);

#endif
