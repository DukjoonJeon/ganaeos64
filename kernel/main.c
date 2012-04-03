#include <mm.h>
#include <delay.h>

extern void start_kernel(void) __attribute__ ((section (".text.start")));

void start_kernel(void)
{
	init_mm();

	while (1) {
		halt();
	}
} 
