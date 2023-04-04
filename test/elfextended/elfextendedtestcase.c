/*   Copyright (c) 2023 David Anderson
    This trivial test code is hereby placed in the public domain
    for anyone to use for any purpose. */

/*  Not to be compiled. This test case should remain unchanged.
    gcc -gdwarf-5 -m32 -O0 elfextendedtestcase.c -o testobj

    gcc -gdwarf-5 -m64 -O0 elfextendedtestcase.c -o testobj64
*/
#if 0
#include "stdio.h"

int main()
{
    int i = 0;
    int max = 5;
    int sum = 0;

    for ( ; i < max; ++i) {
        sum += i;
    }
    return sum;
}
#endif
