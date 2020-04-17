#include <stdio.h>


// assert, for debug
#define ASSERT(condition)                                                     \
    if (!(condition)) {                                                       \
        fprintf(stderr, "Assertion failed: line %d, file \"%s\"\n",           \
                __LINE__, __FILE__);                                          \
		fflush(stderr);							      \
        abort();                                                              \
    }