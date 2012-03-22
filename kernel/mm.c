#include <mm.h>
#include <types.h>
#include <warn.h>
#include <libc.h>

struct physical_memory_block {
    void *addr;
    struct physical_memory_block *next;
};

#define PAGE_SHIFT 12
#define PAGESIZE 4096

#define PAGE_ENTRYSIZE 8
#define PAGE_ENTRYSHIFT 9
#define PAGE_ENTRYCNT (PAGESIZE / PAGE_ENTRYSIZE)

#define INIT_FREE_PHY_MEM_START (2 * 1024 * 1024)
#define INIT_FREE_PHY_MEM_END (8 * 1024 * 1024 - 1)

#define BUDDY_BLOCK_SIZE (2 * 1024 * 1024)

struct physical_memory_block *start_entry;

struct buddy_block_root *buddy_block_root_list;
struct buddy_block_root *buddy_block_next_available;
struct used_logical_addr *prepair_out_of_memory_for_request_logical_addr;

struct used_logical_addr {
    void *start_addr;
    uint64 size;
    struct used_logical_addr *next;
};

struct used_logical_addr used_logical_addr_list;

boolean is_power_of_2(uint64 num)
{
	if (num && !(num & (num - 1))) {
		return TRUE;
	}
    return FALSE;
}

void init_used_logical_addr_list(uint64 physical_memory_size)
{
    used_logical_addr_list.start_addr = (void *)0;
    used_logical_addr_list.size = physical_memory_size;
    used_logical_addr_list.next = NULL;
}

extern void *alloc_heap(uint32 size);
extern void free_heap(void *block);


void *request_logical_addr(uint64 size)
{
    struct used_logical_addr *cur = &used_logical_addr_list;
    struct used_logical_addr *new_addr;
    void *ret = NULL;

    while (cur->next != NULL) {
        if (cur->start_addr + cur->size + size < cur->next->start_addr) {
            /* new_addr = (struct used_logical_addr *)alloc_heap( */
            /*     sizeof (struct used_logical_addr)); */
            new_addr = prepair_out_of_memory_for_request_logical_addr;
            prepair_out_of_memory_for_request_logical_addr = NULL;
            if (new_addr == NULL)
                goto end;

            new_addr->start_addr = cur->start_addr + cur->size;
            new_addr->size = size;
            new_addr->next = cur->next;
            cur->next = new_addr;
            ret = new_addr->start_addr;
            goto end;
        }
        cur = cur->next;
    }

    /* new_addr = (struct used_logical_addr *)alloc_heap( */
    /*     sizeof (struct used_logical_addr)); */
    new_addr = prepair_out_of_memory_for_request_logical_addr;
    prepair_out_of_memory_for_request_logical_addr = NULL;
    if (new_addr == NULL)
        goto end;
    
    new_addr->start_addr = cur->start_addr + cur->size;
    new_addr->size = size;
    new_addr->next = cur->next;
    cur->next = new_addr;
    ret = new_addr->start_addr;
    
end:
    return ret;
}

boolean release_logical_addr(void *addr)
{
    struct used_logical_addr *cur = &used_logical_addr_list;

    /* make a error if addr is 0 */
    if (addr == (void *)0)
        return FALSE;

    while (cur->next != NULL) {
        if (cur->next->start_addr == addr) {
            struct used_logical_addr *del_addr = cur->next;
            cur->next = cur->next->next;
            free_heap(del_addr);
            return TRUE;
        }
    }

    return FALSE;
}

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
    __OUT void **phys_addr)
{
    struct physical_memory_block *block;

    ASSERT(block != NULL);
    
    if (start_entry == NULL)
        return FALSE;

    block = start_entry;
    start_entry = start_entry->next;
    *phys_addr = block->addr;
    free_heap(block);

    return TRUE;
}

static boolean add_free_physical_memory_block(
    __IN void *physical_addr)
{
    struct physical_memory_block *block;

    ASSERT(physical_addr != NULL);

    block = (struct physical_memory_block *)alloc_heap(
        sizeof (struct physical_memory_block));
    if (block == NULL)
        return FALSE;

    block->addr = physical_addr;
    block->next = start_entry;
    start_entry = block;

    return TRUE;
}

