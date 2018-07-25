#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>
#include "stm32f0xx.h"

enum GPIO_MODE {
  MODE_INPUT = 0,
  MODE_OUTPUT = 1,
  MODE_ALTERNATE = 2,
  MODE_ANALOG = 3
};

enum GPIO_PULL {
  PULL_NONE = 0,
  PULL_UP = 1,
  PULL_DOWN = 2
};

enum GPIO_SPEED {
  SPEED_LOW = 0,
  SPEED_MEDIUM = 1,
  SPEED_HIGH = 3
};

enum GPIO_TYPE {
  TYPE_PUSHPULL = 0,
  TYPE_OPENDRAIN = 1
};

#define GPIO_DEFINE(PORT, PIN) { \
  static GPIO_TypeDef * const bank = GPIO##PORT; \
  static const uint32_t mask = (1 << PIN); \
  static const uint32_t pin = PIN; \
  static inline void set() { bank->BSRR = mask; } \
  static inline void reset() { bank->BRR = mask; } \
  static inline uint32_t read() { return bank->IDR & mask; } \
  static inline void mode(GPIO_MODE mode) { \
    const uint32_t ps = pin * 2; \
    __disable_irq(); \
    bank->MODER = (bank->MODER & ~(0x3U << ps)) | (mode << ps); \
    __enable_irq(); \
  } \
  static inline void type(GPIO_TYPE type) { \
    __disable_irq(); \
    bank->OTYPER = (bank->OTYPER & ~mask) | (type << pin); \
    __enable_irq(); \
  } \
  static inline void speed(GPIO_SPEED speed) { \
    const uint32_t ps = pin * 2; \
    __disable_irq(); \
    bank->OSPEEDR = (bank->OSPEEDR & ~(0x3U << ps)) | (speed << ps); \
    __enable_irq(); \
  } \
  static inline void pull(GPIO_PULL pull) { \
    const uint32_t ps = pin * 2; \
    __disable_irq(); \
    bank->PUPDR = (bank->PUPDR & ~(0x3U << ps)) | (pull << ps); \
    __enable_irq(); \
  } \
  static inline void alternate(uint32_t funct) { \
    const uint32_t ps = (pin & 0x07) * 4; \
    const uint32_t pb = pin / 8; \
    __disable_irq(); \
    bank->AFR[pb] = (bank->AFR[pb] & ~(0xFU << ps)) | (funct << ps); \
    __enable_irq(); \
  } \
}

#endif
