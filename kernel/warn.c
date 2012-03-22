#include <warn.h>

void abort(void)
{
first:
    __asm__ __volatile__ ("hlt\n\t");
    goto first;
}
