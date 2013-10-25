#include "anki/common/robot/utilities_c.h"

#if defined(_MSC_VER)
#include <windows.h>
#elif defined(USING_MOVIDIUS_COMPILER)
#include "DrvTimer.h"
#else
#include <sys/time.h>
#endif

#define MAX_PRINTF_DIGITS 50
#define PRINTF_BUFFER_SIZE 1024
int printfBuffer[PRINTF_BUFFER_SIZE];
// int printfBuffer2[PRINTF_BUFFER_SIZE]; // For a single level of recusion
void explicitPrintf(int reverseEachFourCharacters, const char *format, ...)
{
  /*va_list arguments;
  va_start(arguments, format);
  explicitPrintfWithExplicitBuffer(reverseEachFourCharacters, &printfBuffer[0], format, arguments);
  va_end(arguments);*/
  const char * const formatStart = format;
  int numCharacters;
  int i;
  va_list arguments;

  // Count the number of characters
  numCharacters = 0;
  while(*format != 0x00)
  {
    numCharacters++;
    format++;
  }
  format = formatStart;

  if(reverseEachFourCharacters)
    numCharacters += 3;

  numCharacters = MIN(numCharacters, PRINTF_BUFFER_SIZE-5);

  // Reverse the string
  if(reverseEachFourCharacters) {
    for(i=0; i<numCharacters; i+=4) {
      printfBuffer[i] = format[i+3];
      if(format[i+3] == 0x00) {
        printfBuffer[i+1] = 0x00;
      } else {
        printfBuffer[i+1] = format[i+2];
        if(format[i+2] == 0x00) {
          printfBuffer[i+2] = 0x00;
        } else {
          printfBuffer[i+2] = format[i+1];
          if(format[i+1] == 0x00) {
            printfBuffer[i+3] = 0x00;
          } else {
            printfBuffer[i+3] = format[i];
          }
        }
      }
    }
  } else {
    for(i=0; i<numCharacters; i++) {
      printfBuffer[i] = format[i];
    }
  }

  va_start(arguments, format);

  for(i=0; i<numCharacters; i++) {
    if(printfBuffer[i] == '%') {
      const char percentChar = printfBuffer[i+1];
      i++;

      if(percentChar == 'd') {
        const s32 value = va_arg(arguments, s32);
        PrintInt(value);
        format++;
      } else if(percentChar == 'x') {
        const u32 value = va_arg(arguments, u32);
        PrintHex(value);
        format++;
      } else if(percentChar == 'f') {
        // TODO: should this be double?
        const f64 value = va_arg(arguments, f64);
        PrintFloat(value);
        format++;
      } else if(percentChar == 's') {
        const char * stringArgument = va_arg(arguments, char*);
        while(*stringArgument != 0x00) {
          putchar(*stringArgument);
          stringArgument++;
        }
        //explicitPrintfWithExplicitprintfBuffer(reverseEachFourCharacters, &printfprintfBuffer2[0], stringArgument);
      }
      else {
        if(printfBuffer[i] == 0x00)
          break;

        putchar('%');
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

void PrintFloat(f64 value)
{
  const s32 maxDecimalDigits = 6;
  const f64 decimalPart = value - (f64)floorf(value);

  s32 digitIndex = -1;
  s32 numDecimalDigitsUsed = 0;

  if(value < 0.0) {
    putchar('-');
    value = -value;
  }

  // Print the part before the decimal digit

  if(value > 10000000000.0) {
    const f64 topPart = (f64)value / 10000000000.0;
    PrintInt((s32)floorf(topPart));
    putchar('.');
    putchar('.');
    putchar('.');
    return;
  }

  PrintInt((s32)floorf(value));

  // The remainder of this function prints the part after the decimal digit
  value = decimalPart;

  putchar('.');

  if(value < (1.0f / powf(10.0f, maxDecimalDigits-1))) {
    putchar('0');

    return;
  }

  // This loop prints out the part after the decimal point
  digitIndex = 0;
  while(value > 0.0f && numDecimalDigitsUsed < maxDecimalDigits) {
    s32 curDigit;

    numDecimalDigitsUsed++;

    value *= 10.0f;

    if(value < 1.0f) {
      putchar('0');
      continue;
    }

    curDigit = (s32)(Roundf((f32)(value - (10.0*floorf((f32)(value/10.0))))));
    //const int curDigit = ABS(value) % 10;

    // This if statement should nenver be true, but it sometimes is on the myriad1. This will output "BUG".
    if(value < 0.0f){
      putchar('B');
      putchar('U');
      putchar('G');
      putchar('4');
      return;
    }

    if(ABS(value) < 0.00000000001f){
      putchar('B');
      putchar('U');
      putchar('G');
      putchar('5');
      return;
    }

    if(curDigit < 0){
      putchar('B');
      putchar('U');
      putchar('G');
      putchar('6');
      return;
    }

    putchar(curDigit + 48);

    value = value - (f32)curDigit;
  }

  return;
} // void printFloat(f32 value)

void PrintInt(s32 value)
{
  int digits[MAX_PRINTF_DIGITS];

  s32 digitIndex = -1;

  for(digitIndex=0; digitIndex<MAX_PRINTF_DIGITS; digitIndex++) {
    digits[digitIndex] = 0;
  }

  if(value < 0) {
    putchar('-');
    value = -value;
  }

  if(value == 0) {
    putchar('0');
    return;
  }

  digitIndex=0;
  while(value > 0) {
    const s32 curDigit = (value - (10*(value/10)));
    //const int curDigit = ABS(value) % 10;

    // This if statement should never be true, but it sometimes is on the myriad1. This will output "BUG".
    if(value < 0){
      // BUG1
      digits[digitIndex++] = 1;
      digits[digitIndex++] = 23;
      digits[digitIndex++] = 37;
      digits[digitIndex++] = 18;
      break;
    }

    if(value == 0){
      // BUG2
      digits[digitIndex++] = 2;
      digits[digitIndex++] = 23;
      digits[digitIndex++] = 37;
      digits[digitIndex++] = 18;
      break;
    }

    if(curDigit < 0){
      // BUG3
      digits[digitIndex++] = 3;
      digits[digitIndex++] = 23;
      digits[digitIndex++] = 37;
      digits[digitIndex++] = 18;
      break;
    }

    digits[digitIndex++] = curDigit;

    value /= 10;
  }

  digitIndex--;
  for( ; digitIndex>=0; digitIndex--) {
    putchar(digits[digitIndex] + 48);
  }

  return;
} // void printInt(s32 value)

void PrintHex(u32 value)
{
  int digits[MAX_PRINTF_DIGITS];

  s32 digitIndex = -1;

  for(digitIndex=0; digitIndex<MAX_PRINTF_DIGITS; digitIndex++) {
    digits[digitIndex] = 0;
  }

  if(value == 0) {
    putchar('0');
    return;
  }

  digitIndex=0;
  while(value > 0) {
    const s32 curDigit = (value - (16*(value/16)));
    //const int curDigit = ABS(value) % 10;

    // This if statement should never be true, but it sometimes is on the myriad1. This will output "BUG".
    if(value < 0){
      // BUG1
      digits[digitIndex++] = 1;
      digits[digitIndex++] = 23;
      digits[digitIndex++] = 37;
      digits[digitIndex++] = 18;
      break;
    }

    if(value == 0){
      // BUG2
      digits[digitIndex++] = 2;
      digits[digitIndex++] = 23;
      digits[digitIndex++] = 37;
      digits[digitIndex++] = 18;
      break;
    }

    if(curDigit < 0){
      // BUG3
      digits[digitIndex++] = 3;
      digits[digitIndex++] = 23;
      digits[digitIndex++] = 37;
      digits[digitIndex++] = 18;
      break;
    }

    digits[digitIndex++] = curDigit;

    value /= 16;
  }

  digitIndex--;
  for( ; digitIndex>=0; digitIndex--) {
    const s32 curDigit = digits[digitIndex];
    if(curDigit < 10) {
      putchar(curDigit + 48);
    } else {
      putchar(curDigit + 55);
    }
  }

  return;
} // void printInt(s32 value)

IN_DDR double GetTime()
{
#if defined(_MSC_VER)
  f64 timeInSeconds;
  LARGE_INTEGER frequency, counter;
  QueryPerformanceCounter(&counter);
  QueryPerformanceFrequency(&frequency);
  timeInSeconds = (double)(counter.QuadPart)/(double)(frequency.QuadPart);
#elif defined(USING_MOVIDIUS_COMPILER)
  const f64 timeInSeconds = (1.0 / 1000.0) * DrvTimerTicksToMs(DrvTimerGetSysTicks64());
#elif defined(__APPLE_CC__)
  const f64 timeInSeconds = 0.0; // TODO: implement
#else // Generic Unix
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  const f64 timeInSeconds = (double)(ts.tv_sec) + (double)(ts.tv_nsec)/1000000000.0;
#endif

  return timeInSeconds;
}

f32 Roundf(const f32 number)
{
  if(number > 0)
    return floorf(number + 0.5f);
  else
    return floorf(number - 0.5f);
}

f64 Round(const f64 number)
{
  // This casting wierdness is because the myriad doesn't have a double-precision floor function.
  if(number > 0)
    return (f64)(floorf((f32)(number) + 0.5f));
  else
    return (f64)(floorf((f32)(number) - 0.5f));
}

IN_DDR s32 IsPowerOfTwo(u32 x)
{
  // While x is even and greater than 1, keep dividing by two
  while (((x & 1) == 0) && x > 1)
    x >>= 1;

  return (x == 1);
}

s32 IsOdd(const s32 x)
{
#if defined(USING_MOVIDIUS_GCC_COMPILER)
  if(((x>>1)<<1) == x)
    return 0;
  else
    return 1;
#else
  if(x | 1)
    return 1;
  else
    return 0;
#endif
}

s32 Determinant2x2(const s32 a, const s32 b, const s32 c, const s32 d)
{
  return a*d - b*c;
}

IN_DDR u32 Log2u32(u32 x)
{
  u32 powerCount = 0;
  // While x is even and greater than 1, keep dividing by two
  while (x >>= 1) {
    powerCount++;
  }

  return powerCount;
}

IN_DDR u64 Log2u64(u64 x)
{
  u64 powerCount = 0;
  // While x is even and greater than 1, keep dividing by two
  while (x >>= 1) {
    powerCount++;
  }

  return powerCount;
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