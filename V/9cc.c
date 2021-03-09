#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s 'expr'\n", argv[0]);
		return 1;
	}
	char *p = argv[1];
	printf("exit(%ld", strtol(p, &p, 10));
	while (*p) {
		if (*p == '+') {
			p++;
			printf("+%ld", strtol(p, &p, 10));
		} else if (*p == '-') {
			p++;
			printf("-%ld", strtol(p, &p, 10));
		} else {
			fprintf(stderr, "Unexpected character '%c'\n", *p);
			return 1;
		}
	}
	printf(")\n");
	return 0;
}
