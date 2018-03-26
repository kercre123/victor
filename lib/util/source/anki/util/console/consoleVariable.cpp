/*******************************************************************************************************************************
 *
 *  consoleVariable.cpp
 *  Anki::Util
 *
 *  Created by Jarrod Hatfield on 4/7/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *******************************************************************************************************************************/

#include "util/console/consoleVariable.h"
#include "util/console/consoleSystem.h"
#include "util/string/stringHelpers.h"

namespace Anki {
namespace Util {


// Helper function - if a var is named with Hungarian notation - e.g. g_VarName or k_VarName - just use VarName for the console
static const char* SkipHungarianNotation(const char* inVarName)
{
  if (inVarName && (inVarName[0] != 0))
  {
    if (inVarName[1] == '_')
    {
      // in ?_* format (e.g. g_VarName or k_VarName) - use VarName
      return &inVarName[2];
    }
    else if (((inVarName[0] == 'g') || (inVarName[0] == 'k')) && isupper(int(inVarName[1])))
    {
      // in e.g. gVarName or kVarName format - use VarName
      return &inVarName[1];
    }
  }
  
  return inVarName;
}


//------------------------------------------------------------------------------------------------------------------------------
// NOTE: static initialized console variables should have unregisterInDestructor false, this prevents actions during the static
//       destructors dynamically initialzed console variables, e.g. those instantiated after main has started should have dynamic
//       false which will cause unregistration during destruction.
IConsoleVariable::IConsoleVariable(const char* id, const char* category, bool unregisterInDestructor) :
  id_(SkipHungarianNotation(id)),
  category_(category),
  unregisterInDestructor_(unregisterInDestructor) {
  Anki::Util::ConsoleSystem::Instance().Register(id_, this);
}

IConsoleVariable::~IConsoleVariable() {
  if (unregisterInDestructor_) {
    Anki::Util::ConsoleSystem::Instance().Unregister(id_);
  }
}


//==============================================================================================================================
// Template Specializations

//------------------------------------------------------------------------------------------------------------------------------
template<>
std::string ConsoleVar<signed char>::ToString(signed char value) const {
  // specialization required as otherwise this will be displayed as the ascii equivalent
  char result[8];
  snprintf(result, sizeof(result), "%d", value);
  return result;
}

//------------------------------------------------------------------------------------------------------------------------------
template<>
std::string ConsoleVar<unsigned char>::ToString(unsigned char value) const {
  // specialization required as otherwise this will be displayed as the ascii equivalent
  char result[8];
  snprintf(result, sizeof(result), "%d", value);
  return result;
}
  
//------------------------------------------------------------------------------------------------------------------------------
template<>
bool ConsoleVar<signed char>::ParseText(const char* text)
{
  const int desiredVal = atoi(text);
  _value = numeric_cast_clamped<signed char>(desiredVal);
  return (desiredVal == (int)_value);
}
  
//------------------------------------------------------------------------------------------------------------------------------
template<>
bool ConsoleVar<unsigned char>::ParseText(const char* text)
{
  const int desiredVal = atoi(text);
  _value = numeric_cast_clamped<unsigned char>(desiredVal);
  return (desiredVal == (int)_value);
}

//------------------------------------------------------------------------------------------------------------------------------
template<>
bool ConsoleVar<bool>::ParseText(const char* text)
{
  if ((nullptr == text) || (strlen(text) == 0))
  {
    // toggle on empty input
    _value = !_value;
  }
  else if ((stricmp(text, "true") == 0) || (strcmp(text, "1") == 0))
  {
    _value = true;
  }
  else if ((stricmp(text, "false") == 0) || (strcmp(text, "0") == 0))
  {
    _value = false;
  }
  else
  {
    // unexpected input - cannot parse
    return false;
  }
  
  return true;
}

}
}
