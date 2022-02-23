//#define ASSERT(x, y) assert2(__FILE__, __LINE__, x, y, #y)
//#include "common"

#define ASSERT(x, y) assert(x, y, #y)