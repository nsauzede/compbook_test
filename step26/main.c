#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chibicc.h"
#define VERSION "0.step26"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Compiler Book test - %s\n", VERSION);
		fprintf(stderr, "Usage: %s <file.c>\n", argv[0]);
		return 1;
	}
	Token *tok = tokenize(argv[1]);
	Obj *obj = parse(tok);
	codegen(obj);
	return 0;
}
