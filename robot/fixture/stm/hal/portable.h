#ifndef PORTABLE_H
#define PORTABLE_H

#if defined(__EDG__)
#define PRE_PACK_DIRECTIVE  __packed
#define POST_PACK_DIRECTIVE
#elif defined(__GNUC__)
#define PRE_PACK_DIRECTIVE
#define POST_PACK_DIRECTIVE __attribute__((packed))
#else
#error UNDEFINED COMPILER!
#endif

#include "stm32f2xx.h"

// Basic abstractions for low level I/O on our processor
#define PIN_SET(gp, index)              (gp)->BSRRL = (1 << (index))
#define PIN_RESET(gp, index)            (gp)->BSRRH = (1 << (index))
#define GPIO_READ(gp)                   (gp)->IDR
#define PIN_READ(gp,index)              (GPIO_READ(gp) & (index))

#define PIN_IN(gp, index)               (gp)->MODER = ((gp)->MODER & ~(GPIO_MODER_MODER0 << ((index) * 2))) | (GPIO_Mode_IN << ((index) * 2))
#define PIN_OUT(gp, index)              (gp)->MODER = ((gp)->MODER & ~(GPIO_MODER_MODER0 << ((index) * 2))) | (GPIO_Mode_OUT << ((index) * 2))
#define PIN_AF(gp, index)               (gp)->MODER = ((gp)->MODER & ~(GPIO_MODER_MODER0 << ((index) * 2))) | (GPIO_Mode_AF << ((index) * 2))
#define PIN_ANA(gp, index)              (gp)->MODER = ((gp)->MODER & ~(GPIO_MODER_MODER0 << ((index) * 2))) | (GPIO_Mode_AN << ((index) * 2))

#define PIN_GETMODE(gp, index)          (((gp)->MODER >> ((index)*2)) & GPIO_MODER_MODER0)

#define PIN_PULL_NONE(gp, index)        (gp)->PUPDR = ((gp)->PUPDR & ~(GPIO_PUPDR_PUPDR0 << ((index) * 2))) | ((GPIO_PuPd_NOPULL) << ((index) * 2))
#define PIN_PULL_UP(gp, index)          (gp)->PUPDR = ((gp)->PUPDR & ~(GPIO_PUPDR_PUPDR0 << ((index) * 2))) | ((GPIO_PuPd_UP) << ((index) * 2))
#define PIN_PULL_DOWN(gp, index)        (gp)->PUPDR = ((gp)->PUPDR & ~(GPIO_PUPDR_PUPDR0 << ((index) * 2))) | ((GPIO_PuPd_DOWN) << ((index) * 2))
#define PIN_PULL_TYPE(gp, index, type)  (gp)->PUPDR = ((gp)->PUPDR & ~(GPIO_PUPDR_PUPDR0 << ((index) * 2))) | ((type) << ((index) * 2));

#define PIN_PP(gp, index)               (gp)->OTYPER = ((gp)->OTYPER & ~(GPIO_OTYPER_OT_0 << ((index)))) | (GPIO_OType_PP << ((index)))
#define PIN_OD(gp, index)               (gp)->OTYPER = ((gp)->OTYPER & ~(GPIO_OTYPER_OT_0 << ((index)))) | (GPIO_OType_OD << ((index)))

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
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

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(COND,MSG) typedef char static_assertion_##MSG[(COND)?1:-1]
#endif


#endif //PORTABLE_H
