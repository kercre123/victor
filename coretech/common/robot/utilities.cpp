/**
File: utilities.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/
#include <assert.h>
#include "coretech/common/robot/utilities.h"
#include "coretech/common/robot/errorHandling.h"
#include "coretech/common/robot/array2d.h"
#include "util/helpers/ankiDefines.h"
#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

#include <cstdlib>

#if defined(_MSC_VER)
#include <windows.h>
#elif defined(__EDG__)
#include "anki/cozmo/robot/hal.h"
#else
#if defined(__ARM_ARCH_7A__)
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <sys/resource.h>
#include <unistd.h>
#endif

#define PrintfOneArray_FORMAT_STRING_2 "%d %d "
#define PrintfOneArray_FORMAT_STRING_1 "%d "

namespace Anki
{
  namespace Embedded
  {
#if ANKICORETECH_EMBEDDED_USE_OPENCV
    int ConvertToOpenCvType(const char *typeName, size_t byteDepth)
    {
      if(typeName[0] == 'u') { //unsigned
        if(byteDepth == 1) {
          return CV_8U;
        } else if(byteDepth == 2) {
          return CV_16U;
        }
      } else if(typeName[0] == 'f' && byteDepth == 4) { //float
        return CV_32F;
      } else if(typeName[0] == 'd' && byteDepth == 8) { //double
        return CV_64F;
      } else { // signed
        if(byteDepth == 1) {
          return CV_8S;
        } else if(byteDepth == 2) {
          return CV_16S;
        } else if(byteDepth == 4) {
          return CV_32S;
        }
      }

      return -1;
    }
#endif //#if ANKICORETECH_EMBEDDED_USE_OPENCV

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    void CvPutTextFixedWidth(
      cv::Mat& img, const char * text, cv::Point org,
      int fontFace, double fontScale, cv::Scalar color,
      int thickness, int lineType,
      bool bottomLeftOrigin)
    {
      static int last_fontFace = -1;
      static double last_fontScale = -1.0;
      static int last_thickness = -1;
      static int last_lineType = -1;
      static int maxWidth = -1;

      // If the font has changed since last call, compute the new font width
      if(last_fontFace != fontFace || last_fontScale != fontScale || last_thickness != thickness || last_lineType != lineType) {
        last_fontFace = fontFace;
        last_fontScale = fontScale;
        last_thickness = thickness;
        last_lineType = lineType;

        /*// All characters
        const s32 numCharacters = 93;
        const char characters[numCharacters] = "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789~!@#$%^&*()_+-=[]{},.<>/?;':\"";*/

        // These are generally the largest characters
        const s32 numCharacters = 12;
        const char characters[numCharacters] = "mwMW@%^&+-=";

        maxWidth = 0;
        for(s32 i=0; i<(numCharacters-1); i++) {
          char tmpBuffer[2];
          tmpBuffer[0] = characters[i];
          tmpBuffer[1] = '\0';

          cv::Size textSize = cv::getTextSize(std::string(tmpBuffer), fontFace, fontScale, thickness, NULL);
          //CoreTechPrint("%c = %dx%d\n", characters[i], textSize.width, textSize.height);
          maxWidth = MAX(maxWidth, textSize.width);
        }
      }

      const s32 stringLength = static_cast<s32>(strlen(text));

      cv::Point curOrg = org;
      for(s32 iChar=0; iChar<stringLength; iChar++) {
        char tmpBuffer[2];
        tmpBuffer[0] = text[iChar];
        tmpBuffer[1] = '\0';

        cv::putText(img, std::string(tmpBuffer), curOrg, fontFace, fontScale, color, thickness, lineType, bottomLeftOrigin);

        curOrg.x += maxWidth - 1;
      }
    }

    void CvPutTextWithBackground(
      cv::Mat& img, const char * text, cv::Point org,
      int fontFace, double fontScale, cv::Scalar textColor, cv::Scalar backgroundColor,
      int thickness, int lineType, bool orgIsCenter)
    {
      // Based on example at http://docs.opencv.org/modules/core/doc/drawing_functions.html

      int baseline = 0;
      const cv::Size textSize = cv::getTextSize(text, fontFace, fontScale, thickness, &baseline);
      baseline += thickness;

      cv::Point rectanglePoint1  = org;
      cv::Point rectanglePoint2  = org + cv::Point(textSize.width, textSize.height);
      cv::Point textPoint = org + cv::Point(0, textSize.height);

      if(orgIsCenter) {
        const cv::Point textSize2(textSize.width / 2, textSize.height / 2);
        rectanglePoint1 -= textSize2;
        rectanglePoint2 -= textSize2;
        textPoint -= textSize2;
      }

      // draw the box
      cv::rectangle(img, rectanglePoint1, rectanglePoint2, backgroundColor, -1);

      // then put the text itself
      cv::putText(img, text, textPoint, fontFace, fontScale, textColor, thickness, lineType);
    }
