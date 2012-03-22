
void start_kernel(void) __attribute__ (( section (".text.start") ))
{
    init_paging();
    init_mm();
}
