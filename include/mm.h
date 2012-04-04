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

/* extern void *alloc_heap(uint32 size); */

#endif
