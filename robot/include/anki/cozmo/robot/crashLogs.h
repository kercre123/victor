#ifndef __CRASHLOGS_H
#define __CRASHLOGS_H
/** Struct and enum definitions for robot firmware crash records to be stored on the robot and reported back later
 * @author Bryon Vandiver <bryon@anki.com>
 * @author Daniel Casner <daniel@anki.com>
 */

#include "anki/cozmo/robot/ctassert.h"

#define CRASH_RECORD_SIZE (1024)
#define CRASH_RECORD_PAYLOAD_SIZE (CRASH_RECORD_SIZE-16)

typedef struct {
  uint32_t nWritten;  ///< Write to 0 when written
  uint32_t nReported; ///< Wrtie to 0 when reported
  int32_t  reporter;  ///< Who crashed
  int32_t  errorCode; ///< Simple error type code
  uint32_t dump[CRASH_RECORD_PAYLOAD_SIZE/sizeof(uint32_t)]; ///< Actual crash dump data
} CrashRecord;

ct_assert(sizeof(CrashRecord) == CRASH_RECORD_SIZE);

typedef struct {
  uint32_t r8;
  uint32_t r9;
  uint32_t r10;
  uint32_t r11;
  uint32_t r4;
  uint32_t r5;
  uint32_t r6;
  uint32_t r7;
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  uint32_t lr;
  uint32_t pc;
  uint32_t psr;
} CrashLog_NRF;

ct_assert(sizeof(CrashLog_NRF) <= CRASH_RECORD_PAYLOAD_SIZE);

typedef struct {
  uint32_t r8;
  uint32_t r9;
  uint32_t r10;
  uint32_t r11;
  uint32_t r4;
  uint32_t r5;
  uint32_t r6;
  uint32_t r7;
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  uint32_t lr;
  uint32_t pc;
  uint32_t psr;

  // These are not stacked automatically
  uint32_t bfar;
  uint32_t cfsr;
  uint32_t hfsr;
  uint32_t dfsr;
  uint32_t afsr;
  uint32_t shcsr;
} CrashLog_K02;

ct_assert(sizeof(CrashLog_K02) <= CRASH_RECORD_PAYLOAD_SIZE);

typedef struct {
  int epc1;
  int ps;
  int sar;
  int xx1;
  int a0;
  int a2;
  int a3;
  int a4;
  int a5;
  int a6;
  int a7;
  int a8;
  int a9;
  int a10;
  int a11;
  int a12;
  int a13;
  int a14;
  int a15;
  int exccause;
} ex_regs_esp;

#define ESP_STACK_DUMP_SIZE  ((CRASH_RECORD_PAYLOAD_SIZE - sizeof(ex_regs_esp) - (sizeof(int)*10))/sizeof(int))

#define ESP_LOG_FORMAT_MAGIC (0xCDCD0000)

typedef struct {
  ex_regs_esp regs;
  int sp;
  int excvaddr;
  int depc;
  int logFormat;
  int version;
  uint32_t appRunID[4];
  int stackDumpSize;
  int stack[ESP_STACK_DUMP_SIZE];
} CrashLog_ESP;

ct_assert(sizeof(CrashLog_ESP) <= CRASH_RECORD_PAYLOAD_SIZE);

typedef struct {
  uint8_t integralDrift;
  uint8_t phaseErrorCount;
  uint8_t rxOverflowCount;
  uint8_t txOverflowCount;
  uint8_t relayWriteInd;
  uint8_t relayReadInd;
  uint8_t lastRelayBuffer[512]; //RELAY_BUFFER_SIZE from i2spi.c
} CrashLog_I2Spi;
  
ct_assert(sizeof(CrashLog_I2Spi) <= CRASH_RECORD_PAYLOAD_SIZE);

typedef struct {
   void* addr;
   int32_t error_code;
} CrashLog_BootError;
#define BOOT_ERROR_MAX_ENTRIES  (CRASH_RECORD_PAYLOAD_SIZE / sizeof(CrashLog_BootError))
ct_assert(sizeof(CrashLog_BootError)*BOOT_ERROR_MAX_ENTRIES <= CRASH_RECORD_PAYLOAD_SIZE);



#endif
