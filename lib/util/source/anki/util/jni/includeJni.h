//
//  includeJni.h
//  native
//
//  Created by damjan stulic on 5/26/14.
//  Copyright (c) 2014 anki. All rights reserved.
//

#ifndef native_includeJni_h
#define native_includeJni_h

#if defined(ANDROID)
#include <jni.h>
#include <assert.h>
#define JNI_CHECK(__jniEnv) assert(nullptr != (__jniEnv))
#elif defined(__APPLE__) || defined(LINUX)
#include <JavaVM/jni.h>
#define JNI_CHECK(__jniEnv) ({ if (nullptr == (__jniEnv)) return; })
//#include "fakeJNI.h"
#endif

#endif // native_includeJni_h
