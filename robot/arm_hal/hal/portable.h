#ifndef PORTABLE_H
#define PORTABLE_H

// Basic abstractions for low level I/O on our processor
#define GPIO_SET(gp, pin)               (gp)->BSRRL = (pin)
#define GPIO_RESET(gp, pin)             (gp)->BSRRH = (pin)
#define GPIO_READ(gp)                   (gp)->IDR

#define PIN_IN(gp, src)                 (gp)->MODER &= ~(3 << ((src) * 2))
#define PIN_OUT(gp, src)                (gp)->MODER = ((gp)->MODER & ~(3 << ((src) * 2))) | ((GPIO_Mode_OUT << ((src) * 2)))
#define PIN_AF(gp, src)                 (gp)->MODER = ((gp)->MODER & ~(3 << ((src) * 2))) | ((GPIO_Mode_AF << ((src) * 2)))

#define PIN_SETAF(gp, src, af)          (gp)->AFR[(src) >> 3] = ((gp)->AFR[(src) >> 3] & ~((u32)(0xF) << (((src) & 7) * 4))) | ((af) << (((src) & 7) * 4))
#define PIN_PULLUP(gp, src)             (gp)->PUPDR = ((gp)->PUPDR & ~(3 << ((src) * 2))) | ((GPIO_PuPd_UP << ((src) * 2)))
#define PIN_PULLDOWN(gp, src)           (gp)->PUPDR = ((gp)->PUPDR & ~(3 << ((src) * 2))) | ((GPIO_PuPd_DOWN << ((src) * 2)))
#define PIN_NOPULL(gp, src)             (gp)->PUPDR = ((gp)->PUPDR & ~(3 << ((src) * 2))) | ((GPIO_PuPd_NOPULL << ((src) * 2)))
#define PIN_PULL_TYPE(gp, src, type)    (gp)->PUPDR = ((gp)->PUPDR & ~(GPIO_PUPDR_PUPDR0 << ((src) * 2))) | ((type) << ((src) * 2))

#define PIN_PP(gp, src)                 (gp)->OTYPER &= ~(1 << (src))
#define PIN_OD(gp, src)                 (gp)->OTYPER |= (1 << (src))

// 8 MHz input clock (external) with 180 MHz output
#define CORE_CLOCK_MHZ (180)
#define APB1_CLOCK_MHZ (CORE_CLOCK_MHZ >> 1)

// Consistent naming scheme for GPIO, pins, and source variables
#define GPIO_PIN_SOURCE(name, gpio, index) \
  const u8 SOURCE_##name = (index); \
  const u32 PIN_##name = 1 << (index); \
  GPIO_TypeDef* const GPIO_##name = (gpio);
  
#ifdef __EDG__
# define ASSERT_CONCAT_(a, b) a##b
# define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
# define static_assert(e, msg) \
    enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
#endif

#endif
