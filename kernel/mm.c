#include <mm.h>

enum physical_page_index {
    PHYSICAL_PAGES_1 = 0,
    PHYSICAL_PAGES_2 = 1,
    PHYSICAL_PAGES_4 = 2,
    PHYSICAL_PAGES_UNLIMITED = 3,
    PHYSICAL_PAGES_SIZE = 4,
};

enum physical_page_length {
    PHYSICAL_PAGE_LENGTH_1 = 1,
    PHYSICAL_PAGE_LENGTH_2 = 2,
    PHYSICAL_PAGE_LENGTH_4 = 4,
};

struct physical_page {
    uint64 present:1;
    uint64 addr:63;
    uint32 length;
    struct *physical_page *next;
};

static uint64 physical_memory_size = 0;
static struct physical_page physical_page_head[PHYSICAL_PAGES_SIZE];

static uint64 calc_physical_mem_size(void)
{
    /* TODO: implement getting physical memory size */
    return physical_memory_size = 64 * 1024 * 1024;
}

uint64 get_physical_mem_size(void)
{
    ASSERT(physical_memory_size != 0);
    return physical_memory_size;
}

static uint8 get_pageindex_by_pagesize(uint32)
{
    uint8 index;
    if (nr_physical_pages <= PHYSICAL_PAGE_LENGTH_1)
        index = PHYSICAL_PAGES_1;
    else if (nr_physical_pages <= PHYSICAL_PAGE_LENGTH_2)
        index = PHYSICAL_PAGES_2;
    else if (nr_physical_pages <= PHYSICAL_PAGE_LENGTH_4)
        index = PHYSICAL_PAGES_4;
    else if (nr_physical_pages <= PHYSICAL_PAGE_LENGTH_8)
        index = PHYSICAL_PAGES_8;
    else 
        index = PHYSICAL_PAGES_UNLIMITED;

    return index;
}

static void init_free_physical_page(void)
{
    struct physical_page ppage;
    uint64 nr_physical_pages;
    uint8 loopi;
    ASSERT(physical_memory_size != 0);

    for (loopi = 0; loopi < PHYSICAL_PAGES_SIZE; loopi++)
        physical_page_head[loopi].present = 0;

    nr_physical_pages = physical_memory_size / PAGESIZE;
    nr_physical_pages -= UP_TO_DIRECT_MAPPED_PAGE / PAGESIZE;
    ppage.present = 1;
    ppage.addr = UP_TO_DIRECT_MAPPED_PAGE;
    ppage.length = nr_physical_pages;
    ppage.next = NULL;

    physical_page_head[get_pageindex_by_pagesize(nr_physical_pages)] = ppage;
}

boolean get_free_physical_pages(uint32 length, uint64 *freepage_addr)
{
    int index;
    struct physical_page *head;
    
    ASSERT(length > 0);
    ASSERT(freepage_addr != NULL);

    index = get_pageindex_by_pagesize(length);
    head = &physical_page_head[index];

    if (head->length == length) {
        *head = head->next;
    } else if (head->length < length) {
        int mod_index;

        head->length -= length;
        *freepage_addr = head->addr;
        head->addr += PAGESIZE * length;

        if (index != (mod_index = get_pageindex_by_pagesize(length))) {
            *head = head->next;
        }
    } else {
        return FALSE;
    }

    return TRUE;
}

uint64 get_free_physical_page(void)
{
    get_free_physical_pages(1);
}

static void init_paging(uint64)
{
    int16 nr_required_pml4_entry = UP_TO_DIRECT_MAPPED_PAGE >> ;
    int16 nr_required_pdpt_entry;
    int16 nr_required_pd_entry;
    int16 nr_required_page_entry;

    ASSERT(physical_memory_size != 0);

    init_free_physical_page();
}

void init_mm(void)
{
    uint64 mem_size; 
    
    mem_size = calc_physical_mem_size();
}
