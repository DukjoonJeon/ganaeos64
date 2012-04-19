#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

/* TODO: remove blank buddy block root */

typedef unsigned char uint8;
typedef char int8;
typedef unsigned short uint16;
typedef short int16;
typedef unsigned long int uint32;
typedef long int int32;
typedef unsigned long long int uint64;
typedef long long int int64;
typedef int boolean;

#define TRUE 1
#define FALSE 0

#define BUDDY_MAX_DEPTH 22 /* 2mb */
#define BUDDY_MIN_SIZE_SHIFT 5 /* 32byte */
#define BUDDY_AVAILABLE_DEPTH (BUDDY_MAX_DEPTH - BUDDY_MIN_SIZE_SHIFT)
#define BUDDY_ENTRY_ROOT_SIZE (1 << (BUDDY_MAX_DEPTH - 1))

#define ASSERT(__ARG) while (0)

#define BLOCK_TO_CHUNK(__block) (((struct heap_block_chunk *)(__block)) - 1)
#define CHUNK_TO_BLOCK(__block) (((struct heap_block_chunk *)(__block)) + 1)

struct buddy_entry {
	struct buddy_entry *next;
	uint32 size;
};

struct heap_block_chunk {
    struct buddy_block_root *root;
	uint16 depth;
	uint32 size;
};

struct buddy_block_root {
    struct buddy_entry *size_head[BUDDY_AVAILABLE_DEPTH];
	uint32 used_size;
	uint32 free_size;
    struct buddy_block_root *next;
};

struct buddy_block_root *buddy_block_root_list = NULL;
struct buddy_block_root *buddy_block_next_available;

extern void *alloc_heap(uint32 size);