static boolean get_available_physical_memory_block(__IN uint64 size)
{
    uint64 count;
    struct physical_memory_block *cur;

    ASSERT(size > 0);
    ASSERT((size % PAGESIZE) == 0);

    cur = start_entry;
    count = size / PAGESIZE;
    
    while (cur != NULL && count) {
        count--;
        cur = cur->next;
    }

    return count == 0? TRUE: FALSE;
}

static void init_first_free_physical_memory_block(uint64 physical_memory_size)
{
    void *cur = (void *)(INIT_FREE_PHY_MEM_START + BUDDY_BLOCK_SIZE);
    struct physical_memory_block *block;
    uint16 pml4_index = 0;
    uint16 pdpte_index = 0;
    uint16 pde_index = 0; 
    uint16 pte_index = 0;
    page_t *pml4_addr;
    page_t *pdpte_addr;
    page_t *pde_addr;
    page_t *pte_addr;
    page_t page;
    void *new_physical_addr;
    void *physical_addr;
    void *physical_logical;

    /* premapped memory */
    while (cur < (void *)INIT_FREE_PHY_MEM_END) {
        if (add_free_physical_memory_block(cur) == FALSE)
            abort();
        cur += PAGESIZE;
    }

    pml4_addr = get_pml4();
    while (cur < (void *)physical_memory_size) {
        pml4_index = get_pml4_index(cur);
        pdpte_index = get_pdpte_index(cur);
        pde_index = get_pde_index(cur);
        pte_index = get_pte_index(cur);

        /* check existing pml4 entry */
        page = *(pml4_addr + pml4_index);
        /* make new entry if not exist */
        if ((page & PAGE_P_BIT) == 0) {
            if (get_free_physical_memory_block(&physical_addr) == FALSE)
                abort();
            *(pml4_addr + pml4_index) = 
                (page_t)physical_addr | PAGE_P_BIT |
                PAGE_RW_BIT | PAGE_US_BIT;
            page = (page_t)physical_addr;
            memset((void *)page, 0, PAGESIZE);
        }

        pdpte_addr = (page_t *)(page & ~(PAGESIZE - 1));
        page = *(pdpte_addr + pdpte_index);
        /* make new entry if not exist */
        if ((page & PAGE_P_BIT) == 0) {
            if (get_free_physical_memory_block(&physical_addr) == FALSE)
                abort();
            *(pdpte_addr + pdpte_index) =
                (page_t)physical_addr | PAGE_P_BIT |
                PAGE_RW_BIT | PAGE_US_BIT;
            page = (page_t)physical_addr;
            memset((void *)page, 0, PAGESIZE);
        }

        pde_addr = (page_t *)(page & ~(PAGESIZE - 1));
        page = *(pde_addr + pde_index);
        /* make new entry if not exist */
        if ((page & PAGE_P_BIT) == 0) {
            if (get_free_physical_memory_block(&physical_addr) == FALSE)
                abort();
            *(pde_addr + pde_index) =
                (page_t)physical_addr | PAGE_P_BIT |
                PAGE_RW_BIT | PAGE_US_BIT;
            page = (page_t)physical_addr;
            memset((void *)page, 0, PAGESIZE);
        }

        pte_addr = (page_t *)(page & ~(PAGESIZE - 1));
        *(pte_addr + pte_index) = 
            (page_t)cur | PAGE_P_BIT | PAGE_RW_BIT | PAGE_US_BIT;

        if (add_free_physical_memory_block(cur) == FALSE)
            abort();

        cur = ((uint8 *)cur) + PAGESIZE;
    }
}

