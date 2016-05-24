/*******************************************************************************************************************************
 *
 *  consoleTypes.h
 *
 *  Created by Jarrod Hatfield on 4/24/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *  Description:
 *  - Defines the types required by the console system api so that we can include them from multiple sources without including
 *    everything else that goes with those files.
 *
 *******************************************************************************************************************************/

#ifndef CONSOLE_TYPES_H
#define CONSOLE_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct ConsoleFunctionContext* ConsoleFunctionContextRef;
  typedef void (*ConsoleFunc)( ConsoleFunctionContextRef );
  
#ifdef __cplusplus
  namespace Anki{ namespace Util {
    class IConsoleChannel;
  } }
  typedef Anki::Util::IConsoleChannel * ConsoleChannelRef;
#else
  typedef struct IConsoleChannel * ConsoleChannelRef;
#endif

  //----------------------------------------------------------------------------------------------------------------------------
  // Data passed into a console function that stores all required information to carry out all console function needs.
  struct ConsoleFunctionContext
  {
    const char* function;   // Name of the function that we're calling (note: this is just for the system to look up the function)
    const char* arguments;  // String representing the input from the console (note: use the ConsoleArg helper functions)
    ConsoleChannelRef channel;
  };


#ifdef __cplusplus
}
#endif

#endif // CONSOLE_TYPES_H
