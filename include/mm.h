#ifndef __MM_H__
#define __MM_H__

#include <types.h>

#define UP_TO_DIRECT_MAPPED_PAGE (2 * 1024 * 1024)

extern void init_mm(void);

#define PAGE_ATTR 0x0Fff
#define PAGE_P_BIT (1 << 0)

typedef uint64 addr_t;
typedef uint64 page_t;

#endif
