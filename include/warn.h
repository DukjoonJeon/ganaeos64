#ifndef __WARN_H__
#define __WARN_H__

#include <console.h>

extern void abort(void);

#define ASSERT(_expr) do {                      \
        if (!(_expr)) {                         \
            printk(# _expr);                    \
            abort();                            \
        }                                       \
    } while (0)

#endif /* #ifndef __WARN_H__ */
