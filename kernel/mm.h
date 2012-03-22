#ifndef __MM_H__
#define __MM_H__

#include <types.h>

#define UP_TO_DIRECT_MAPPED_PAGE (2 * 1024 * 1024)

extern uint64 get_physical_mem_size(void);
extern void init_mm(void);

#endif