boolean init_heap(__IN struct buddy_block_root *buddy_block_root,
                  __IN void *heap_addr)
{
	uint16 loopi;
	struct buddy_block_root *cur_block_root;

	buddy_block_root->size_head[0] = 
        (struct buddy_entry *)heap_addr;
	if (buddy_block_root->size_head[0] == NULL) {
		/* printf("%s(%d): malloc fail\n", __func__, __LINE__); */
		return FALSE;
	}

    buddy_block_root->next = NULL;
	buddy_block_root->used_size = 0;
	buddy_block_root->free_size = BUDDY_ENTRY_ROOT_SIZE;

	/* printf("%s(%d): size_head[0]'s addr is %x\n",  */
    /*        __func__, __LINE__, buddy_block_root->size_head); */

	/* printf("%s(%d): free_size is %d\n",  */
    /*        __func__, __LINE__, buddy_block_root->free_size); */
	
	buddy_block_root->size_head[0]->next = NULL;
	buddy_block_root->size_head[0]->size = 1 << (BUDDY_MAX_DEPTH - 1);

	for (loopi = 1; loopi < BUDDY_AVAILABLE_DEPTH; loopi++)
		buddy_block_root->size_head[loopi] = NULL;

	if (buddy_block_root_list == NULL) {
		/* printf("%s(%d): first adding buddy block root\n", __func__, __LINE__); */
		buddy_block_root_list = buddy_block_root;
	} else {
		cur_block_root = buddy_block_root_list;
		while (cur_block_root->next != NULL)
			cur_block_root = cur_block_root->next;
		cur_block_root->next = buddy_block_root;
	}

	/* preallocated memory for heap incresing */
	buddy_block_next_available = 
		(struct buddy_block_root *)alloc_heap(sizeof (struct buddy_block_root));
	if (buddy_block_next_available == NULL)
		return FALSE;
			
	return TRUE;
}

uint32 round_up_to_next_power_of_2(__IN uint32 size)
{
	/* round up the next highest power of 2 */
    /* http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2 */
	size--;
	size |= size >> 1;
	size |= size >> 2;
	size |= size >> 4;
	size |= size >> 8;
	size |= size >> 16;
	return size + 1;
}

boolean get_depth(__IN uint32 size, __OUT uint16 *depth)
{
	int16 count_depth = 0;

	if ((size && !(size & (size - 1))) == 0) {
		/* fprintf(stderr, "it is not power of 2\n"); */
		return FALSE;
	}

	count_depth = !!(size & 0xffffffff);
	count_depth += !!(size & 0xaaaaaaaa);
	count_depth += !!(size & 0xcccccccc) << 1;
	count_depth += !!(size & 0xf0f0f0f0) << 2;
	count_depth += !!(size & 0xff00ff00) << 3;
	count_depth += !!(size & 0xffff0000) << 4;
	*depth = BUDDY_MAX_DEPTH - count_depth;
	
	return TRUE;
}

boolean _split_buddy_block(__IN struct buddy_block_root *block_root,
                           __IN uint16 depth)
{
	struct buddy_entry *first_buddy_entry;
	struct buddy_entry *second_buddy_entry;

    /* TODO assert */ 

	/* printf("%s(%d): start depth %d\n", __func__, __LINE__, depth); */

	first_buddy_entry = block_root->size_head[depth];
	if (first_buddy_entry == NULL)
		return FALSE;

	/* printf("%s(%d): remove depth block %d\n", __func__, __LINE__, depth); */
    block_root->size_head[depth] = block_root->size_head[depth]->next;

	first_buddy_entry->size = first_buddy_entry->size / 2;
	second_buddy_entry = 
		(struct buddy_entry *)(
			(uint8 *)first_buddy_entry + first_buddy_entry->size);
	first_buddy_entry->next = second_buddy_entry;
	second_buddy_entry->size = first_buddy_entry->size;
	second_buddy_entry->next = block_root->size_head[depth + 1];
	block_root->size_head[depth + 1] = first_buddy_entry;

	/* printf("%s(%d): first_buddy_entry %x, second_buddy_entry %x, size %d\n", */
	/* 	   __func__, __LINE__, first_buddy_entry, second_buddy_entry, */
	/* 	   first_buddy_entry->size); */

	return TRUE;
}

void *_alloc_heap_on_buddy_block_root(struct buddy_block_root *block_root,
                                     uint16 depth)
{
    void *ret;
    uint16 cur_depth;

    /* ASSERT(size && !(size & (size - 1))); */
    ASSERT(block_root != NULL);

	if (block_root->size_head[depth] != NULL) {
		ret = block_root->size_head[depth];
		block_root->size_head[depth] = block_root->size_head[depth]->next;

		/* printf("%s(%d): matched size is %d, addr = %x\n", */
        /*        __func__, __LINE__, depth, ret); */
		
        return ret;
	}

	/* printf("%s(%d): matched size is not exist %d\n", __func__, __LINE__, depth); */
	
	/* retry with split */
	cur_depth = depth - 1;
	/* find available larger block */
	while (block_root->size_head[cur_depth] == NULL && cur_depth > 0)
		cur_depth--;

	/* printf("%s(%d): split from %d to %d\n", */
	/* 	   __func__, __LINE__, cur_depth, depth); */

	if (block_root->size_head[cur_depth] == NULL)
		return NULL;

	for (; cur_depth < depth; cur_depth++) {
		if (_split_buddy_block(block_root, cur_depth) == FALSE)
			return NULL;
	} 

	ret = block_root->size_head[depth];
	block_root->size_head[depth] = block_root->size_head[depth]->next;
	/* printf("%s(%d): matched size is %d, addr = %x\n", __func__, __LINE__, depth, ret);_ */

    return ret;
}

