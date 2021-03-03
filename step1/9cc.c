#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <number>\n", argv[0]);
		return 1;
	}
	int number = 0;
	int arg = 1;
	sscanf(argv[arg++], "%d", &number);
	printf(".intel_syntax noprefix\n");
	printf(".globl main\n");
	printf("\n");
	printf("main:\n");
	printf("\tmov rax, %d\n", number);
	printf("\tret\n");
	return 0;
}