#endif //#if ANKICORETECH_EMBEDDED_USE_OPENCV

    s32 FindBytePattern(const void * restrict buffer, const s32 bufferLength, const u8 * restrict bytePattern, const s32 bytePatternLength)
    {
      s32 curIndex = 0;
      s32 numBytesInPatternFound = 0;

      // Check if the bytePattern is valid
#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS
      for(s32 i=0; i<bytePatternLength; i++) {
        for(s32 j=0; j<bytePatternLength; j++) {
          if(i == j)
            continue;

          if(bytePattern[i] == bytePattern[j]) {
            AnkiError("FindBytePattern", "bytePattern is not unique");
            return -1;
          }
        }
      }
#endif

      const u8 * restrict bufferU8 = reinterpret_cast<const u8*>(buffer);

      while(curIndex < bufferLength) {
        if(bufferU8[curIndex] == bytePattern[numBytesInPatternFound]) {
          numBytesInPatternFound++;
        } else if(bufferU8[curIndex] == bytePattern[0]) {
          numBytesInPatternFound = 1;
        } else {
          numBytesInPatternFound = 0;
        }

        curIndex++;

        if(numBytesInPatternFound == bytePatternLength) {
          return curIndex - bytePatternLength;
        }
      } // while(curIndex < bufferLength)

      return -1;
    }

    s32 RandS32(const s32 minLimit, const s32 maxLimit)
    {
      const s32 num = (rand() % (maxLimit - minLimit + 1)) + minLimit;

      return num;
    }

    f32 GetTimeF32()
    {
#if defined(_MSC_VER)
      f32 timeInSeconds;

      static f32 frequency = 0;
      static LONGLONG startCounter = 0;

      LARGE_INTEGER counter;

      if(frequency == 0) {
        LARGE_INTEGER frequencyTmp;
        QueryPerformanceFrequency(&frequencyTmp);
        frequency = (f32)frequencyTmp.QuadPart;
      }

      QueryPerformanceCounter(&counter);

      // Subtract startSeconds, so the floating point number has reasonable precision
      if(startCounter == 0) {
        startCounter = counter.QuadPart;
      }

      timeInSeconds = (f32)(counter.QuadPart - startCounter) / frequency;
#elif defined(__APPLE_CC__)
      struct timeval time;
#     if !defined(ANKI_PLATFORM_IOS)
      // TODO: Fix build error when using this in an iOS build for arm architectures
      gettimeofday(&time, NULL);
#     endif
      // Subtract startSeconds, so the floating point number has reasonable precision
      static long startSeconds = 0;
      if(startSeconds == 0) {
        startSeconds = time.tv_sec;
      }

      const f32 timeInSeconds = (f32)(time.tv_sec-startSeconds) + ((f32)time.tv_usec / 1000000.0f);
#elif defined(__EDG__)  // ARM-MDK
      const f32 timeInSeconds = Anki::Vector::HAL::GetMicroCounter() / 1000000.0f;
#else // Generic Unix
      timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      const f32 timeInSeconds = (f32)(ts.tv_sec) + (f32)(ts.tv_nsec)/1000000000.0f;
#endif

      return timeInSeconds;
    } // f32 GetTimeF32()

    f64 GetTimeF64()
    {
#if defined(_MSC_VER)
      f64 timeInSeconds;

      static f64 frequency = 0;
      static LONGLONG startCounter = 0;

      LARGE_INTEGER counter;

      if(frequency == 0) {
        LARGE_INTEGER frequencyTmp;
        QueryPerformanceFrequency(&frequencyTmp);
        frequency = (f64)frequencyTmp.QuadPart;
      }

      QueryPerformanceCounter(&counter);

      // Subtract startSeconds, so the floating point number has reasonable precision
      if(startCounter == 0) {
        startCounter = counter.QuadPart;
      }

      timeInSeconds = (f64)(counter.QuadPart - startCounter) / frequency;
#elif defined(__APPLE_CC__)
      struct timeval time;
#     if !defined(ANKI_PLATFORM_IOS)
      // TODO: Fix build error when using this in an iOS build for arm architectures
      gettimeofday(&time, NULL);
#     endif

      // Subtract startSeconds, so the floating point number has reasonable precision
      static long startSeconds = 0;
      if(startSeconds == 0) {
        startSeconds = time.tv_sec;
      }

      const f64 timeInSeconds = (f64)(time.tv_sec-startSeconds) + ((f64)time.tv_usec / 1000000.0);
#elif defined(__EDG__)  // ARM-MDK
      const f64 timeInSeconds = Anki::Vector::HAL::GetMicroCounter() * (1.0 / 1000000.0);
#else // Generic Unix
      timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      const f64 timeInSeconds = (f64)(ts.tv_sec) + (f64)(ts.tv_nsec) * (1.0 / 1000000000.0);
#endif

      return timeInSeconds;
    } // f64 GetTimeF64()

    u32 GetTimeU32()
    {
#if defined(_MSC_VER)
      static LONGLONG startCounter = 0;

      LARGE_INTEGER counter;

      QueryPerformanceCounter(&counter);

      // Subtract startCounter, so the floating point number has reasonable precision
      if(startCounter == 0) {
        startCounter = counter.QuadPart;
      }

      return static_cast<u32>((counter.QuadPart - startCounter) & 0xFFFFFFFF);
#elif defined(__APPLE_CC__)
      struct timeval time;
#     if !defined(ANKI_PLATFORM_IOS)
      // TODO: Fix build error when using this in an iOS build for arm architectures
      gettimeofday(&time, NULL);
#     endif

      // Subtract startSeconds, so the floating point number has reasonable precision
      static long startSeconds = 0;
      if(startSeconds == 0) {
        startSeconds = time.tv_sec;
      }

      return (u32)(time.tv_sec-startSeconds)*1000000 + (u32)time.tv_usec;
#elif defined (__EDG__)  // MDK-ARM
      return Anki::Vector::HAL::GetMicroCounter();
#else
      timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return (u32)ts.tv_sec * 1000000 + (u32)(ts.tv_nsec/1000);
#endif
    } // u32 GetTimeU32()

