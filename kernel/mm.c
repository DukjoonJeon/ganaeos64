#include <mm.h>
#include <types.h>
#include <warn.h>
#include <libc.h>

struct physical_memory_block {
    void *addr;
    struct physical_memory_block *next;
};

#define PAGESIZE 4096

#define INIT_FREE_PHY_MEM_START (2 * 1024 * 1024)
#define INIT_FREE_PHY_MEM_END (4 * 1024 * 1024 - 1)

struct physical_memory_block *start_entry = NULL;

static boolean is_aligned(void *addr, uint32 align)
{
    ASSERT(align >= 1);

    if ((addr_t)addr & (addr_t)(align - 1) != 0)
        return FALSE;

    return TRUE;
}

static page_t *get_pml4(void)
{
    addr_t  pml4_addr;
    __asm__ __volatile__ (
        "movq %%cr3, %%rax\n\t"
        : "=a" (pml4_addr)
        : /* no input */
    );

    return (page_t *)pml4_addr;
}

static boolean get_child_page_by_index(__IN page_t *parent_addr,
                                       __IN uint16 index,
                                       __OUT page_t **child_addr)
{
    page_t result;

    ASSERT((index >= 0) && (index < 512));
    ASSERT(parent_addr != NULL);
    ASSERT(((addr_t)parent_addr & (addr_t)(PAGESIZE-1)) == 0);
    ASSERT(*child_addr != NULL);

    result = parent_addr[index];
    *child_addr = (page_t *)(result & ~PAGE_ATTR);

    if ((result & PAGE_P_BIT) == 0)
        return FALSE;
    return TRUE;
}

static uint16 get_pml4_index(__IN void *logical_addr)
{
    ASSERT((addr_t)logical_addr < ((addr_t)1 << 48));
    
    return (addr_t)logical_addr >> (9 + 9 + 9 + 12);
}

static uint16 get_pdpte_index(__IN void *logical_addr)
{
    ASSERT((addr_t)logical_addr < ((addr_t)1 << 48));

    return (addr_t)logical_addr >> (9 + 9 + 12) & 0x1ff;
}

static uint16 get_pde_index(__IN void *logical_addr)
{
    ASSERT((addr_t)logical_addr < ((addr_t)1 << 48));

    return ((addr_t)logical_addr >> (9 + 12)) & 0x01ff;
}

static uint16 get_pte_index(__IN void *logical_addr)
{
    ASSERT((addr_t)logical_addr < ((addr_t)1 << 48));
    
    return (addr_t)logical_addr >> 12 & 0x1ff;
}

static boolean get_physical_addr(__IN void *logical_addr,
                                 __OUT void **physical_addr)
{
    uint16 pml4_index;
    uint16 pdpte_index;
    uint16 pte_index;
    uint16 pde_index;
    page_t *pml4_addr;
    page_t *pdpte_addr;
    page_t *pte_addr;
    page_t *pde_addr;
    page_t page;

    ASSERT((addr_t)logical_addr < ((addr_t)1 << 48));
    ASSERT(((addr_t)pml4_addr & (addr_t)(PAGESIZE-1)) == 0);
    ASSERT(*physical_addr != (addr_t)NULL);

    pml4_index = get_pml4_index(logical_addr);
    pdpte_index = get_pdpte_index(logical_addr);
    pte_index = get_pte_index(logical_addr);
    pde_index = get_pde_index(logical_addr);

    ASSERT(pml4_index < 512);
    ASSERT(pdpte_index < 512);
    ASSERT(pte_index < 512);
    ASSERT(pde_index < 512);

    pml4_addr = get_pml4();
    if (is_aligned(pml4_addr, PAGESIZE) == FALSE)
        return FALSE;
    page = *(pml4_addr + pml4_index);
    if ((page & PAGE_P_BIT) == 0)
        return FALSE;

    pdpte_addr = (page_t *)(page & ~(PAGESIZE - 1));
    page = *(pdpte_addr + pdpte_index);
    if ((page & PAGE_P_BIT) == 0)
        return FALSE;

    pde_addr = (page_t *)(page & ~(PAGESIZE - 1));
    page = *(pde_addr + pde_index);
    if ((page & PAGE_P_BIT) == 0)
        return FALSE;

    pte_addr = (page_t *)(page & ~(PAGESIZE - 1));
    page = *(pte_addr + pte_index);
    if ((page & PAGE_P_BIT) == 0)
        return FALSE;

    *physical_addr = (void *)((page & ~(PAGESIZE - 1)) + ((addr_t)logical_addr & (PAGESIZE - 1)));

    return TRUE;
}

