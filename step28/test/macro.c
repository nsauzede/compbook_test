#

#ifdef DEFINED
#include "test.h"
int main() {
	printf("OK\n");
	return 0;
}
#else
#define DEFINED
#include "macro.c"
#endif
