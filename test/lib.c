/**
 * Simple implementation of common library functions
 */

#include "lib.h"


void print_c(char ch)
{
    asm("li a7, 1;"
        "scall");
}

void print_d(int num)
{
    asm("li a7, 2;"
        "scall");
}

void exit_proc() {
    asm("li a7, 10;"
        "scall");
}