static boolean get_free_physical_memory_block(
    __OUT struct physical_memory_block **block)
{
    ASSERT(block != NULL);
    
    if (start_entry == NULL)
        return FALSE;

    *block = start_entry;
    start_entry = start_entry->next;

    return TRUE;
}

static boolean add_free_physical_memory_block(
    __IN struct physical_memory_block *block)
{
    ASSERT(block != NULL);

    block->next = start_entry;
    start_entry = block;
}

static void _init_first_free_physical_memory_block(void)
{
    void *cur = (void *)INIT_FREE_PHY_MEM_START;
    struct physical_memory_block *block;

    while (cur < (void *)INIT_FREE_PHY_MEM_END) {
        block = (struct physical_memory_block *)cur;
        block->addr = cur;
        if (add_free_physical_memory_block(block) == FALSE)
            abort();
        cur += PAGESIZE;
    }
}

static void init_physical_memory_block(__IN addr_t physical_memory_size)
{
    uint16 pml4_index = 0;
    uint16 pdpte_index = 0;
    uint16 pde_index = 0; 
    uint16 pte_index = 0;
    page_t *pml4_addr;
    page_t *pdpte_addr;
    page_t *pde_addr;
    page_t *pte_addr;
    page_t page;
    struct physical_memory_block *free_memory_block;
    page_t *current;

    ASSERT(physical_memory_size > UP_TO_DIRECT_MAPPED_PAGE);

	_init_first_free_physical_memory_block();
	current = (page_t *)INIT_FREE_PHY_MEM_END + 1;
        
    pml4_addr = get_pml4();
    while (current < (page_t *)physical_memory_size) {
        pml4_index = get_pml4_index(current);
        pdpte_index = get_pdpte_index(current);
        pde_index = get_pde_index(current);
        pte_index = get_pte_index(current);

        /* check existing pml4 entry */
        page = *(pml4_addr + pml4_index);
        /* make new entry if not exist */
        if ((page & PAGE_P_BIT) == 0) {
            if (get_free_physical_memory_block(&free_memory_block) == FALSE)
                abort();
            *(pml4_addr + pml4_index) = 
                (page_t)free_memory_block->addr | PAGE_P_BIT |
                PAGE_RW_BIT | PAGE_US_BIT;
            page = (page_t)free_memory_block->addr;
            memset((void *)page, 0, PAGESIZE);
        }

        pdpte_addr = (page_t *)(page & ~(PAGESIZE - 1));
        page = *(pdpte_addr + pdpte_index);
        /* make new entry if not exist */
        if ((page & PAGE_P_BIT) == 0) {
            if (get_free_physical_memory_block(&free_memory_block) == FALSE)
                abort();
            *(pdpte_addr + pdpte_index) =
                (page_t)free_memory_block->addr | PAGE_P_BIT |
                PAGE_RW_BIT | PAGE_US_BIT;
            page = (page_t)free_memory_block->addr;
            memset((void *)page, 0, PAGESIZE);
        }

        pde_addr = (page_t *)(page & ~(PAGESIZE - 1));
        page = *(pde_addr + pde_index);
        /* make new entry if not exist */
        if ((page & PAGE_P_BIT) == 0) {
            if (get_free_physical_memory_block(&free_memory_block) == FALSE)
                abort();
            *(pde_addr + pde_index) =
                (page_t)free_memory_block->addr | PAGE_P_BIT |
                PAGE_RW_BIT | PAGE_US_BIT;
            page = (page_t)free_memory_block->addr;
            memset((void *)page, 0, PAGESIZE);
        }

        pte_addr = (page_t *)(page & ~(PAGESIZE - 1));
        *(pte_addr + pte_index) = 
            (page_t)current | PAGE_P_BIT | PAGE_RW_BIT | PAGE_US_BIT;

        /* add new free memory block */
        /* if (current > INIT_FREE_PHY_MEM_END) { */
        /*     ((struct physical_memory_block *)current)->addr = current; */
        /*     add_free_physical_memory_block((struct physical_memory_block *)current); */
        /* } */
        
        current += PAGESIZE;
    }
}

void init_mm(void)
{
	init_physical_memory_block(64 * 1024 * 1024);
}
