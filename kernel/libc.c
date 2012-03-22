#include <libc.h>
#include <types.h>
#include <warn.h>

int memset(__IN void *target, __IN uint8 value, __IN uint64 size)
{
    uint64 loopi;

    ASSERT(target != NULL);

    for (loopi = 0; loopi < size; loopi++)
        *((uint8 *)target + loopi) = value;

    return loopi;
}

