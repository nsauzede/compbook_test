#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

Node *code[100];
char *user_input;

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <'code'>\n", argv[0]);
		return 1;
	}
	user_input = argv[1];
	tokenize();
	program();
	printf(".intel_syntax noprefix\n");
	printf(".globl main\n");
	printf("\n");
	printf("main:\n");
	printf("\tpush rbp\n");
	printf("\tmov rbp, rsp\n");
	printf("\tsub rsp, 208\n");
	for (int i = 0; code[i]; i++) {
		gen(code[i]);
		printf("\tpop rax\n");
	}
	printf("\tmov rsp,rbp\n");
	printf("\tpop rbp\n");
	printf("\tret\n");
	return 0;
}