void *_alloc_heap(uint32 adj_size)
{
    struct buddy_block_root *cur = buddy_block_root_list;
    void *ret = NULL;
	boolean success;
	uint16 depth;
	uint16 cur_depth;

	/* printf("%s(%d): start\n",__func__, __LINE__); */

	success = get_depth(adj_size, &depth);
	if (success == FALSE)
		return NULL;

	/* printf("%s(%d): depth is %d\n",__func__, __LINE__, depth); */

	if (depth >= BUDDY_AVAILABLE_DEPTH) {
		/* printf("%s(%d): depth is too deep : %d, force setted %d\n", */
		/* 	   __func__, __LINE__, */
		/* 	   depth, BUDDY_MAX_DEPTH - 1); */
		depth = BUDDY_AVAILABLE_DEPTH - 1;
	}

    while (cur != NULL) {
		if (cur->free_size >= adj_size) {
			ret = _alloc_heap_on_buddy_block_root(cur, depth);
			if (ret != NULL) {
				cur->free_size -= adj_size;
				cur->used_size += adj_size;
				((struct heap_block_chunk *)ret)->root = cur;
				((struct heap_block_chunk *)ret)->depth = depth;
				((struct heap_block_chunk *)ret)->size = adj_size;
				break;
			}
		}
        cur = cur->next;
    }
    
    return ret;
}

boolean fill_physical_page_in_logical_addr(void *logical_addr, uint64 size)
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
    void *new_physical_addr;
    void *physical_addr;
    void *physical_logical;
    void *target_addr = logical_addr + size;

    /* ASSERT(physical_memory_size > UP_TO_DIRECT_MAPPED_PAGE); */

    if (get_available_physical_memory_block(BUDDY_BLOCK_SIZE) == FALSE)
        return FALSE;

    pml4_addr = get_pml4();
    while (logical_addr < target_addr) {
        /* it will be success */
        get_free_physical_memory_block(&new_physical_addr);

        pml4_index = get_pml4_index(logical_addr);
        pdpte_index = get_pdpte_index(logical_addr);
        pde_index = get_pde_index(logical_addr);
        pte_index = get_pte_index(logical_addr);

        /* check existing pml4 entry */
        page = *(pml4_addr + pml4_index);
        /* make new entry if not exist */
        if ((page & PAGE_P_BIT) == 0) {
            if (get_free_physical_memory_block(&physical_addr) == FALSE)
                abort();
            *(pml4_addr + pml4_index) = 
                (page_t)physical_addr | PAGE_P_BIT |
                PAGE_RW_BIT | PAGE_US_BIT;
            page = (page_t)physical_addr;
            memset((void *)page, 0, PAGESIZE);
        }

        pdpte_addr = (page_t *)(page & ~(PAGESIZE - 1));
        page = *(pdpte_addr + pdpte_index);
        /* make new entry if not exist */
        if ((page & PAGE_P_BIT) == 0) {
            if (get_free_physical_memory_block(&physical_addr) == FALSE)
                abort();
            *(pdpte_addr + pdpte_index) =
                (page_t)physical_addr | PAGE_P_BIT |
                PAGE_RW_BIT | PAGE_US_BIT;
            page = (page_t)physical_addr;
            memset((void *)page, 0, PAGESIZE);
        }

        pde_addr = (page_t *)(page & ~(PAGESIZE - 1));
        page = *(pde_addr + pde_index);
        /* make new entry if not exist */
        if ((page & PAGE_P_BIT) == 0) {
            if (get_free_physical_memory_block(&physical_addr) == FALSE)
                abort();
            *(pde_addr + pde_index) =
                (page_t)physical_addr | PAGE_P_BIT |
                PAGE_RW_BIT | PAGE_US_BIT;
            page = (page_t)physical_addr;
            memset((void *)page, 0, PAGESIZE);
        }

        pte_addr = (page_t *)(page & ~(PAGESIZE - 1));
        *(pte_addr + pte_index) = 
            (page_t)new_physical_addr | PAGE_P_BIT | PAGE_RW_BIT | PAGE_US_BIT;

        logical_addr = ((uint8 *)logical_addr) + PAGESIZE;
    }

    return TRUE;
}

