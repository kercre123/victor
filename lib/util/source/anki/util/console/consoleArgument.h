/*******************************************************************************************************************************
 *
 *  IConsoleFunctionArg
 *  Anki::Util
 *
 *  Created by Jarrod Hatfield on 4/24/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *  Description:
 *  - Templated arguments for use with a ConsoleFunction
 *
 *******************************************************************************************************************************/

#ifndef ANKIUTIL_CONSOLE_ARGUMENT
#define ANKIUTIL_CONSOLE_ARGUMENT

#include "util/stringTable/stringID.h"
#include <stdint.h>

namespace Anki{ namespace Util {

//----------------------------------------------------------------------------------------------------------------------------
// All of our accepted types
#define CONSOLE_INCLUDE_MANUAL_TYPES
#define CONSOLE_ARG_TYPE( __argname__, __argtype__, ... ) \
  ConsoleArgType_##__argname__,
  
typedef enum
{
  // Read these in from this file ...
  #include "util/console/consoleSupportedTypes.def"

  ConsoleArgType_Num,
} ConsoleArgType;

#undef CONSOLE_ARG_TYPE
#undef CONSOLE_INCLUDE_MANUAL_TYPES

//******************************************************************************************************************************
class IConsoleFunctionArg
{
public:
  IConsoleFunctionArg( StringID name, bool optional );
  virtual ~IConsoleFunctionArg() { }
  
  virtual void ClearArg() { isSet_ = false; }
  bool IsSet() const { return isSet_; }
  
  StringID GetArgName() const { return name_; }
  bool IsOptional() const { return isOptional_; }
  
  template<typename T>
  bool GetArgumentValue( T& value ) const;
  
  // To be implemented by template specialized classes
  virtual bool SetArgValue( const std::string& textvalue ) = 0;
  virtual ConsoleArgType GetArgType() const = 0;
  
  //----------------------------------------------------------------------------------------------------------------------------
  // Member data
protected:
  StringID name_;
  bool isOptional_;
  bool isSet_;
  
private:
  IConsoleFunctionArg();
  IConsoleFunctionArg( const IConsoleFunctionArg& );
};


//******************************************************************************************************************************
template<typename T>
class ConsoleArg : public IConsoleFunctionArg
{
public:
  ConsoleArg( StringID name, bool optional );
  
  template<typename U>
  bool GetArgValueAsType( U& value ) const;
  
  virtual bool SetArgValue( const std::string& textvalue );
  virtual ConsoleArgType GetArgType() const;
  
  //----------------------------------------------------------------------------------------------------------------------------
  // Member data
private:
  T value_;
  
  ConsoleArg();
  ConsoleArg( const ConsoleArg& );
};

//******************************************************************************************************************************
// char* specialization
template<>
class ConsoleArg<const char*> : public IConsoleFunctionArg
{
public:
  ConsoleArg( StringID name, bool optional );
  
  template<typename U>
  bool GetArgValueAsType( U& value ) const;
  
  virtual bool SetArgValue( const std::string& textvalue );
  virtual ConsoleArgType GetArgType() const { return ConsoleArgType_String; }
  
  //----------------------------------------------------------------------------------------------------------------------------
  // Member data
private:
  char value_[512];
  
  ConsoleArg();
  ConsoleArg( const ConsoleArg& );
};

// Include our templated definitions here ...
#include "util/console/consoleArgument_priv.h"

} // namespace Anki
} //namespace Util

#endif // ANKIUTIL_CONSOLE_ARGUMENT
