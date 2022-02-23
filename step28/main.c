#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chibicc.h"
#define VERSION "0.step26"

int main(int argc, char *argv[]) {
	char *in = 0;
	char *out = 0;
	int arg = 1;
	while (arg < argc) {
		if (*argv[arg] == '-' && strlen(argv[arg]) > 1) {

		if (!strcmp(argv[arg], "-S")) {
			arg++;
			continue;
		}
		if (!strcmp(argv[arg], "-o")) {
			if (out) {
				fprintf(stderr, "Output already defined\n");
				exit(1);
			}
			arg++;
			if (arg >= argc) {
				fprintf(stderr, "Missing argument\n");
				exit(1);
			}
			out = argv[arg++];
			continue;
		}
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

			fprintf(stderr, "Unkown option '%s'\n", argv[arg]);
			exit(1);
		}
		if (!in) {
			in = argv[arg++];
		} else {
			fprintf(stderr, "Input already defined\n");
			exit(1);
		}
	}
	if (!in) {
		fprintf(stderr, "Compiler Book test - %s\n", VERSION);
		fprintf(stderr, "Usage: %s [-x c] <file.c>\n", argv[0]);
		return 1;
	}
	Token *tok = tokenize(in);
	Obj *obj = parse(tok);
	char *assembly = codegen(obj);
	if (out) {
		FILE *f = fopen(out, "wt");
		fwrite(assembly, strlen(assembly), 1, f);
		fclose(f);
	} else {
		puts(assembly);
	}
	return 0;
}
