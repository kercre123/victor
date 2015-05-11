//
//  DAS.cpp
//  BaseStation
//
//  Created by damjan stulic on 1/2/13.
//  Copyright (c) 2013 Anki. All rights reserved.
//

#include "anki/common/basestation/utils/logging/DAS/DAS.h"

#include <stdarg.h>


#ifdef __cplusplus
extern "C" {
#endif

int (*sLogFunction)(const char *format, ...) = printf;
  
void _DASSetLogFunction(int (*function)(const char *, ...))
{
  if (function != NULL) {
    sLogFunction = function;
  }
}
  
void _DASLogFlush()
{
  if (sLogFunction == printf) {
    fflush(stdout);
  }
}
  
static FILE* sLogFileHandle = stdout;
void _DASSetLogFileHandle(FILE *handle)
{
  if (handle != NULL) {
    sLogFileHandle = handle;
  }
}
  
// defaults to WARN
int __FAKE_DAS_gloabl_log_level = 1;

void _DAS_Logf(DASLogLevel level, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
{
  if((int)level < __FAKE_DAS_gloabl_log_level)
    return;

  char renderedLogString[2048];
  va_list argList;
  va_start(argList, line);
  vsnprintf(renderedLogString, 2048, eventValue, argList);
  va_end(argList);
  _DAS_Log(level, eventName, renderedLogString, file, funct, line, NULL);
  _DASLogFlush();
}

/*
void _DAS_Log(DASLogLevel level, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
{
  if((int)level < __FAKE_DAS_gloabl_log_level)
    return;

  va_list argList;
  va_start(argList, line);
  sLogFunction("LOG[%d] - %s - %s", level, eventName, eventValue);
  // fprintf(sLogFileHandle, "LOG[%d] - %s - %s", level, eventName, eventValue);
  const char* key = NULL;
  while (NULL != (key = va_arg(argList, const char*))) {
    const char* value;
    value = va_arg(argList, const char*);
    if(value == NULL) break;
    sLogFunction(" {%s:%s}", key, value);
    //fprintf(sLogFileHandle, " {%s:%s}", key, value);
  }
  sLogFunction("%s", "\n");
  //fprintf(sLogFileHandle, "\n");
  va_end(argList);
  //fflush(sLogFileHandle);
  _DASLogFlush();
}
*/

void _DAS_Log(DASLogLevel level, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
{
  if((int)level < __FAKE_DAS_gloabl_log_level)
    return;
  
  switch (level) {
    case DASLogLevel_Warn:   // Sent to server in production by default
    case DASLogLevel_Error:  // Sent to server in production by default
      _DASSetLogFileHandle(stderr);
      break;
    default:
      _DASSetLogFileHandle(stdout);
      break;
  }
  
  va_list argList;
  va_start(argList, line);
  //sLogFunction("LOG[%d] - %s - %s", level, eventName, eventValue);
  fprintf(sLogFileHandle, "LOG[%d] - %s - %s", level, eventName, eventValue);
  const char* key = NULL;
  while (NULL != (key = va_arg(argList, const char*))) {
    const char* value;
    value = va_arg(argList, const char*);
    if(value == NULL) break;
    //sLogFunction(" {%s:%s}", key, value);
    fprintf(sLogFileHandle, " {%s:%s}", key, value);
  }
  //sLogFunction("%s", "\n");
  fprintf(sLogFileHandle, "\n");
  va_end(argList);
  fflush(sLogFileHandle);
  //_DASLogFlush();
}  
  
  
//  void _DAS_SetGlobal(const char* key, const char* value) {};
//  int  _DAS_IsEventEnabledForLevel(const char* eventName, DASLogLevel level) { return 0; };

#ifdef __cplusplus
} // extern "C"
#endif
