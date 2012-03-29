#include <mm.h>

__attribute__ (( section (".text.start") )) void start_kernel(void)
{
	init_mm();
} 
