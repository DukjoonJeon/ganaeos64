#ifndef __MM_H__
#define __MM_H__

#include <types.h>

#define UP_TO_DIRECT_MAPPED_PAGE (2 * 1024 * 1024)
#define MAX_LOGICAL_MEMORY_SIZE (1 << (9 + 9 + 9 + 9 + 12))

extern void init_mm(void);

#define PAGE_ATTR 0x0Fff
#define PAGE_P_BIT (1 << 0)
#define PAGE_RW_BIT (1 << 1)
#define PAGE_US_BIT (1 << 2)

typedef uint64 addr_t;
typedef uint64 page_t;

extern void *alloc_heap(uint32 size);
extern void free_heap(void *block);

#define BUDDY_MAX_DEPTH 22 /* 2mb */
#define BUDDY_MIN_SIZE_SHIFT 5 /* 32byte */
#define BUDDY_AVAILABLE_DEPTH (BUDDY_MAX_DEPTH - BUDDY_MIN_SIZE_SHIFT)
#define BUDDY_ENTRY_ROOT_SIZE (1 << (BUDDY_MAX_DEPTH - 1))

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

#endif
