#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chibicc.h"
#define VERSION "0.step26"

int main(int argc, char *argv[]) {
	char *file = 0;
	int arg = 1;
	while (arg < argc) {
		if (!strcmp(argv[arg], "-x")) {
			arg++;
			if (arg >= argc) {
				fprintf(stderr, "Missing argument\n");
				exit(1);
			}
			if (strcmp(argv[arg], "c")) {
				fprintf(stderr, "Bad argument\n");
				exit(1);
			}
			arg++;
			continue;
		}
		file = argv[arg];
		break;
	}
	if (!file) {
		fprintf(stderr, "Compiler Book test - %s\n", VERSION);
		fprintf(stderr, "Usage: %s [-x c] <file.c>\n", argv[0]);
		return 1;
	}
	Token *tok = tokenize(file);
	Obj *obj = parse(tok);
	codegen(obj);
	return 0;
}
