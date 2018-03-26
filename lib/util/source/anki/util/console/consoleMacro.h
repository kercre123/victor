/*******************************************************************************************************************************
 *
 *  ConsoleMacro.h
 *
 *  Created by Raul Sampedro on 7/9/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *  Description:
 *  - Defines macro to enable remote console code.
 *
 *******************************************************************************************************************************/

#ifndef CONSOLE_MACRO_H
#define CONSOLE_MACRO_H

#ifndef REMOTE_CONSOLE_ENABLED
  #if defined(NDEBUG)
    #define REMOTE_CONSOLE_ENABLED 1 // also enabled for release builds
  #else
    #define REMOTE_CONSOLE_ENABLED 1 // enabled for debug builds
  #endif
#endif

#if REMOTE_CONSOLE_ENABLED
  #define REMOTE_CONSOLE_ENABLED_ONLY( expr )   expr
#else
  #define REMOTE_CONSOLE_ENABLED_ONLY( expr )
#endif

#endif // CONSOLE_MACRO_H
