#include <delay.h>

void halt(void)
{
	__asm__ __volatile__ ("hlt\n\t");
}
