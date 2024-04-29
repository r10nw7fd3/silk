#include <silk.h>
#include <stdio.h>

int main(int argc, char** argv) {
	if(argc != 2) {
		printf("Usage: %s <file.js>\n", argv[0]);
		return 1;
	}

	const char* filename = argv[1];
	int ret = silk_run_file(filename);
	printf("silk_run_file() returned %d\n", ret);
	return ret;
}
