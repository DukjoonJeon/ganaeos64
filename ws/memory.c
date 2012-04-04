#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

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
	uint32 size;
	struct buddy_entry *next;
};

struct heap_block_chunk {
    struct buddy_entry *root;
	uint32 size;
};

struct buddy_block_root {
    struct buddy_entry *size_head[BUDDY_AVAILABLE_DEPTH];
    struct buddy_block_root *next;
};

struct buddy_block_root buddy_block_root_first;
struct buddy_block_root *buddy_block_root_list;

boolean init_heap(void)
{
	uint16 loopi;

    buddy_block_root_first.next = NULL;
	
	buddy_block_root_first.size_head[0] = 
        (struct buddy_entry *)malloc(BUDDY_ENTRY_ROOT_SIZE);
	if (buddy_block_root_first.size_head[0] == NULL)
		return FALSE;

	printf("%s(%d): size_head[0]'s addr is %x\n", 
           __func__, __LINE__, buddy_block_root_first.size_head);

	buddy_block_root_first.size_head[0]->next = NULL;
	buddy_block_root_first.size_head[0]->size = 1 << (BUDDY_MAX_DEPTH - 1);

	for (loopi = 1; loopi < BUDDY_MAX_DEPTH; loopi++)
		buddy_block_root_first.size_head[loopi] = NULL;

    buddy_block_root_list = &buddy_block_root_first;

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

	printf("%s(%d): start depth %d\n", __func__, __LINE__, depth);

	first_buddy_entry = block_root->size_head[depth];
	if (first_buddy_entry == NULL)
		return FALSE;

	/* printf("%s(%d): remove depth block %d\n", __func__, __LINE__, depth); */
    block_root->size_head[depth] = block_root->size_head[depth]->next;

	second_buddy_entry = 
		(struct buddy_entry *)(
			(uint8 *)first_buddy_entry + first_buddy_entry->size);
	first_buddy_entry->next = second_buddy_entry;
	second_buddy_entry->size = 
        first_buddy_entry->size =
        first_buddy_entry->size / 2;
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

		printf("%s(%d): matched size is %d, addr = %x\n",
               __func__, __LINE__, depth, ret);
		
        return ret;
	}

	printf("%s(%d): matched size is not exist %d\n", __func__, __LINE__, depth);
	
	/* retry with split */
	cur_depth = depth - 1;
	/* find available larger block */
	while (block_root->size_head[cur_depth] == NULL && cur_depth > 0)
		cur_depth--;

	printf("%s(%d): split from %d to %d\n", __func__, __LINE__, cur_depth, depth);
	if (block_root->size_head[cur_depth] != NULL) {
		for (; cur_depth < depth; cur_depth++) {
			if (_split_buddy_block(block_root, cur_depth) == FALSE)
				return NULL;
		}
	}

	ret = block_root->size_head[depth];
	block_root->size_head[depth] = block_root->size_head[depth]->next;
	printf("%s(%d): matched size is %d, addr = %x\n", __func__, __LINE__, depth, ret);

    return ret;
}

void *_alloc_heap(uint16 depth)
{
    struct buddy_block_root *cur = buddy_block_root_list;
    void *ret;

    while (cur != NULL) {
        ret = _alloc_heap_on_buddy_block_root(cur, depth);
        if (ret != NULL)
            return ret;
        cur = cur->next;
    }
    
    return NULL;
}

void *alloc_heap(uint32 size)
{
	uint32 adj_size;
	uint16 depth;
	uint16 cur_depth;
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

	success = get_depth(adj_size, &depth);
	if (success == FALSE)
		return NULL;

	if (depth >= BUDDY_AVAILABLE_DEPTH) {
		printf("%s(%d): depth is too deep : %d, force setted %d\n",
			   __func__, __LINE__,
			   depth, BUDDY_MAX_DEPTH - 1);
		depth = BUDDY_AVAILABLE_DEPTH - 1;
	}

    ret = _alloc_heap(depth);
    if (ret != NULL)
        goto end;

end:
	((struct heap_block_chunk *)ret)->size = adj_size;
	return CHUNK_TO_BLOCK(ret);;
}

int main(void)
{
	int i;
	uint16 d;
	char *aa;

	if (init_heap() == FALSE) {
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

    
	aa = (char *)alloc_heap(1);
	printf("alloced size is %d\n", BLOCK_TO_CHUNK(aa)->size);
	printf("------------------------------------------\n");
	aa = (char *)alloc_heap(15);
	printf("alloced size is %d\n", BLOCK_TO_CHUNK(aa)->size);
	printf("------------------------------------------\n");
	aa = (char *)alloc_heap(31);
	printf("alloced size is %d\n", BLOCK_TO_CHUNK(aa)->size);
	printf("------------------------------------------\n");
	aa = (char *)alloc_heap(50);
	printf("alloced size is %d\n", BLOCK_TO_CHUNK(aa)->size);
    for (int i = 0; i < 5000; i++) {
        printf("------------------------------------------\n");
        aa = (char *)alloc_heap(500);
        if (aa == NULL) {
            printf("alloc fail %d\n", i);
            break;
        }
        else
            printf("alloced size is %d\n", BLOCK_TO_CHUNK(aa)->size);

	}
	return 0;
}
