#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>

#ifndef NULL
#define NULL 0
#endif

#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

#ifndef MIN
#define MIN(a,b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#endif

#ifndef ROUNDED_DIV
#define ROUNDED_DIV(A,B)  (((A) + (B)/2) / (B))
#endif

#ifndef CEILDIV
#define CEILDIV(A,B) (((A) + (B) - 1) / (B))
#endif

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(COND,MSG) typedef char static_assertion_##MSG[(COND)?1:-1]
#endif

#endif /* __COMMON_H */

