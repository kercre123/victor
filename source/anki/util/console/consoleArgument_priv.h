/*******************************************************************************************************************************
 *
 *  IConsoleFunctionArg
 *  Anki::Util
 *
 *  Created by Jarrod Hatfield on 4/24/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *  Description:
 *  - Implements the private details of templated functions for the console arguments.
 *  - Templates need to be visible to the current translation unit, so we're putting these definitions here to decouple them
 *    from the header file.
 *
 *******************************************************************************************************************************/

#ifndef ANKIUTIL_CONSOLE_ARGUMENT_PRIV
#define ANKIUTIL_CONSOLE_ARGUMENT_PRIV


//------------------------------------------------------------------------------------------------------------------------------
// By deafult, our value is implicitly casted to our arg type.
// We could specialized this function per arg type, per value type if we wanted to.
template<typename T>
template<typename U>
inline bool ConsoleArg<T>::GetArgValueAsType( U& value ) const
{
  value = static_cast<U>(value_);
  return true;
}

//------------------------------------------------------------------------------------------------------------------------------
// Create a case statement for each of our supported argument types.
#define CONSOLE_INCLUDE_MANUAL_TYPES
#define CONSOLE_ARG_TYPE( __argname__, __argtype__, ... ) \
  case ConsoleArgType_##__argname__: success = static_cast<const ConsoleArg<__argtype__>*>(this)->GetArgValueAsType( value ); \
    break;

template<typename T>
inline bool IConsoleFunctionArg::GetArgumentValue( T& value ) const
{
  bool success = false;
  
  if ( IsSet() )
  {
    // Each argument class knows what type it is, so cast us to that type and get the value.
    // Maybe there's a better way to do this, but I couldn't think of one.
    const ConsoleArgType type = GetArgType();
    switch ( type )
    {
      // This will fill in a case statement for each of our supported types ... yay macros!
      #include "util/console/consoleSupportedTypes.def"
      
      case ConsoleArgType_Num:// assert( false );
        break;
    }
  }
  
  return success;
}

#undef CONSOLE_ARG_TYPE
#undef CONSOLE_INCLUDE_MANUAL_TYPES


//==============================================================================================================================
// Note: Console Manual Types (CONSOLE_INCLUDE_MANUAL_TYPES)
// If you created your own specialized template class, you'll need to define your specialized functions here ...


//------------------------------------------------------------------------------------------------------------------------------
// const char* specialization.
template<typename U>
inline bool ConsoleArg<const char*>::GetArgValueAsType( U& value ) const
{
  // Ignore anything that isn't a const char* (see function below)
  return false;
}

//------------------------------------------------------------------------------------------------------------------------------
// const char* specialization.
template<>
inline bool ConsoleArg<const char*>::GetArgValueAsType( const char*& value ) const
{
  value = value_;
  return true;
}

//------------------------------------------------------------------------------------------------------------------------------
// const char* specialization.
// I needed to specialize this function so it wouldn't call into the non-string version of GetArgValueAsType(...), which
// was given the compiler fits because it couldn't convert anything to const char*.
// ( This stops the functions GetArgValueAsType<T>( const char*& value ) from even being created )
template<>
inline bool IConsoleFunctionArg::GetArgumentValue( const char*& value ) const
{
  bool success = false;
  
  if ( IsSet() )
  {
    const ConsoleArgType type = GetArgType();
    if ( type == ConsoleArgType_String )
    {
      success = static_cast<const ConsoleArg<const char*>*>(this)->GetArgValueAsType( value );
    }
  }
  
  return success;
}

#endif // ANKIUTIL_CONSOLE_ARGUMENT_PRIV
