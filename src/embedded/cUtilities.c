#include "anki/embeddedCommon/benchmarking.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef MAX
#define MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef MIN
#define MIN( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

#ifdef USING_MOVIDIUS_GCC_COMPILER
#define PRINTF_BUFFER_SIZE 1024
int printfBuffer[PRINTF_BUFFER_SIZE];
void explicitPrintf(const char *format, ...)
{
#define MAX_PRINTF_DIGITS 50
  int digits[MAX_PRINTF_DIGITS];

  // Count the number of characters
  int numCharacters = 0;
  const char * const formatStart = format;
  while(*format != 0x00)
  {
    numCharacters++;
    format++;
  }
  numCharacters = 4 * ((numCharacters+3) / 4);

  format = formatStart;

  // Count the number of arguments
  int numArguments = 0;
  while(*format != 0x00)
  {
    if(*format == '%') {
      numArguments++;
    }

    format++;
  }
  format = formatStart;

  // Reverse the string
  int i;
  for(i=0; i<numCharacters; i+=4) {
    printfBuffer[i] = format[i+3];
    printfBuffer[i+1] = format[i+2];
    printfBuffer[i+2] = format[i+1];
    printfBuffer[i+3] = format[i];
  }
  printfBuffer[i-3] = 0x00;

  va_list arguments;
  va_start(arguments, numArguments);

  for(i=0; i<numCharacters; i++) {
    if(printfBuffer[i] == '%') {
      int j;
      const char percentChar = printfBuffer[i+1];
      i++;

      if(percentChar == 'd') {
        for(j=0; j<MAX_PRINTF_DIGITS; j++) {
          digits[j] = 0;
        }

        int value = va_arg(arguments, int);
        if(value < 0) {
          DrvApbUartPutChar('-');
          value = -value;
        }

        if(value == 0) {
          DrvApbUartPutChar('0');
          DrvApbUartPutChar(' ');
          format++;
          continue;
        }

        j=0;
        while(value > 0) {
          const int curDigit = value - (10*(value/10));

          digits[j++] = curDigit;

          value /= 10;
        }

        j--;
        for( ; j>=0; j--) {
          DrvApbUartPutChar(digits[j] + 48);
        }

        DrvApbUartPutChar(' ');
      } else if(percentChar == 's') {
        char* value = va_arg(arguments, char*);
        while(*value != 0x00) {
          DrvApbUartPutChar(*value);
          value++;
        }
      } else {
        DrvApbUartPutChar(printfBuffer[i]);
      }
    } else {
      DrvApbUartPutChar(printfBuffer[i]);
    }
  } // for(i=0; i<numCharacters; i++)

  va_end(arguments);
}

#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER