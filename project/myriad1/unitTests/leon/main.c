///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     main leon file
///

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "isaac_registers.h"
#include "mv_types.h"
#include "DrvSvu.h"
#include <DrvSvuDebug.h>
#include "DrvCpr.h"
#include "DrvTimer.h"
#include <DrvL2Cache.h>

#include "swcShaveLoader.h"
#include "app_config.h"
#include "swcTestUtils.h"

#include "cppInterface.h"

#ifndef IN_DDR
#define IN_DDR __attribute__((section(".ddr_direct.text")))
//#define IN_DDR __attribute__((section(".ddr.text")))
#endif

performanceStruct perfStr;

// Copied from leon3.h
__inline__ void sparc_leon3_disable_cache(void) {
  //asi 2
  __asm__ volatile ("lda [%%g0] 2, %%l1\n\t"  \
    "set 0x00000f, %%l2\n\t"  \
    "andn  %%l2, %%l1, %%l2\n\t" \
    "sta %%l2, [%%g0] 2\n\t"  \
    :  : : "l1", "l2");
};

//#define PRINTF_BUFFER_SIZE 1024
//IN_DDR int printfBuffer[PRINTF_BUFFER_SIZE];
//void explicitPrintf(const char *format, ...)
//{
//#define MAX_PRINTF_DIGITS 50
//  int digits[MAX_PRINTF_DIGITS];
//
//  // Count the number of characters
//  int numCharacters = 0;
//  const char * const formatStart = format;
//  while(*format != 0x00)
//  {
//    numCharacters++;
//    format++;
//  }
//  //numCharacters = 4 * ((numCharacters+3) / 4);
//
//  format = formatStart;
//
//  // Count the number of arguments
//  int numArguments = 0;
//  while(*format != 0x00)
//  {
//    if(*format == '%') {
//      numArguments++;
//    }
//
//    format++;
//  }
//  format = formatStart;
//
//  // Reverse the string
//  int i;
//  for(i=0; i<numCharacters; i+=4) {
//    printfBuffer[i] = format[i+3];
//    printfBuffer[i+1] = format[i+2];
//    printfBuffer[i+2] = format[i+1];
//    printfBuffer[i+3] = format[i];
//  }
//  printfBuffer[i-3] = 0x00;
//
//  /*
//  for(i=0; i<numCharacters; i++) {
//  DrvApbUartPutChar(printfBuffer[i]);
//  }
//
//  printf("a  \n");
//  printf(&printfBuffer[0]);
//  printf("b  \n");
//  */
//  va_list arguments;
//  va_start(arguments, numArguments);
//
//  for(i=0; i<numCharacters; i++) {
//    if(printfBuffer[i] == '%') {
//      int j;
//      const char percentChar = printfBuffer[i+1];
//      i++;
//
//      if(percentChar == 'd') {
//        for(j=0; j<MAX_PRINTF_DIGITS; j++) {
//          digits[j] = 0;
//        }
//
//        int value = va_arg(arguments, int);
//        if(value < 0) {
//          DrvApbUartPutChar('-');
//          value = -value;
//        }
//
//        if(value == 0) {
//          DrvApbUartPutChar('0');
//          DrvApbUartPutChar(' ');
//          format++;
//          continue;
//        }
//
//        j=0;
//        while(value > 0) {
//          const int curDigit = value - (10*(value/10));
//
//          digits[j++] = curDigit;
//
//          value /= 10;
//        }
//
//        j--;
//        for( ; j>=0; j--) {
//          DrvApbUartPutChar(digits[j] + 48);
//        }
//
//        DrvApbUartPutChar(' ');
//      } else if(percentChar == 's') {
//        char* value = va_arg(arguments, char*);
//        while(*value != 0x00) {
//          DrvApbUartPutChar(*value);
//          value++;
//        }
//      } else {
//        DrvApbUartPutChar(printfBuffer[i]);
//      }
//    } else {
//      DrvApbUartPutChar(printfBuffer[i]);
//    }
//  } // for(i=0; i<numCharacters; i++)
//
//  va_end(arguments);
//
//  /*va_list args;
//  va_start(args, numArguments);
//  printf(printfBuffer, args);
//  va_end(args);*/
//}

