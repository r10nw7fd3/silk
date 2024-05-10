#include <silk.h>
#include <string.h>

int silk_ctx_init(Silk_Ctx* ctx) {
	memset(ctx, 0, sizeof(Silk_Ctx));
	return 0;
}

void silk_ctx_deinit(Silk_Ctx* ctx) {
	(void) ctx;
}
