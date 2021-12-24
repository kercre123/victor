#ifndef __JAVA_COMMON_H__
#define __JAVA_COMMON_H__

#if !defined(__ppc__)
// to suppress warning from jni.h on OS X
#define TARGET_RT_MAC_CFM 0
#endif
#include <jni.h>

#include "converters.h"
#include "listconverters.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/java.hpp"

#ifdef _MSC_VER
#pragma warning(disable : 4800 4244)
#endif

#endif  //__JAVA_COMMON_H__
