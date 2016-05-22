// This file abstracts some low-level details about the K02
#ifndef PORTABLE_H
#define PORTABLE_H

// Basic abstractions for low level I/O on our processor
#define GPIO_SET(gp, pin)                (gp)->PSOR = (pin)
#define GPIO_RESET(gp, pin)              (gp)->PCOR = (pin)
#define GPIO_READ(gp)                    (gp)->PDIR

// Note:  These are not interrupt-safe, so do not use in main thead except during init()
#define GPIO_IN(gp, pin)                 (gp)->PDDR &= ~(pin)
#define GPIO_OUT(gp, pin)                (gp)->PDDR |= (pin)

#include <stdint.h>
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// This sets up everything about the pin in one call - pull-up/pull-down, open-drain, altmux, etc
// OR together the bits you want from enum SourceSetup_t into setup
#define SOURCE_SETUP(gp, src, setup)    ((PORT_Type *)(PORTA_BASE + PORT_INDEX(gp)*0x1000))->PCR[(src)] = (setup)
typedef enum {
  SourcePullDown = 2,
  SourcePullUp = 3,
  SourceSlow = 4,         // Slow slew rate enabled
  SourceFilter = 0x10,    // Passive input filter enabled
  SourceOpenDrain = 0x20, // Open drain enabled
  SourceStrong = 0x40,    // High drive strength enabled (only works on a few pins, see datasheet)
  SourceAnalog = 0x000, // Analog mode
  SourceGPIO = 0x100,   // GPIO mode
  SourceAlt2 = 0x200,   // Alternate 2
  SourceAlt3 = 0x300,   // Alternate 3
  SourceAlt4 = 0x400,   // Alternate 4
  SourceAlt5 = 0x500,   // Alternate 5
  SourceAlt6 = 0x600,   // Alternate 6
  SourceAlt7 = 0x700,   // Alternate 7
  SourceLock = 0x8000,  // Lock the pin setup until next reset (to protect HW from buggy code)
  SourceDMARise   = 0x10000,  // DMA request on rising edge
  SourceDMAFall   = 0x20000,  // DMA request on falling edge
  SourceDMAEither = 0x30000,  // DMA request on either edge
  SourceIRQLow    = 0x80000,  // Interrupt when logic 0
  SourceIRQRise   = 0x90000,  // Interrupt on rising edge
  SourceIRQFall   = 0xA0000,  // Interrupt on falling edge
  SourceIRQEither = 0xB0000,  // Interrupt on either edge
  SourceIRQHigh   = 0xC0000,  // Interrupt when logic 1
  SourceIRQAck    = 0x1000000,// Acknowledge interrupt by writing this bit
} SourceSetup_t;

// This gets the port index of the GPIO, where A=0, B=1, etc
#define PORT_INDEX(gp) ((((int)gp)-PTA_BASE) >> 6)

// 100MHz core clock, /2 peripheral (bus) clock, /4 CAM_CLOCK, /10 I2SPI_CLOCK 
#define CORE_CLOCK (100000000)
#define BUS_CLOCK (CORE_CLOCK / 2)
#define CAM_CLOCK (CORE_CLOCK / 4)
#define I2SPI_CLOCK (CORE_CLOCK / 10)

// Consistent naming scheme for GPIO, pins, and source variables
// Set name to the name of the pin, ptx to the GPIO unit (PTA/PTB/etc), and index to the pin number
#define GPIO_PIN_SOURCE(name, ptx, index) \
  static const u8 SOURCE_##name = (index); \
  static const u32 PIN_##name = 1 << (index); \
  static GPIO_Type* const GPIO_##name = (ptx);

#ifdef __EDG__
  #pragma diag_suppress 177   // Suppress unused variable warning - since this generates a lot of them
  #define ASSERT_CONCAT_(a, b) a##b
  #define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
  #define static_assert(e, msg) \
    enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
#endif

// Allocate in RAM block 0 - dedicated to camera DMA and swizzling in camera.c
// This section can only be used by either CPU or DMA, not both at once
// Thus, the RAM is only useful at the start of HALExec() or with careful timing in camera.c
#define CAMRAM __attribute__((section("CAMRAM")))

// RAM for block 1 - generally usable for executing code
#define CODERAM __attribute__((section("CODERAM")))

#endif
