#include "test.h"

int main() {
  ASSERT(1, ({ int x[3]={1, 2, 3}; x[0]; }));
  ASSERT(2, ({ int x[3]={1, 2, 3}; x[1]; }));
  ASSERT(3, ({ int x[3]={1, 2, 3}; x[2]; }));

  printf("OK\n");
  return 0;
}