boolean init_heap(struct buddy_block_root *buddy_block_root)
{
	uint16 loopi;
	struct buddy_block_root *cur_block_root;

	buddy_block_root->size_head[0] = 
        (struct buddy_entry *)malloc(BUDDY_ENTRY_ROOT_SIZE);
	if (buddy_block_root->size_head[0] == NULL) {
		printf("%s(%d): malloc fail\n", __func__, __LINE__);
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

uint32 round_up_to_next_power_of_2(uint32 size)
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

boolean get_depth(uint32 size, uint16 *depth)
{
	int count_depth = 0;

	if ((size && !(size & (size - 1))) == 0) {
		fprintf(stderr, "it is not power of 2\n");
		return FALSE;
	}

	count_depth = !!(size & 0xffffffff);
	count_depth += !!(size & 0xaaaaaaaa);
	count_depth += (!!(size & 0xcccccccc) << 1);
	count_depth += (!!(size & 0xf0f0f0f0) << 2);
	count_depth += (!!(size & 0xff00ff00) << 3);
	count_depth += (!!(size & 0xffff0000) << 4);
	*depth = BUDDY_MAX_DEPTH - count_depth;
	
	return TRUE;
}

boolean _split_buddy_block(struct buddy_block_root *block_root, uint16 depth)
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
		printf("%s(%d): depth is too deep : %d, force setted %d\n",
			   __func__, __LINE__,
			   depth, BUDDY_MAX_DEPTH - 1);
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

boolean add_new_buddy_block_root(void)
{
	if (init_heap(buddy_block_next_available) == FALSE)
		return FALSE;
}

void *alloc_heap(uint32 size)
{
	uint32 adj_size;

	boolean success;
	void *ret;

	printf("%s(%d): alloc size %d\n", __func__, __LINE__, size);

	if (size > BUDDY_ENTRY_ROOT_SIZE) {
		fprintf(stderr, "size is too large\n");
		return NULL;
	}

	adj_size = size + sizeof (struct heap_block_chunk);
	adj_size = round_up_to_next_power_of_2(adj_size);

	printf("%s(%d): adjust size %d\n", __func__, __LINE__, adj_size);

    ret = _alloc_heap(adj_size);
    if (ret != NULL)
        goto end;

	printf("%s(%d): add a new buddy block root and retry allocation\n",
		   __func__, __LINE__);

    /* TODO: merge splited block and retry allocation */
	

	/* add a new buddy block root and retry allocation */
	if (add_new_buddy_block_root() == FALSE)
		return NULL;
    ret = _alloc_heap(adj_size);
    if (ret != NULL)
        goto end;

	printf("%s(%d): ret %x\n", __func__, __LINE__, ret);
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

	return init_heap(&buddy_block_root_first);
}

int main(void)
{
	int i;
	uint16 d;
	char *aa;
	char *bb;
	char *cc;

	if (init_alloc() == FALSE) {
		fprintf(stderr, "init_heap is error\n");
		return 1;
	}

	/* printf("----------------------------depth test start\n"); */
	/* for (i = 0; i < 32; i++) { */
	/* 	if (get_depth(1 << i, &d) == FALSE) */
	/* 		printf("error\n"); */
	/* 	printf("   %d:%lu's depth is %d\n", i, 1 << i, d); */
	/* } */
	/* printf("----------------------------depth test end\n"); */

	/* printf("----------------------------2power test start\n"); */
	/* for (i = 0; i < 32; i++) { */
	/* 	printf("   %d: %lu ?= %lu\n", */
	/* 		   i, round_up_to_next_power_of_2((1 << i) - 1), 1 << i); */
	/* }	 */
	/* printf("----------------------------2power test end\n"); */

    
	/* aa = (char *)alloc_heap(1); */
	/* printf("alloced size is %d\n", BLOCK_TO_CHUNK(aa)->size); */
	/* printf("------------------------------------------\n"); */
	/* aa = (char *)alloc_heap(15); */
	/* printf("alloced size is %d\n", BLOCK_TO_CHUNK(aa)->size); */
	/* printf("------------------------------------------\n"); */
	/* aa = (char *)alloc_heap(31); */
	/* printf("alloced size is %d\n", BLOCK_TO_CHUNK(aa)->size); */
	/* printf("------------------------------------------\n"); */
	/* aa = (char *)alloc_heap(50); */
	/* printf("alloced size is %d\n", BLOCK_TO_CHUNK(aa)->size); */
    /* for (int i = 0; i < 5000; i++) { */
    /*     printf("------------------------------------------\n"); */
    /*     aa = (char *)alloc_heap(500 * 10); */
    /*     if (aa == NULL) { */
    /*         printf("alloc fail %d\n", i); */
    /*         break; */
    /*     } */
    /*     else */
    /*         printf("alloced size is %d\n", BLOCK_TO_CHUNK(aa)->size); */

	/* } */

	printf("------------------------------------------\n");
	aa = (char *)alloc_heap(500 * 10);
	if (aa == NULL) {
		printf("alloc fail %d\n", i);

	}
	else
		printf("alloced size is %d, addr is %x\n", BLOCK_TO_CHUNK(aa)->size, aa);

	printf("------------------------------------------\n");
	bb = (char *)alloc_heap(500 * 10);
	if (bb == NULL) {
		printf("alloc fail %d\n", i);

	}
	else
		printf("alloced size is %d, addr is %x\n", BLOCK_TO_CHUNK(bb)->size, bb);

	printf("------------------------------------------\n");
	cc = (char *)alloc_heap(500 * 10);
	if (cc == NULL) {
		printf("alloc fail %d\n", i);

	}
	else
		printf("alloced size is %d, addr is %x\n", BLOCK_TO_CHUNK(cc)->size, cc);
	
	free_heap(aa);
	free_heap(cc);

	printf("------------------------------------------\n");
	aa = (char *)alloc_heap(500 * 10);
	if (aa == NULL) {
		printf("alloc fail %d\n", i);

	}
	else
		printf("alloced size is %d, addr is %x\n", BLOCK_TO_CHUNK(aa)->size, aa);

	printf("------------------------------------------\n");
	aa = (char *)alloc_heap(500 * 10);
	if (aa == NULL) {
		printf("alloc fail %d\n", i);

	}
	else
		printf("alloced size is %d, addr is %x\n", BLOCK_TO_CHUNK(aa)->size, aa);

    /* for (int i = 0; i < 5000; i++) { */
    /*     printf("------------------------------------------\n"); */
    /*     aa = (char *)alloc_heap(500 * 10); */
    /*     if (aa == NULL) { */
    /*         printf("alloc fail %d\n", i); */
    /*         break; */
    /*     } */
    /*     else */
    /*         printf("alloced size is %d, addr is %x\n", BLOCK_TO_CHUNK(aa)->size, aa); */

	/* 	free_heap(aa); */
	/* } */

	return 0;
}
