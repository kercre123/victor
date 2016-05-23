/*******************************************************************************************************************************
 *
 *  ConsoleSystemAPI.h
 *
 *  Created by Jarrod Hatfield on 4/7/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *  Description:
 *  - Defines the 'c-api' required to use the console system.
 *  - Separated from ConsoleSystem.h for readability.
 *
 *******************************************************************************************************************************/

#ifndef CONSOLE_SYSTEM_C_API
#define CONSOLE_SYSTEM_C_API

#include "util/console/consoleTypes.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

  //============================================================================================================================
  // Console Variable C-Interface
  
  void ConsoleVar_Register_bool( const char* name, const char* category, bool *var );

  void ConsoleVar_Register_int8_t( const char* name, const char* category, int8_t *var );
  void ConsoleVar_Register_uint8_t( const char* name, const char* category, uint8_t *var );

  void ConsoleVar_Register_int16_t( const char* name, const char* category, int16_t *var );
  void ConsoleVar_Register_uint16_t( const char* name, const char* category, uint16_t *var );

  void ConsoleVar_Register_int32_t( const char* name, const char* category, int32_t *var );
  void ConsoleVar_Register_uint32_t( const char* name, const char* category, uint32_t *var );

  void ConsoleVar_Register_int64_t( const char* name, const char* category, int64_t *var );
  void ConsoleVar_Register_uint64_t( const char* name, const char* category, uint64_t *var );

  void ConsoleVar_Register_float( const char* name, const char* category, float *var );
  void ConsoleVar_Register_double( const char* name, const char* category, double *var );

  //============================================================================================================================
  // Console Function C-Interface
  
  void ConsoleFunction_Register( const char* name, ConsoleFunc function, const char* categoryName, const char* args );
  
  // These are the functions we define for each supported type.
  #define CONSOLE_INCLUDE_MANUAL_TYPES
  #define CONSOLE_ARG_TYPE( __argname__, __argtype__, ... ) \
    __argtype__ ConsoleArg_Get_##__argname__( ConsoleFunctionContextRef context, const char* varname ); \
    __argtype__ ConsoleArg_GetOptional_##__argname__( ConsoleFunctionContextRef context, const char* varname, __argtype__ defaultvalue );
  
    #include "util/console/consoleSupportedTypes.def"
  
  #undef CONSOLE_ARG_TYPE
  #undef CONSOLE_INCLUDE_MANUAL_TYPES
  
  // Add any specific type support here that can't be handled with macro magic ...
  
  char ConsoleArg_Get_Char( ConsoleFunctionContextRef context, const char* varname );
  char ConsoleArg_GetOptional_Char( ConsoleFunctionContextRef context, const char* varname, char defaultvalue );
  
  unsigned char ConsoleArg_Get_UChar( ConsoleFunctionContextRef context, const char* varname );
  unsigned char ConsoleArg_GetOptional_UChar( ConsoleFunctionContextRef context, const char* varname, unsigned char defaultvalue );
  
  // Wrapper functions that just call into our supported type functions.
  short ConsoleArg_Get_Short( ConsoleFunctionContextRef context, const char* varname );
  short ConsoleArg_GetOptional_Short( ConsoleFunctionContextRef context, const char* varname, short defaultvalue );
  
  unsigned short ConsoleArg_Get_UShort( ConsoleFunctionContextRef context, const char* varname );
  unsigned short ConsoleArg_GetOptional_UShort( ConsoleFunctionContextRef context, const char* varname, unsigned short defaultvalue );
  
  int ConsoleArg_Get_Int( ConsoleFunctionContextRef context, const char* varname );
  int ConsoleArg_GetOptional_Int( ConsoleFunctionContextRef context, const char* varname, int defaultvalue );
  
  unsigned int ConsoleArg_Get_UInt( ConsoleFunctionContextRef context, const char* varname );
  unsigned int ConsoleArg_GetOptional_UInt( ConsoleFunctionContextRef context, const char* varname, unsigned int defaultvalue );
  
  // Note:  In order to support unique argument type for our console functions, there is a somewhat complicated system that
  //        parses strings to identify all of the types.  This system spans across multiple files, classes and functions.  In
  //        order to make adding/removing supported types simple, I'm using macros in all of the places we need to do something
  //        over all of our supported types.  The alternative is forcing you to have to make changes in multiple places every time
  //        you want to add/remove type support, which will probably lead to more mistakes than the marcos.


#ifdef __cplusplus
}
#endif

#endif // CONSOLE_SYSTEM_C_API
