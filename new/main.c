#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"
#define VERSION "0.step5"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Compiler Book test - %s\n", VERSION);
		fprintf(stderr, "Usage: %s <'code'>\n", argv[0]);
		return 1;
	}
	Token *tok = tokenize(argv[1]);
	Obj *obj = parse(tok);
	codegen(obj);
	return 0;
}
