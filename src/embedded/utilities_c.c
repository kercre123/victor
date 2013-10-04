#include "anki/embeddedCommon/utilities_c.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

//#ifdef USING_MOVIDIUS_COMPILER
//#define putchar DrvApbUartPutChar
//#endif

#define MAX_PRINTF_DIGITS 50
#define PRINTF_BUFFER_SIZE 1024
int printfBuffer[PRINTF_BUFFER_SIZE];
void explicitPrintf(int reverseWords, const char *format, ...)
{
  const char * const formatStart = format;
  int digits[MAX_PRINTF_DIGITS];
  //  int numArguments;
  int numCharacters;
  int i;
  va_list arguments;

  //#ifdef USING_MOVIDIUS_GCC_COMPILER
  //  // TODO: figure out how to make this work without reversing
  //  reverseWords = 1;
  //#endif

  // Count the number of characters
  numCharacters = 0;
  while(*format != 0x00)
  {
    numCharacters++;
    format++;
  }
  if(reverseWords) {
    numCharacters = 4 * ((numCharacters+3) / 4);
  }

  numCharacters = MIN(numCharacters, MAX_PRINTF_DIGITS-5);

  format = formatStart;

  //// Count the number of arguments
  //numArguments = 0;
  //while(*format != 0x00)
  //{
  //  if(*format == '%') {
  //    numArguments++;
  //  }

  //  format++;
  //}
  //format = formatStart;

  // Reverse the string

  if(reverseWords) {
    for(i=0; i<(numCharacters+3); i+=4) {
      printfBuffer[i] = format[i+3];
      printfBuffer[i+1] = format[i+2];
      printfBuffer[i+2] = format[i+1];
      printfBuffer[i+3] = format[i];
    }

    printfBuffer[i] = 0x00;
  } else {
    /*putchar('V');
    putchar('V');
    putchar('V');
    putchar('V');
    return;*/

    /*for(i=0; i<numCharacters; i++) {
    printfBuffer[i] = format[i];
    }*/
    //printfBuffer[i] = 0x00;

    for(i=0; i<numCharacters; i++) {
      printfBuffer[i] = format[i];
    }
    printfBuffer[i] = 0x00;
  }

  //va_start(arguments, numArguments);

  va_start(arguments, format);

  for(i=0; i<(numCharacters+3); i++) {
    if(printfBuffer[i] == '%') {
      int j;
      int value;
      const char percentChar = printfBuffer[i+1];
      i++;

      if(percentChar == 'd') {
        for(j=0; j<MAX_PRINTF_DIGITS; j++) {
          digits[j] = 0;
        }

        value = va_arg(arguments, int);
        if(value < 0) {
          putchar('-');
          value = -value;
        }

        if(value == 0) {
          putchar('0');
          putchar(' ');
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
          putchar(digits[j] + 48);
        }

        putchar(' ');
      } else if(percentChar == 's') {
        char* value = va_arg(arguments, char*);
        while(*value != 0x00) {
          putchar(*value);
          value++;
        }
      } else {
        if(printfBuffer[i] == 0x00)
          break;

        putchar(printfBuffer[i]);
      }
    } else {
      if(printfBuffer[i] == 0x00)
        break;

      putchar(printfBuffer[i]);
    }
  } // for(i=0; i<numCharacters; i++)

  va_end(arguments);
}

#if defined(USING_MOVIDIUS_GCC_COMPILER)
IN_DDR void* explicitMemset(void * dst, int value, size_t size)
{
  size_t i;
  for(i=0; i<size; i++)
  {
    ((char*)dst)[i] = value;
  }

  return dst;
}
#endif // #if defined(USING_MOVIDIUS_GCC_COMPILER)

#if defined(USING_MOVIDIUS_GCC_COMPILER)
IN_DDR f32 powF32S32(const f32 x, const s32 y)
{
  s32 i;
  f32 xAbs;
  f32 result = 1.0f;

  if(y == 0)
    return result;

  xAbs = ABS(x);

  for(i=1; i<=y; i++) {
    result *= x;
  }

  if(x < 0.0f)
    result = 1.0f / result;

  return result;
}
#endif // #if defined(USING_MOVIDIUS_GCC_COMPILER)

#if defined(USING_MOVIDIUS_GCC_COMPILER)
IN_DDR f64 powF64S32(const f64 x, const s32 y)
{
  s32 i;
  f64 xAbs;
  f64 result = 1.0;

  if(y == 0)
    return result;

  xAbs = ABS(x);

  for(i=1; i<=y; i++) {
    result *= x;
  }

  if(x < 0.0)
    result = 1.0 / result;

  return result;
}
#endif // #if defined(USING_MOVIDIUS_GCC_COMPILER)