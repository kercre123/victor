#ifndef COZMO_TYPES_H
#define COZMO_TYPES_H

#include "anki/common/shared/radians.h"

#include "anki/common/types.h"
#include "anki/common/robot/geometry.h"

// Some of this should be moved to a shared file to be used by basestation as well

// Memory map for our core (STM32F051)
#define CORE_CLOCK          48000000
#define CORE_CLOCK_LOW      (CORE_CLOCK / 6)
#define ROM_SIZE            65536
#define RAM_SIZE            8192
#define ROM_START           0x08000000
#define FACTORY_BLOCK       0x08001400
#define USER_BLOCK_A        0x08001800
#define USER_BLOCK_B        0x08001C00    // Must follow block A
#define APP_START           0x08002000
#define APP_END             0x08010000
#define RAM_START           0x20000000
#define CRASH_REPORT_START  0x20001fc0    // Final 64 bytes of RAM
#define VECTOR_TABLE_ENTRIES 48

// So we can explicitly specify when something is interpreted as a boolean value
typedef u8 BOOL;


///// Misc. vars that may or may not belong here

#define INVALID_IDEAL_FOLLOW_LINE_IDX s16_MAX


#endif // COZMO_TYPES_H
