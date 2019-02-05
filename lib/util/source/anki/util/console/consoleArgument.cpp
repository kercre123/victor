/*******************************************************************************************************************************
 *
 *  IConsoleFunctionArg
 *  Anki::Util
 *
 *  Created by Jarrod Hatfield on 4/24/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *******************************************************************************************************************************/


#include "util/console/consoleArgument.h"
#include "util/helpers/includeSstream.h"
#include <assert.h>
#include <cstring>

namespace Anki{ namespace Util {

static const std::string OPTIONAL_CHAR = "-";

//------------------------------------------------------------------------------------------------------------------------------
IConsoleFunctionArg::IConsoleFunctionArg( StringID name, bool optional ) :
  name_( name ),
  isOptional_( optional ),
  isSet_( false )
{
  
}

//------------------------------------------------------------------------------------------------------------------------------
template<typename T>
ConsoleArg<T>::ConsoleArg( StringID name, bool optional ) :
  IConsoleFunctionArg( name, optional )
{
  
}

//------------------------------------------------------------------------------------------------------------------------------
// Read the value in from a string and convert it to our arg type.
template<typename T>
bool ConsoleArg<T>::SetArgValue( const std::string& textvalue )
{
  // Allow optional arguments.
  if ( IsOptional() && ( textvalue == OPTIONAL_CHAR ) )
  {
    return true;
  }
  
  // We're relying on istringstream to do the conversion for us.
  std::istringstream is( textvalue );
  is.setf( std::ios::boolalpha );
  
  T temp;
  is >> temp;
  
  if ( !is.fail() )
  {
    value_ = temp;
    isSet_ = true;
    
    return true;
  }
  
  return false;
}

//------------------------------------------------------------------------------------------------------------------------------
// Specialization for a char/int8_t ... istringstream will try to read it as a character, instead of an 8-bit integer
template<>
bool ConsoleArg<int8_t>::SetArgValue( const std::string& textvalue )
{
  // Allow optional arguments.
  if ( IsOptional() && ( textvalue == OPTIONAL_CHAR ) )
  {
    return true;
  }
  
  // We're relying on istringstream to do the conversion for us.
  std::istringstream is( textvalue );
  
  int temp = 0;
  is >> temp;
  
  if ( !is.fail() )
  {
    value_ = temp;
    isSet_ = true;
    
    return true;
  }
  
  return false;
}

//------------------------------------------------------------------------------------------------------------------------------
// Specialization for unsigned char/uint8_t ... istringstream will try to read it as a character, instead of an 8-bit integer
template<>
bool ConsoleArg<uint8_t>::SetArgValue( const std::string& textvalue )
{
  // Allow optional arguments.
  if ( IsOptional() && ( textvalue == OPTIONAL_CHAR ) )
  {
    return true;
  }
  
  // We're relying on istringstream to do the conversion for us.
  std::istringstream is( textvalue );

  unsigned int temp = 0;
  is >> temp;
  
  if ( !is.fail() )
  {
    value_ = temp;
    isSet_ = true;
    
    return true;
  }
  
  return false;
}

//==============================================================================================================================
// Note: Console Manual Types (CONSOLE_INCLUDE_MANUAL_TYPES)
// If you created your own specialized template class, you'll need to define your specialized functions here ...

//------------------------------------------------------------------------------------------------------------------------------
// const char* specialization.
ConsoleArg<const char*>::ConsoleArg( StringID name, bool optional ) :
  IConsoleFunctionArg( name, optional )
{
  
}
  
//------------------------------------------------------------------------------------------------------------------------------
// const char* specialization
bool ConsoleArg<const char*>::SetArgValue( const std::string& textvalue )
{
  // Allow optional arguments.
  if ( IsOptional() && ( textvalue == OPTIONAL_CHAR ) )
  {
    return true;
  }
  
  strncpy( value_, textvalue.c_str(), 512 );
  isSet_ = true;
  return true;
}


//------------------------------------------------------------------------------------------------------------------------------
// Supported template specialized functions.
#define CONSOLE_ARG_TYPE( __argname__, __argtype__, ... ) \
  template<> ConsoleArgType ConsoleArg<__argtype__>::GetArgType() const { return ConsoleArgType_##__argname__; }
  
  #include "util/console/consoleSupportedTypes.def"
  
#undef CONSOLE_ARG_TYPE


//------------------------------------------------------------------------------------------------------------------------------
// Explicit template instantiation ... these are the only types that we accept as console arguments.

#define CONSOLE_ARG_TYPE( __argname__, __argtype__, ... ) \
  template class ConsoleArg<__argtype__>;
  
#include "util/console/consoleSupportedTypes.def"
  
#undef CONSOLE_ARG_TYPE


} // namespace Anki
} //namespace Util
