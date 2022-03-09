
#

#ifdef DEFINED
int main() {
	printf("OK\n");
	return 0;
}
#else
#define DEFINED
#include "macro.c"
#endif
