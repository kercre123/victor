/*******************************************************************************************************************************
 *
 *  ConsoleSystemAPI.cpp
 *
 *  Created by Jarrod Hatfield on 4/7/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *******************************************************************************************************************************/

#include "util/console/consoleSystemAPI.h"
#include "util/console/consoleSystem.h"

//============================================================================================================================
// Console Variable C-Interface

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleVar_Register_bool( const char* name, const char* category, bool *var )
{
  Anki::Util::ConsoleSystem::Instance().Register( *var, name, category );
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleVar_Register_int8_t( const char* name, const char* category, int8_t *var )
{
  Anki::Util::ConsoleSystem::Instance().Register( *var, name, category );
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleVar_Register_uint8_t( const char* name, const char* category, uint8_t *var )
{
  Anki::Util::ConsoleSystem::Instance().Register( *var, name, category );
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleVar_Register_int16_t( const char* name, const char* category, int16_t *var )
{
  Anki::Util::ConsoleSystem::Instance().Register( *var, name, category );
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleVar_Register_uint16_t( const char* name, const char* category, uint16_t *var )
{
  Anki::Util::ConsoleSystem::Instance().Register( *var, name, category );
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleVar_Register_int32_t( const char* name, const char* category, int32_t *var )
{
  Anki::Util::ConsoleSystem::Instance().Register( *var, name, category );
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleVar_Register_uint32_t( const char* name, const char* category, uint32_t *var )
{
  Anki::Util::ConsoleSystem::Instance().Register( *var, name, category );
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleVar_Register_int64_t( const char* name, const char* category, int64_t *var )
{
  Anki::Util::ConsoleSystem::Instance().Register( *var, name, category );
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleVar_Register_uint64_t( const char* name, const char* category, uint64_t *var )
{
  Anki::Util::ConsoleSystem::Instance().Register( *var, name, category );
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleVar_Register_float( const char* name, const char* category, float *var )
{
  Anki::Util::ConsoleSystem::Instance().Register( *var, name, category );
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleVar_Register_double( const char* name, const char* category, double *var )
{
  Anki::Util::ConsoleSystem::Instance().Register( *var, name, category );
}


//============================================================================================================================
// Console Function C-Interface


//------------------------------------------------------------------------------------------------------------------------------
void ConsoleFunction_Register( const char* name, ConsoleFunc function, const char* categoryName, const char* args )
{
  Anki::Util::ConsoleSystem::Instance().Register( name, function, categoryName, args );
}


//------------------------------------------------------------------------------------------------------------------------------
#define CONSOLE_INCLUDE_MANUAL_TYPES
#define CONSOLE_ARG_TYPE( __argname__, __argtype__, ... ) \
  __argtype__ ConsoleArg_Get_##__argname__( ConsoleFunctionContextRef context, const char* varname ) \
  { \
    __argtype__ result = (__argtype__)0; \
    Anki::Util::ConsoleSystem::Instance().GetArgumentValue( *context, varname, result ); \
    return result; \
  } \
  \
  __argtype__ ConsoleArg_GetOptional_##__argname__( ConsoleFunctionContextRef context, const char* varname, __argtype__ defaultvalue ) \
  { \
    __argtype__ result = defaultvalue; \
    Anki::Util::ConsoleSystem::Instance().GetArgumentValue( *context, varname, result ); \
    return result; \
  }

  // This will define all of our functions for every supported type we have.
  #include "util/console/consoleSupportedTypes.def"

#undef CONSOLE_ARG_TYPE
#undef CONSOLE_INCLUDE_MANUAL_TYPES

// Add any specific type support here that can't be handled with macro magic ...

//------------------------------------------------------------------------------------------------------------------------------
char ConsoleArg_Get_Char( ConsoleFunctionContextRef context, const char* varname )
{
  char result = 0;
  Anki::Util::ConsoleSystem::Instance().GetArgumentValue( *context, varname, result );
  return result;
}

//------------------------------------------------------------------------------------------------------------------------------
char ConsoleArg_GetOptional_Char( ConsoleFunctionContextRef context, const char* varname, char defaultvalue )
{
  char result = defaultvalue;
  Anki::Util::ConsoleSystem::Instance().GetArgumentValue( *context, varname, result );
  return result;
}

//------------------------------------------------------------------------------------------------------------------------------
unsigned char ConsoleArg_Get_UChar( ConsoleFunctionContextRef context, const char* varname )
{
  unsigned char result = 0;
  Anki::Util::ConsoleSystem::Instance().GetArgumentValue( *context, varname, result );
  return result;
}

//------------------------------------------------------------------------------------------------------------------------------
unsigned char ConsoleArg_GetOptional_UChar( ConsoleFunctionContextRef context, const char* varname, unsigned char defaultvalue )
{
  unsigned char result = defaultvalue;
  Anki::Util::ConsoleSystem::Instance().GetArgumentValue( *context, varname, result );
  return result;
}


//------------------------------------------------------------------------------------------------------------------------------
// These are just wrappers that call into the already defined functions above.

//------------------------------------------------------------------------------------------------------------------------------
short ConsoleArg_Get_Short( ConsoleFunctionContextRef context, const char* varname )
{
  return ConsoleArg_Get_Int16( context, varname );
}

//------------------------------------------------------------------------------------------------------------------------------
short ConsoleArg_GetOptional_Short( ConsoleFunctionContextRef context, const char* varname, short defaultvalue )
{
  return ConsoleArg_GetOptional_Int16( context, varname, defaultvalue );
}

//------------------------------------------------------------------------------------------------------------------------------
unsigned short ConsoleArg_Get_UShort( ConsoleFunctionContextRef context, const char* varname )
{
  return ConsoleArg_Get_UInt16( context, varname );
}

//------------------------------------------------------------------------------------------------------------------------------
unsigned short ConsoleArg_GetOptional_UShort( ConsoleFunctionContextRef context, const char* varname, unsigned short defaultvalue )
{
  return ConsoleArg_GetOptional_UInt16( context, varname, defaultvalue );
}

//------------------------------------------------------------------------------------------------------------------------------
int ConsoleArg_Get_Int( ConsoleFunctionContextRef context, const char* varname )
{
  return ConsoleArg_Get_Int32( context, varname );
}

//------------------------------------------------------------------------------------------------------------------------------
int ConsoleArg_GetOptional_Int( ConsoleFunctionContextRef context, const char* varname, int defaultvalue )
{
  return ConsoleArg_GetOptional_Int32( context, varname, defaultvalue );
}

//------------------------------------------------------------------------------------------------------------------------------
unsigned int ConsoleArg_Get_UInt( ConsoleFunctionContextRef context, const char* varname )
{
  return ConsoleArg_Get_UInt32( context, varname );
}

//------------------------------------------------------------------------------------------------------------------------------
unsigned int ConsoleArg_GetOptional_UInt( ConsoleFunctionContextRef context, const char* varname, unsigned int defaultvalue )
{
  return ConsoleArg_GetOptional_UInt32( context, varname, defaultvalue );
}

