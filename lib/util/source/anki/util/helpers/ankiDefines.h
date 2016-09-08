/**
 * File: ankiDefines.h
 *
 * Author: raul
 * Created: 04/08/15
 *
 * Description: Top level macros/defines for other parts of the code to rely on. This is not a place to dump all
 *              conditional macros to enable prints, debugs, etc. This is more of a top level define for platforms, 
 *              or other compilation flags.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __Anki_Defines_H__
#define __Anki_Defines_H__

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Platform
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if defined(__APPLE__) && defined(__MACH__)
  // Apple OSX and iOS (Darwin)
  #include <TargetConditionals.h>
  #if (TARGET_IPHONE_SIMULATOR == 1) || (TARGET_OS_IPHONE == 1)

    #define ANKI_PLATFORM_IOS 1

  #elif (TARGET_OS_MAC == 1)

  #define ANKI_PLATFORM_OSX 1

  #endif

#elif defined(ANDROID)
  #define ANKI_PLATFORM_ANDROID 1

#elif defined(LINUX)
  #define ANKI_PLATFORM_LINUX 1

#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// JNI
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Should not need this. Platforms specific code is not shared
// #define ANKI_USE_JNI !(defined(ANKI_PLATFORM_IOS) || ( defined(STANDALONE) && STANDALONE ))

#endif // header
