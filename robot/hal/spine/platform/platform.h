#ifndef SPINE_PLATFORM_H
#define SPINE_PLATFORM_H

#ifdef ANDROID
#define  PLATFORM_ANDROID 1
#else
#define  PLATFORM_OSX 1
#endif

/* Platform selector. **
 * -
 * Makefile must define exactly one of
 *
 * PLATFORM_ANDRIOD
 * PLATFORM_OSX
 * PLATFORM_SIM ?
 */

#if PLATFORM_ANDROID
#include "android/spine_logging.h"
#include "android/serial_support.h"
#include <unistd.h>
#include <termios.h>

#define SPINE_TTY "/dev/ttyHS0"
#define SPINE_TTY_LEGACY "/dev/ttyHSL1"
#define SPINE_BAUD B3000000

#define static_assert _Static_assert


#elif PLATFORM_OSX
#include "osx/spine_logging.h"
#include "osx/serial_support.h"

#define SPINE_TTY "/dev/ttyUSB_LINK"
#define SPINE_BAUD B230400


#elif PLATFORM_SIM
#include "platform/simulator/hal_logging.h"

#endif

#endif//SPINE_PLATFORM_H
