/*******************************************************************************************************************************
 *
 *  ConsoleFunction.cpp
 *  Anki::Util
 *
 *  Created by Jarrod Hatfield on 4/7/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *******************************************************************************************************************************/

#include "util/console/consoleFunction.h"
#include "util/console/consoleSystem.h"
#include "util/helpers/includeSstream.h"
#include <assert.h>
#include <stdint.h>

namespace Anki{ namespace Util
{

static const int ARG_TEXT_LENGTH              = 256;
static const ConsoleArgType INVALID_TYPE      = ConsoleArgType_Num;
static const char* OPTIONAL_DECLARATOR        = "optional";


//------------------------------------------------------------------------------------------------------------------------------
IConsoleFunction::IConsoleFunction( const std::string& keystring,
                                    std::function<void(ConsoleFunctionContextRef)>&& function,
                                    const char* categoryName,
                                    const std::string& args )
  : function_( std::move(function) )
  , id_(keystring)
  , category_(categoryName)
{
  ParseFunctionSignature( args );
  Anki::Util::ConsoleSystem::Instance().Register( keystring, this  );
}
  
//------------------------------------------------------------------------------------------------------------------------------
IConsoleFunction::IConsoleFunction( const std::string& keystring, ConsoleFunc function, const char* categoryName, const std::string& args )
  : IConsoleFunction( keystring,
                      [function](ConsoleFunctionContextRef c) { if( function != nullptr ) { function(c); } },
                      categoryName,
                      args )
{
}

//------------------------------------------------------------------------------------------------------------------------------
IConsoleFunction::~IConsoleFunction()
{
  ArgList::iterator it = args_.begin();
  ArgList::iterator end = args_.end();
  for ( ; it != end; ++it )
  {
    delete (*it);
  }
  
  args_.clear();
}

//------------------------------------------------------------------------------------------------------------------------------
void IConsoleFunction::Reset()
{
  // Make sure all of our required variables have been set.
  ArgList::iterator it = args_.begin();
  ArgList::iterator end = args_.end();
  for ( ; it != end; ++it )
  {
    (*it)->ClearArg();
  }
}
  
//------------------------------------------------------------------------------------------------------------------------------
// This takes a whitespace separated list of arguments and fills in our args_ vector.
IConsoleFunction::ERROR_CODE IConsoleFunction::ParseText( const std::string& text )
{
  ERROR_CODE error = ERROR_OK;
  
  size_t begin = 0;
  size_t end = text.find_first_of( ' ', begin );
  
  std::string token;
  unsigned int index = 0;
  const size_t num = args_.size();
  
  bool done = text.empty();
  while ( !done )
  {
    const size_t length = ( ( end != std::string::npos ) ? ( end - begin ) : (int)std::string::npos );
    token = text.substr( begin, length );
    if ( index < num )
    {
      IConsoleFunctionArg* arg = args_[index];
      if ( !arg->SetArgValue( token ) )
      {
        error = ERROR_INVALID_ARG;
        break;
      }
    }
    else
    {
      error = ERROR_TOO_MANY_ARGS;
      break;
    }
  
    ++index;
    
    // We're done when we've read our last argument.
    done = ( end == std::string::npos );
    if ( !done )
    {
      begin = ( end + 1 );
      end = text.find_first_of( ' ', begin );
    }
  }
  
  if ( ( error == ERROR_OK ) && !VerifyFunctionArgs() )
  {
    error = ERROR_NOT_ENOUGH_ARGS;
  }
  
  return error;
}

//------------------------------------------------------------------------------------------------------------------------------
// This reads in the argument signature for the function and constructs placeholders in the args_ vector.
void IConsoleFunction::ParseFunctionSignature( const std::string& args )
{
  signature_ = args;
  
  char argName[ARG_TEXT_LENGTH];

  std::istringstream stream( args );
  stream.setf( std::ios::skipws );
  
  while ( stream.good() )
  {
    stream.getline( argName, ARG_TEXT_LENGTH, ',' );
    stream.ignore( ARG_TEXT_LENGTH, ' ' );
    
    ParseArgDeclaration( argName );
  }
}

//------------------------------------------------------------------------------------------------------------------------------
// Parse the argument declaration text and try to deduce the type and name of the variable.
// Not the most efficient function around, but it's only run at registration time and isn't a performance concern.
// The user can specify if the argument is optional or not, meaning it doesn't need to be sent to the function.
// eg. The string "bool aBool" will create and argument of type bool with the name aBool.
void IConsoleFunction::ParseArgDeclaration( const char* arg )
{
  // Always deal with lowercase, makes parsing easier.
  std::string argstring( arg );
  ToLower( argstring );
  
  // No arguments for this function.
  if ( argstring.empty() )
  {
    return;
  }
  
  size_t begin = 0;
  size_t end = argstring.length();
  
  // User can specify if this arg is optional or not by preceding the arg with "optional"
  bool optional = false;
  const size_t opt = argstring.find( OPTIONAL_DECLARATOR );
  if ( opt != std::string::npos )
  {
    begin = ( opt + strlen( OPTIONAL_DECLARATOR ) );
    optional = true;
  }
  
  // Get the start/end of actual text.
  begin = argstring.find_first_not_of( ' ', begin );
  end = argstring.find_last_not_of( ' ', end );
  
  if ( ( begin == std::string::npos ) || ( end == std::string::npos ) || ( begin >= end ) )
  {
    assert( false ); // Invalid argument ... check signature_ to see where you went wrong
    return;
  }
  
  // Kill all leading and trailing white space.  Use a temp string to work around malloc error
  // that causes crash when using Xcode 9.1 + iOS SDK + -Os optimization.
  std::string tmp = std::move(argstring);
  argstring = tmp.substr(begin, (end - begin + 1));

  // Find where the type ends and the name begins, and build our type name.
  begin = 0;
  end = argstring.find_last_of( ' ' );
  if ( end == std::string::npos )
  {
    assert( false ); // No argument/type name was specified ... check signature_ to see where you went wrong.
    return;
  }
  
  std::string typestring = argstring.substr( begin, (end - begin) );
  
  const ConsoleArgType type = GetArgTypeFromString( typestring );
  if ( type != INVALID_TYPE )
  {
    // Grab the name of the string.
    std::string argName = argstring.substr( end + 1 );
    if ( !argName.empty() )
    {
      const StringID name = GetSearchKey( argName );
      IConsoleFunctionArg* consoleArg = CreateConsoleArg( type, name, optional );
      if ( consoleArg != NULL )
      {
        if ( !AddArgumentToFunction( consoleArg ) )
        {
          delete consoleArg;
        }
      }
    }
  }
  else
  {
    assert( false ); // Invalid argument type ... check signature_ to see where you went wrong
  }
}

//------------------------------------------------------------------------------------------------------------------------------
void IConsoleFunction::ToLower( std::string& text ) const
{
  std::string::iterator it = text.begin();
  std::string::iterator end = text.end();
  for ( ; it != end; ++it )
  {
    (*it) = tolower( (*it) );
  }
}

//------------------------------------------------------------------------------------------------------------------------------
StringID IConsoleFunction::GetSearchKey( const std::string& key ) const
{
  std::string result( key );
  ToLower( result );
  
  return StringID( result );
}

//------------------------------------------------------------------------------------------------------------------------------
bool IConsoleFunction::AddArgumentToFunction( IConsoleFunctionArg* arg )
{
  bool succcess = false;
  
  if ( arg != NULL )
  {
    const IConsoleFunctionArg* existing = FindArgument( arg->GetArgName() );
    if ( existing == NULL )
    {
      args_.push_back( arg );
      succcess = true;
    }
    else
    {
      assert( false ); // Argument name duplicated for this function.
    }
  }
  
  return succcess;
}


//------------------------------------------------------------------------------------------------------------------------------
// Helper function to get the arg type from the specified arg string.
#define CONSOLE_INCLUDE_MANUAL_TYPES
#define CONSOLE_ARG_TYPE( __argname__, __argtype__, __argtokens__ ) \
  __argtokens__,

ConsoleArgType IConsoleFunction::GetArgTypeFromString( const std::string& argstring )
{
  // This map holds all of the accepted string tokens that we're matching per type.
  static const char* ARG_STRING_MAP[ConsoleArgType_Num] =
  {
    // See this file to learn what strings we're accepting for each type ...
    #include "util/console/consoleSupportedTypes.def"
  };
  
  char token[ARG_TEXT_LENGTH];
  for ( int i = 0; i < ConsoleArgType_Num; ++i )
  {
    std::istringstream stream( ARG_STRING_MAP[i] );
    while ( stream.good() )
    {
      stream.getline( token, ARG_TEXT_LENGTH, ',' );
      stream.ignore( ARG_TEXT_LENGTH, ' ' );
      
      if ( argstring == token )
      {
        return (ConsoleArgType)i;
      }
    }
  }
  
  return INVALID_TYPE;
}

#undef CONSOLE_ARG_TYPE
#undef CONSOLE_INCLUDE_MANUAL_TYPES
  

//------------------------------------------------------------------------------------------------------------------------------
// Create an IConsoleFunctionArg* for each supported data type.
// Using a macro so that it's easy to add/remove support for data types.
#define CONSOLE_INCLUDE_MANUAL_TYPES
#define CONSOLE_ARG_TYPE( __argname__, __argtype__, ... ) \
  case ConsoleArgType_##__argname__: arg = ( new ConsoleArg<__argtype__>( name, isOptional ) ); break;

IConsoleFunctionArg* IConsoleFunction::CreateConsoleArg( ConsoleArgType type, StringID name, bool isOptional )
{
  IConsoleFunctionArg* arg = NULL;
  
  switch ( type )
  {
    // This will include a case statement for each supported type in the form ...
    // case ConsoleArg_TYPE: arg     = ( new ConsoleArg<TYPE>::ConsoleArg( name, isOptional ) ); break;
    #include "util/console/consoleSupportedTypes.def"
      
    case ConsoleArgType_Num:
      break;
  }
  
  assert( arg != NULL ); // Argument was not constructed.
  return arg;
}
  
#undef CONSOLE_ARG_TYPE
#undef CONSOLE_INCLUDE_MANUAL_TYPES


//------------------------------------------------------------------------------------------------------------------------------
const IConsoleFunctionArg* IConsoleFunction::FindArgument( const std::string& argname ) const
{
  const StringID argid = GetSearchKey( argname );
  return FindArgument( argid );
}

//------------------------------------------------------------------------------------------------------------------------------
const IConsoleFunctionArg* IConsoleFunction::FindArgument( StringID argid ) const
{
  ArgList::const_iterator it = args_.begin();
  ArgList::const_iterator end = args_.end();
  for ( ; it != end; ++it )
  {
    const IConsoleFunctionArg* arg = (*it);
    if ( argid == arg->GetArgName() )
    {
      return arg;
    }
  }
  
  return NULL;
}

//------------------------------------------------------------------------------------------------------------------------------
bool IConsoleFunction::VerifyFunctionArgs() const
{
  // Make sure all of our required variables have been set.
  ArgList::const_iterator it = args_.begin();
  ArgList::const_iterator end = args_.end();
  for ( ; it != end; ++it )
  {
    if ( !(*it)->IsOptional() && !(*it)->IsSet() )
    {
      return false;
    }
  }
  
  return true;
}


} // namespace Anki
} //namespace Util
