#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>

#if defined(STM32F2XX) //fixture mcu
  #include "stm32f2xx.h"
#elif defined(STM32F030xC) //body mcu
  #include "stm32f0xx.h"
#else
  #error No MCU target defined
#endif

#ifdef __cplusplus

enum GPIO_MODE {      //GPIOMode_TypeDef
  MODE_INPUT = 0,     //GPIO_Mode_IN
  MODE_OUTPUT = 1,    //GPIO_Mode_OUT
  MODE_ALTERNATE = 2, //GPIO_Mode_AF
  MODE_ANALOG = 3     //GPIO_Mode_AN
};

enum GPIO_PULL {      //GPIOPuPd_TypeDef
  PULL_NONE = 0,      //GPIO_PuPd_NOPULL
  PULL_UP = 1,        //GPIO_PuPd_UP
  PULL_DOWN = 2       //GPIO_PuPd_DOWN
};

enum GPIO_SPEED {     //GPIOSpeed_TypeDef
  SPEED_LOW = 0,      //STM32F2XX: GPIO_Speed_2MHz   -- STM32F030xC: GPIO_Speed_Level_1 (Low 2 MHz)
  SPEED_MEDIUM = 1,   //STM32F2XX: GPIO_Speed_25MHz  -- STM32F030xC: GPIO_Speed_Level_2 (Medium 10 MHz)
  //SPEED_2,          //STM32F2XX: GPIO_Speed_50MHz  -- STM32F030xC: (undefined)
  SPEED_HIGH = 3      //STM32F2XX: GPIO_Speed_100MHz -- STM32F030xC: GPIO_Speed_Level_3 (High 50 MHz)
};

enum GPIO_TYPE {      //GPIOOType_TypeDef
  TYPE_PUSHPULL = 0,  //GPIO_OType_PP
  TYPE_OPENDRAIN = 1  //GPIO_OType_OD
};

//slightly different register definitions for bit set/reset
#if defined(STM32F2XX)
static inline void gpio_set_(GPIO_TypeDef * const bank, const uint32_t mask) { bank->BSRRL = mask; }
static inline void gpio_reset_(GPIO_TypeDef * const bank, const uint32_t mask) { bank->BSRRH = mask; }
#elif defined(STM32F030xC)
static inline void gpio_set_(GPIO_TypeDef * const bank, const uint32_t mask) { bank->BSRR = mask; }
static inline void gpio_reset_(GPIO_TypeDef * const bank, const uint32_t mask) { bank->BRR = mask; }
#endif

#define GPIO_DEFINE(PORT, PIN) { \
  static GPIO_TypeDef * const bank  = GPIO##PORT; \
  static const uint32_t       mask = (1 << PIN); \
  static const uint32_t       pin = PIN; \
  static inline void      set() { gpio_set_(bank,mask); } \
  static inline void      reset() { gpio_reset_(bank,mask); } \
  static inline bool      read() { return (bank->IDR & mask) > 0; } \
  static inline void      write(bool set_) { if(set_) set(); else reset(); } \
  static inline void      mode(GPIO_MODE m) { bank->MODER = (bank->MODER & ~(0x3U << (pin*2))) | (m << (pin*2)); } \
  static inline void      pull(GPIO_PULL p) { bank->PUPDR = (bank->PUPDR & ~(0x3U << (pin*2))) | (p << (pin*2)); } \
  static inline void      type(GPIO_TYPE t) { bank->OTYPER = (bank->OTYPER & ~mask) | (t << pin); } \
  static inline void      speed(GPIO_SPEED s) { bank->OSPEEDR = (bank->OSPEEDR & ~(0x3U << (pin*2))) | (s << (pin*2)); } \
  static inline void      init(GPIO_MODE m, GPIO_PULL p=PULL_NONE, GPIO_TYPE t=TYPE_PUSHPULL, GPIO_SPEED s=SPEED_HIGH) { mode(m); pull(p); type(t); speed(s); } \
  static inline void      alternate(uint32_t AFx /*GPIO_AF_xxx*/) { \
    const uint32_t ps = (pin & 0x07) * 4; \
    const uint32_t pb = pin / 8; \
    bank->AFR[pb] = (bank->AFR[pb] & ~(0xFU << ps)) | ((AFx & 0xFU) << ps); } \
  static inline int       getMode(void) { return (bank->MODER >> (pin*2)) & 0x3U; } \
  static inline int       getPull(void) { return (bank->PUPDR >> (pin*2)) & 0x3U; } \
  static inline int       getType(void) { return (bank->OTYPER >> pin) & 0x1U; } \
  static inline int       getSpeed(void) { return (bank->OSPEEDR >> (pin*2)) & 0x3U; } \
}

#endif /* __cplusplus */

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
