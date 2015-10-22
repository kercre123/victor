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

#include "lib/stm32f2xx.h"

#define GPIO_SET(gp, index)             (gp)->BSRRL = (1 << (index))
#define GPIO_RESET(gp, index)           (gp)->BSRRH = (1 << (index))
#define GPIO_READ(gp)                   (gp)->IDR

#define PIN_IN(gp, index)               (gp)->MODER = ((gp)->MODER & ~(GPIO_MODER_MODER0 << ((index) * 2))) | (GPIO_Mode_IN << ((index) * 2))
#define PIN_OUT(gp, index)              (gp)->MODER = ((gp)->MODER & ~(GPIO_MODER_MODER0 << ((index) * 2))) | (GPIO_Mode_OUT << ((index) * 2))
#define PIN_AF(gp, index)               (gp)->MODER = ((gp)->MODER & ~(GPIO_MODER_MODER0 << ((index) * 2))) | (GPIO_Mode_AF << ((index) * 2))

#define PIN_PULL_NONE(gp, index)        (gp)->PUPDR = ((gp)->PUPDR & ~(GPIO_PUPDR_PUPDR0 << ((index) * 2))) | ((GPIO_PuPd_NOPULL) << ((index) * 2))
#define PIN_PULL_UP(gp, index)          (gp)->PUPDR = ((gp)->PUPDR & ~(GPIO_PUPDR_PUPDR0 << ((index) * 2))) | ((GPIO_PuPd_UP) << ((index) * 2))
#define PIN_PULL_DOWN(gp, index)        (gp)->PUPDR = ((gp)->PUPDR & ~(GPIO_PUPDR_PUPDR0 << ((index) * 2))) | ((GPIO_PuPd_DOWN) << ((index) * 2))
#define PIN_PULL_TYPE(gp, index, type)  (gp)->PUPDR = ((gp)->PUPDR & ~(GPIO_PUPDR_PUPDR0 << ((index) * 2))) | ((type) << ((index) * 2));

#define PIN_PP(gp, index)               (gp)->OTYPER = ((gp)->OTYPER & ~(GPIO_OTYPER_OT_0 << ((index)))) | (GPIO_OType_PP << ((index)))
#define PIN_OD(gp, index)               (gp)->OTYPER = ((gp)->OTYPER & ~(GPIO_OTYPER_OT_0 << ((index)))) | (GPIO_OType_OD << ((index)))

#ifndef BOOL
  typedef u8 BOOL;
#endif

#ifndef NULL
# define NULL 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

#ifndef FALSE
# define FALSE 0
#endif

typedef int error_t;
typedef void (*TestFunction)(void);

extern char* const g_hex;

// ct_assert is a compile time assertion, useful for checking sizeof() and other compile time knowledge
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define ct_assert(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }

#endif
