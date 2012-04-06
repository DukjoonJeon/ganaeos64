#include <mm.h>
#include <delay.h>

extern void start_kernel(void) __attribute__ ((section (".text.start")));

void memory_test(void)
{
	char *a;
	char *b;
	char *c;
	char *d;

	a = (char *)alloc_heap(1);
	b = (char *)alloc_heap(10);
	c = (char *)alloc_heap(100);
	d = (char *)alloc_heap(10);

	free_heap(a);
	free_heap(b);

	a = (char *)alloc_heap(10);
	b = (char *)alloc_heap(11);

	free_heap(c);

	for (int i = 0; i < 1000; i++) {
		a = (char *)alloc_heap(3000);
	}
}

void start_kernel(void)
{
	init_mm();



	while (1) {
		halt();
	}
} 