boolean add_new_buddy_block_root(void)
{
    void *logical_addr;
    boolean ret;

    logical_addr = request_logical_addr(BUDDY_BLOCK_SIZE);
    if (logical_addr == NULL)
        return FALSE;

    ret = fill_physical_page_in_logical_addr(logical_addr, BUDDY_BLOCK_SIZE);
    if (ret == FALSE)
        return FALSE;
    
	if (init_heap(buddy_block_next_available, logical_addr) == FALSE)
		return FALSE;

    if (prepair_out_of_memory_for_request_logical_addr == NULL)
        prepair_out_of_memory_for_request_logical_addr =
            (struct used_logical_addr *)alloc_heap(
                sizeof (struct used_logical_addr));
}

void *alloc_heap(uint32 size)
{

	uint32 adj_size;

	boolean success;
	void *ret;

	/* printf("%s(%d): alloc size %d\n", __func__, __LINE__, size); */

	if (size > BUDDY_ENTRY_ROOT_SIZE) {
		/* fprintf(stderr, "size is too large\n"); */
		return NULL;
	}

	adj_size = size + sizeof (struct heap_block_chunk);
	adj_size = round_up_to_next_power_of_2(adj_size);

	/* printf("%s(%d): adjust size %d\n", __func__, __LINE__, adj_size); */

    ret = _alloc_heap(adj_size);
    if (ret != NULL)
        goto end;

	/* printf("%s(%d): add a new buddy block root and retry allocation\n", */
	/* 	   __func__, __LINE__); */

    /* TODO: merge splited block and retry allocation */
	

	/* add a new buddy block root and retry allocation */
	if (add_new_buddy_block_root() == FALSE)
		return NULL;
    ret = _alloc_heap(adj_size);
    if (ret != NULL)
        goto end;

	/* printf("%s(%d): ret %x\n", __func__, __LINE__, ret); */
end:
	return CHUNK_TO_BLOCK(ret);;
}

void free_heap(void *block)
{
	struct heap_block_chunk *chunk;
	struct buddy_block_root *root;
	struct buddy_entry *entry;
	struct buddy_entry *cur_entry;
	uint16 depth;
	uint32 size;

	chunk = BLOCK_TO_CHUNK(block);
	root = chunk->root;
	depth = chunk->depth;
	size = chunk->size;
	
	entry = (struct buddy_entry *)chunk;
	entry->size = size;

	if (root->size_head[depth] == NULL) {
		root->size_head[depth] = entry;
		entry->next = NULL;

		return;
	}

	cur_entry = root->size_head[depth]; 
	while (cur_entry->next != NULL && cur_entry->next < entry)
		cur_entry = cur_entry->next;
	entry->next = cur_entry->next;
	cur_entry->next = entry;
}

boolean init_alloc(void)
{
	static struct buddy_block_root buddy_block_root_first;

	return init_heap(&buddy_block_root_first, (void *)INIT_FREE_PHY_MEM_START);
}

uint64 get_physical_mem_size(void)
{
	return 64 * 1024 * 1024;
}

void init_global_vars(void)
{
    start_entry = NULL;
    buddy_block_root_list = NULL;
}

void prealloced_for_out_of_memory()
{
    prepair_out_of_memory_for_request_logical_addr = 
        (struct used_logical_addr *)alloc_heap(
            sizeof (struct used_logical_addr));

}

void init_mm(void)
{
	uint64 phys_mem_size;

    init_global_vars();
	phys_mem_size = get_physical_mem_size();
    init_used_logical_addr_list(phys_mem_size);
	init_alloc();
    prealloced_for_out_of_memory();
	init_first_free_physical_memory_block(phys_mem_size);
}
