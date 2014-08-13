/**
 * File: logging
 *
 * Author: damjan
 * Created: 4/3/2014
 *
 * Description: print macros
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include <anki/common/basestation/utils/logging/logging.h>

#ifndef FAKE_DAS
#include <util/console/consoleInterface.h>
#endif


namespace AnkiUtil {

bool _errG = false;


#if REMOTE_CONSOLE_ENABLED
#ifndef FAKE_DAS

static const char* DAS_LEVELS[DASLogLevel_NumLevels] =
{
  "debug",
  "info",
  "event",
  "warn",
  "error",
};


void Set_Log_Level( ConsoleFunctionContextRef context )
{
  std::string eventName = ConsoleArg_Get_String( context, "eventname" );
  std::string levelString = ConsoleArg_Get_String( context, "level" );
  
  DASLogLevel logLevel = DASLogLevel_NumLevels;
  for ( int i = 0; i < DASLogLevel_NumLevels; ++i )
  {
    if ( levelString == DAS_LEVELS[i] )
    {
      logLevel = (DASLogLevel)i;
      break;
    }
  }
  
  if ( logLevel != DASLogLevel_NumLevels )
  {
    DASSetLevel( eventName.c_str(), logLevel );
    context->channel->WriteLog("[%s] Level = %s", eventName.c_str(), DAS_LEVELS[logLevel]);
  }
}
CONSOLE_FUNC( Set_Log_Level, const char* eventName, const char* level );


void Get_Log_Level( ConsoleFunctionContextRef context )
{
  std::string eventName = ConsoleArg_Get_String( context, "eventname" );
  
  for ( int i = 0; i < DASLogLevel_NumLevels; ++i )
  {
    const DASLogLevel logLevel = (DASLogLevel)i;
    if ( _DAS_IsEventEnabledForLevel( eventName.c_str(), logLevel ) )
    {
      context->channel->WriteLog("[%s] Level = %s", eventName.c_str(), DAS_LEVELS[i]);
      break;
    }
  }
}
CONSOLE_FUNC( Get_Log_Level, const char* eventName );

#endif // FAKE_DAS
#endif // REMOTE_CONSOLE_ENABLED
  
} // namespace Util