//void explicitPrintf(const char *format, ...)
//{
//#define MAX_PRINTF_DIGITS 50
//  int digits[MAX_PRINTF_DIGITS];
//  int curChar = 0;
//  int percentSignFound = 0;
//
//  int numArguments = 0;
//  int previousChar = ' ';
//
//  const char * const formatStart = format;
//  while(format != 0x00)
//  {
//    if(*format == '%') {
//      numArguments++;
//    }
//
//    format++;
//  }
//  format = formatStart;
//
//  va_list arguments;
//  va_start(arguments, numArguments);
//
//  while(format != 0x00) {
//    if(curChar >= 3) {
//      curChar = 0;
//      DrvApbUartPutChar(*format);
//      DrvApbUartPutChar(charBuffer[2]);
//      DrvApbUartPutChar(charBuffer[1]);
//      DrvApbUartPutChar(charBuffer[0]);
//      format++;
//    } else { // if(curChar >= 3)
//      if(percentSignFound) {
//        int i;
//        for(i=0; i<MAX_PRINTF_DIGITS; i++) {
//          digits[i] = 0;
//        }
//
//        int value = va_arg(arguments, int);
//        if(value < 0) {
//          DrvApbUartPutChar('-');
//          value = -value;
//        }
//
//        if(value == 0) {
//          DrvApbUartPutChar('0');
//          DrvApbUartPutChar(' ');
//          format++;
//          continue;
//        }
//
//        i=0;
//        while(value > 0) {
//          const int curDigit = value - (10*(value/10));
//
//          digits[i++] = curDigit;
//
//          value /= 10;
//        }
//
//        i--;
//        for( ; i>=0; i--) {
//          DrvApbUartPutChar(digits[i] + 48);
//        }
//
//        DrvApbUartPutChar(' ');
//        format++;
//
//        percentSignFound = 0;
//      } else { // if(percentSignFound)
//        if(*format == '%') {
//          while(curChar <= 3) {
//            charBuffer[curChar++] = ' ';
//          }
//          percentSignFound = 1;
//          previousChar = *(format - 1);
//          format++;
//        } else {
//          charBuffer[curChar++] = *format;
//          format++;
//        }
//      } // if(percentSignFound) ... else
//    } // if(curChar >= 3) ... else
//  } // while(format != 0x00)
//  va_end(arguments);
//} // void explicitPrintf(const char *format, ...)

void realMain()
{
  u32          i;
  u32          *fl;
  u32          test_pass = 1;

  *(volatile u32*)MXI_CMX_CTRL_BASE_ADR |= (1 << 24);

  DrvL2CacheSetupPartition(PART128KB);
  DrvL2CacheAllocateSetPartitions();
  SET_REG_WORD(L2C_MXITID_ADR, 0x0);

  sparc_leon3_disable_cache();

  //explicitPrintf("Hello dog %d  \n", 1234);
  //explicitPrintf("Hello dog %s %d     \n", "piggle", 1234);

  //printf("Starting unit tests\n");

  swcShaveProfInit(&perfStr);

  perfStr.countShCodeRun = 2;

  swcLeonFlushCaches();

  swcShaveProfStartGathering(0, &perfStr);

  runTests();

  swcShaveProfStopGathering(0, &perfStr);

  //    The below printf is modified from swcShaveProfPrint(0, &perfStr);
  //printf("\nLeon executed %d cycles in %06d micro seconds ([%d ms])\n",(u32)(perfStr.perfCounterTimer), (u32)(DrvTimerTicksToMs(perfStr.perfCounterTimer)*1000), (u32)(DrvTimerTicksToMs(perfStr.perfCounterTimer)));

  //printf("Finished unit tests\n");
}

int __attribute__((section(".sys.text.start"))) main(void)
{
  initClocksAndMemory();

  //    DrvTimerInit();

  //    SleepMs(10000);

  //    const u64 startTime = DrvTimerGetSysTicks64();

  //    while((DrvTimerGetSysTicks64() - startTime) < 180000000*10000) {}

  //printf("Start\n");
  realMain();

  return 0;
}