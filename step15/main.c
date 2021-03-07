#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <'code'>\n", argv[0]);
		return 1;
	}
	tokenize(argv[1]);
	parse();
	generate();
	return 0;
}
