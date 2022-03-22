#define ASSERT(x, y) printf("%s:%d:", __FILE__, __LINE__); \
	assert(x, y, #y)

void assert(int expected, int actual, char *code);
int printf();