#if defined(_MSC_VER)
    // returns secondTime - firstTime
    static u64 FiletimeDelta(const FILETIME &secondTime, const FILETIME &firstTime)
    {
      LARGE_INTEGER firstTimeI;
      LARGE_INTEGER secondTimeI;

      firstTimeI.LowPart = firstTime.dwLowDateTime;
      firstTimeI.HighPart = firstTime.dwHighDateTime;

      secondTimeI.LowPart = secondTime.dwLowDateTime;
      secondTimeI.HighPart = secondTime.dwHighDateTime;

      return secondTimeI.QuadPart - firstTimeI.QuadPart;
    }
#endif

    f32 GetCpuUsage()
    {
      static bool firstCall = true;

      f64 percentUsage;

#if defined(_MSC_VER)

      FILETIME idleTime;
      FILETIME kernelTime;
      FILETIME userTime;
      GetSystemTimes(&idleTime, &kernelTime, &userTime);

      static FILETIME lastIdleTime;
      static FILETIME lastKernelTime;
      static FILETIME lastUserTime;
      // static s32 numCpus;

      if(firstCall) {
        // SYSTEM_INFO sysinfo;
        // GetSystemInfo( &sysinfo );
        // numCpus = sysinfo.dwNumberOfProcessors;

        percentUsage = 0;
        firstCall = false;
      } else {
        const u64 idleDelta = FiletimeDelta(idleTime, lastIdleTime);
        const u64 kernelDelta = FiletimeDelta(kernelTime, lastKernelTime);
        const u64 userDelta = FiletimeDelta(userTime, lastUserTime);

        const u64 totalSystemDelta = kernelDelta + userDelta;

        //percentUsage = 100.0f * static_cast<f32>(numCpus) * static_cast<f32>(totalSystemDelta - idleDelta) / static_cast<f32>(totalSystemDelta);
        percentUsage = 100.0 * static_cast<f64>(totalSystemDelta - idleDelta) / static_cast<f64>(totalSystemDelta);
      }

      lastIdleTime = idleTime;
      lastKernelTime = kernelTime;
      lastUserTime = userTime;

#elif defined(__EDG__)  // ARM-MDK
      // Cannot query on the M4
      percentUsage = 0;
#else // Generic Unix or OSX

      const f64 numCpus = sysconf( _SC_NPROCESSORS_ONLN );

      f64 curCpuTime;
      f64 curTime;

      static f64 lastCpuTime;
      static f64 lastTime;

      struct rusage usage;
      getrusage(RUSAGE_SELF, &usage);

      curTime = GetTimeF64();

      // User + system (kernel) time
      curCpuTime =
        static_cast<f64>(usage.ru_utime.tv_sec) + static_cast<f64>(usage.ru_utime.tv_usec) / 1000000.0 +
        static_cast<f64>(usage.ru_stime.tv_sec) + static_cast<f64>(usage.ru_stime.tv_usec) / 1000000.0;

      if(firstCall) {
        percentUsage = 0;
        firstCall = false;
      } else {
        // TODO: Check that everything works, with the number of cores and everything
        percentUsage = 100.0 * (curCpuTime - lastCpuTime) / (curTime - lastTime) / numCpus;
      }

      lastCpuTime = curCpuTime;
      lastTime = curTime;
#endif

      return static_cast<f32>(percentUsage);
    } // f32 GetCpuUsage()
  } // namespace Embedded
} // namespace Anki
