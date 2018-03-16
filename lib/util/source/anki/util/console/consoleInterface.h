/*******************************************************************************************************************************
 *
 *  ConsoleInterface.h
 *
 *  Created by Jarrod Hatfield on 4/7/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *  Description:
 *  - Defines the 'api' required to use the console system.
 *
 *******************************************************************************************************************************/

#ifndef CONSOLE_INTERFACE_H
#define CONSOLE_INTERFACE_H

#include "util/console/consoleMacro.h"
#include "util/console/consoleTypes.h"
#include "util/console/consoleSystemAPI.h"
#include "util/console/consoleChannel.h"

//==============================================================================================================================
// C++ Console Variable Interface
#ifdef __cplusplus

#include "util/console/consoleSystem.h"


#if REMOTE_CONSOLE_ENABLED


  #define CONSOLE_VAR_EXTERN( type, name ) \
    extern type name;

  #define CONSOLE_VAR( type, name, path, default ) \
    type name = default; \
    namespace { \
      static Anki::Util::ConsoleVar<type> cvar_##name( name, #name, path, false ); \
    }

  #define WRAP_CONSOLE_VAR( type, name, path ) \
    namespace { \
      static Anki::Util::ConsoleVar<type> cvar_##name( name, #name, path, false ); \
    }

  #define WRAP_EXTERN_CONSOLE_VAR( type, name, path ) \
    extern type name; \
    namespace { \
      static Anki::Util::ConsoleVar<type> cvar_##name( name, #name, path, false ); \
    }

  #define CONSOLE_VAR_RANGED( type, name, path, default, minVal, maxVal ) \
    type name = default; \
    namespace { \
      static Anki::Util::ConsoleVar<type> cvar_##name( name, #name, path, minVal, maxVal, false ); \
    }

  #define CONSOLE_VAR_ENUM( type, name, path, default, values ) \
    type name = default; \
    namespace { \
      static Anki::Util::ConsoleVar<type> cvar_##name( name, #name, path, values, false ); \
    }

  #define CONSOLE_FUNC( name, path, args... ) \
    namespace { \
      Anki::Util::IConsoleFunction cfunc_##name( #name, &name, path, #args ); \
    }


#else


  #define CONSOLE_VAR_EXTERN( type, name ) \
    extern const type name;

  #define CONSOLE_VAR( type, name, path, default ) \
    extern const type name = default;

  #define WRAP_CONSOLE_VAR( type, name, path )

  #define WRAP_EXTERN_CONSOLE_VAR( type, name, path )

  #define CONSOLE_VAR_RANGED( type, name, path, default, minVal, maxVal ) \
    extern const type name = default;

  #define CONSOLE_VAR_ENUM( type, name, path, default, values ) \
    extern const type name = default;

  #define CONSOLE_FUNC( ... )

#endif // REMOTE_CONSOLE_ENABLED


//==============================================================================================================================
// C Console Variable Interface
#else

#if REMOTE_CONSOLE_ENABLED


  #define CONSOLE_VAR_EXTERN( type, name ) \
    extern type name;

  #define CONSOLE_VAR( type, name, default ) \
    type name = default;

  #define CONSOLE_VAR_REGISTER( type, name, path ) \
    ConsoleVar_Register_##type( #name, #path, &name );

  #define CONSOLE_FUNC_REGISTER( name, path, args... ) \
    ConsoleFunction_Register( #name, name, path, #args );

#else


  #define CONSOLE_VAR_EXTERN( type, name ) \
    extern const type name;

  #define CONSOLE_VAR( type, name, default ) \
    extern const type name = default;

  #define CONSOLE_VAR_REGISTER( ... )

  #define CONSOLE_FUNC_REGISTER( name, path, args... )

#endif // REMOTE_CONSOLE_ENABLED


#endif // __cplusplus


#endif // CONSOLE_INTERFACE_H
