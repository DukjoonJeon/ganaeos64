#include <mm.h>

struct physical_memory_block {
    void *addr;
    struct physical_memory_block *next;
};

#define PAGESIZE 4096

#define INIT_FREE_PHY_MEM_START (2 * 1024 * 1024)
#define INIT_FREE_PHY_MEM_END (4 * 1024 * 1024 - 1)


struct physical_page *start_entry = NULL;

typedef uint64 addr_t;
typedef uint64 page_t;

static page_t *get_pml4(void)
{
    addr_t  pml4_addr;
    __asm__ __volatile__ (
        "movl %%cr3, %%eax\n\t"
        : "=a" (pml4_addr)
        : /* no input */
        :
    );

    return (page_t *)pml4_addr;
}

static inline boolean get_child_page_by_index(__IN page_t *parent_addr,
                                              __IN uint16 index,
                                              __OUT page_t **child_addr)
{
    addr_t result;

    ASSERT((index >= 0) && (index < 512));
    ASSERT((parent_addr != NULL) && (parent_addr & (PAGESIZE-1) == 0));
    ASSERT(*child_addr != NULL);

    result = parent[index];
    *child = (page_t *)(result & ~PAGE_ATTR);

    if ((result & PAGE_P_BIT) == 0)
        return FALSE;
    return TRUE;
}

static inline boolean get_pdpte(__IN page_t *pml4, __IN addr_t logical_addr,
                                __OUT page_t **pdpte)
{
    uint16 index;

    ASSERT(logical_addr < (1 << 48));
    ASSERT((pml4_addr != NULL) && (pml4_addr & (PAGESIZE-1) == 0));
    ASSERT(*pdpte != NULL);

    index = logical_addr >> (12 + 9 + 9 + 9);

    return get_child_page_by_index(pml4_addr, index, pdpte_addr);
}

static inline boolean get_pde(__IN page_t *pdpte_addr, __IN addr_t logical_addr,
                              __OUT page_t **pde_addr)
{
    uint16 index;

    ASSERT(logical_addr < (1 << 48));
    ASSERT((pdpte_addr != NULL) && (pdpte_addr & (PAGESIZE-1) == 0));
    ASSERT(*pde != NULL);

    index = (logical_addr >> (12 + 9 + 9)) & 0x01ff;

    return get_child_page_by_index(pdpte_addr, index, pde_addr);
}

static inline boolean get_pte(__IN page_t *pde_addr, __IN addr_t logical_addr,
                              __OUT page_t **pte_addr)
{
    uint16 index;

    ASSERT(logical_addr < (1 << 48));
    ASSERT((pde_addr != NULL) && (pde_addr & (PAGESIZE-1) == 0));
    ASSERT(*pte != NULL);

    index = (logical_addr >> (12 + 9)) & 0x01ff;

    return get_child_page_by_index(pde_addr, index, pte_addr);
}

static inline boolean get_physical_addr(__IN page_t *pte_addr,
                                        __IN addr_t logical_addr,
                                        __OUT page_t **addr)
{
    uint16 index;

    ASSERT(logical_addr < (1 << 48));
    ASSERT((pte_addr != NULL) && (pte_addr & (PAGESIZE-1) == 0));
    ASSERT(*addr != NULL);

    index = (logical_addr >> (12)) & 0x01ff;

    return get_child_page_by_index(pte_addr, index, addr);
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
    ASSERT(item != NULL);

    block->next = start_entry;
    start_entry = block;
}

static void _init_first_free_physical_memory_block(void)
{
    addr_t cur = INIT_FREE_PHY_MEM_START;
    struct physical_page *block;

    while (cur < INIT_FREE_PHY_MEM_END) {
        block = (struct physical_page *)cur;
        block->addr = cur;
        if (add_free_physical_memory_block(block) == FALSE)
            abort();
        cur += PAGESIZE;
    }
}

static void init_physical_memory_block(__IN addr_t physical_memory_size)
{
    uint16 pml4_index;
    uint16 pdpte_index;
    uint16 pde_index;
    uint16 pte_index;
    
    page_t *pml4;
    page_t *pdpte;
    page_t *pde;
    page_t *pte;

    addr current;

    ASSERT(physical_memory_size > UP_TO_DIRECT_MAPPED_PAGE);

    current = INIT_FREE_PHY_MEM


    pml4 = get_pml4();
    while (current < physical_memory_size) {
        if (get_pdpte(pml4, current, &pdpte) == FALSE) {
            
        }

        if (get_pde(pdpte, current, &pde) == FALSE)
            abort();

    if (get_pde(pde, current, &pte) == FALSE)
        abort();

    }
    
    

    /* linear mapping */
    

 
}

void mm_init(void)
{
    


}
