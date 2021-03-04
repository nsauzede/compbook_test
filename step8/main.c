#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

Token *token;
char *user_input;

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <number>\n", argv[0]);
		return 1;
	}
	user_input = argv[1];
	token = tokenize(user_input);
	Node *node = expr();
	printf(".intel_syntax noprefix\n");
	printf(".globl main\n");
	printf("\n");
	printf("main:\n");
	gen(node);
	printf("\tpop rax\n");
	printf("\tret\n");
	return 0;
}
