/*******************************************************************************************************************************
 *
 *  ConsoleSystem..cpp
 *  Anki::Util
 *
 *  Created by Jarrod Hatfield on 4/7/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *******************************************************************************************************************************/

#include "util/console/consoleSystem.h"
#include "util/console/consoleInterface.h"
#include "util/console/consoleChannel.h"
#include "util/helpers/cConversionHelpers.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

using namespace std;

#define LOG_CHANNEL    "ConsoleSystem"

void NativeAnkiUtilConsoleLoadVarsWithContext(ConsoleFunctionContextRef context);
void NativeAnkiUtilConsoleSaveVarsWithContext(ConsoleFunctionContextRef context);
void NativeAnkiUtilConsoleDeleteVarsWithContext(ConsoleFunctionContextRef context);

std::string g_ConsoleVarIniFilePath = "";

CONSOLE_VAR(bool, kSaveModifiedConsoleVarsOnly, "Console", true);


namespace Anki { namespace Util {


#if defined(DEBUG)
  #define DEBUG_CONSOLE_SYSTEM 0
#else
  #define DEBUG_CONSOLE_SYSTEM 0
#endif

#if DEBUG_CONSOLE_SYSTEM
  #define CONSOLE_DEBUG( message, args... ) PRINT_NAMED_INFO( "ConsoleSystem.Debugging", message, ##args )
#else
  #define CONSOLE_DEBUG( ... )
#endif

static const int TEXT_PARSING_MAX_LENGTH    = 256;
static const char* TEXT_PARSING_TOKENS      = " ";
static const char* ARGS_PARSING_TOKENS      = " ,";


//------------------------------------------------------------------------------------------------------------------------------
ConsoleSystem::ConsoleSystem()
  : _isInitializationComplete(false)
{

}

//------------------------------------------------------------------------------------------------------------------------------
ConsoleSystem::~ConsoleSystem()
{
  editvars_.clear();
  varIds_.clear();
  
  consolefunctions_.clear();
  functIds_.clear();
  
  // Delete our allocated variables
  {
    std::vector<IConsoleVariable*>::iterator it = allocatedVariables_.begin();
    std::vector<IConsoleVariable*>::iterator end = allocatedVariables_.end();
    for ( ; it != end; ++it )
    {
      delete (*it);
    }
    
    allocatedVariables_.clear();
  }
  
  // Delete our allocated functions
  {
    std::vector<IConsoleFunction*>::iterator it = allocatedFunctions_.begin();
    std::vector<IConsoleFunction*>::iterator end = allocatedFunctions_.end();
    for ( ; it != end; ++it )
    {
      delete (*it);
    }
    
    allocatedFunctions_.clear();
  }
  
  // JARROD-TODO: We should probably allocate all of our variables/functions so that we don't rely on some being static and
  // others being allocated.  Makes more sense to have them all created the same.
  // This means that IConsoleVariable and IConsoleFunction would merely be a shell class for allocating vars/funcs at runtime
  // through the ConsoleSystem (same way the c-api does it).
}
  
//------------------------------------------------------------------------------------------------------------------------------
void ConsoleSystem::FinishInitialization(const char* iniPath)
{
  if (!_isInitializationComplete)
  {
    g_ConsoleVarIniFilePath = iniPath;
    _isInitializationComplete = true;
    NativeAnkiUtilConsoleLoadVars();
    
    LOG_INFO("ConsoleSystem.FinishInitialization",
             "NativeAnkiUtilAreDevFeaturesEnabled() = %d",
             NativeAnkiUtilAreDevFeaturesEnabled() ? 1 : 0);
  }
}

//------------------------------------------------------------------------------------------------------------------------------
// In-place conversion of c-string to all lowercase.
void ConsoleSystem::ToLower( char* text )
{
  if ( text != NULL )
  {
    const size_t length = strlen( text );
    for ( int i = 0; i < length; ++i )
    {
      text[i] = tolower( text[i] );
    }
  }
}

//------------------------------------------------------------------------------------------------------------------------------
StringID ConsoleSystem::GetSearchKey( const string& key ) const
{
  string result( key );
  
  string::iterator it = result.begin();
  string::iterator end = result.end();
  for ( ; it != end; ++it )
  {
    (*it) = tolower( (*it) );
  }
  
  return StringID( result );
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleSystem::Register( const std::string& keystring, IConsoleVariable* variable )
{
  const StringID key = GetSearchKey( keystring );

  pair<VariableDatabase::iterator, bool> result;
  result = editvars_.emplace( key, variable );
  
  // We require unique edit var names so that we can access them by name.
  if ( !result.second )
  {
    // Fail!  We're attempting to add a duplicate name.
    LOG_WARNING("ConsoleSystem.Register.DuplicateVariable",
      "Console variable '%s' has already been registered. Duplicate will be ignored.", 
      key.c_str());
  }
  else
  {
    // Keep track of the id as well.
    varIds_.push_back( key );
  }
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleSystem::Register( const std::string& keystring, ConsoleFunc function, const char* categoryName, const std::string& args )
{
  // The IConsoleVariable will register itself within its constructor.
  IConsoleFunction* func = new IConsoleFunction( keystring, function, categoryName, args );
  allocatedFunctions_.push_back( func );
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleSystem::Register( const std::string& keystring, IConsoleFunction* function )
{
  const StringID key = GetSearchKey( keystring );
  pair<FunctionDatabase::iterator, bool> result;
  result = consolefunctions_.emplace( key, function );
  
  // We require unique console function names so that we can call them by name.
  if (!result.second)
  {
    // Fail!  We're attempting to add a duplicate name.
    LOG_WARNING("ConsoleSystem.Register.DuplicateFunction",
      "Console function '%s' has already been registered. Duplicate will be ignored.", 
      key.c_str());
  }
  else
  {
    // We added one, keep track of the name we added.
    functIds_.push_back( key );
  }
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleSystem::Unregister( const std::string& keystring )
{
  const StringID key = GetSearchKey( keystring );
  editvars_.erase( key );
}

//------------------------------------------------------------------------------------------------------------------------------
IConsoleVariable* ConsoleSystem::FindVariable( const char* name )
{
  IConsoleVariable* var = NULL;
  
  const StringID key = GetSearchKey( name );
  VariableDatabase::iterator found = editvars_.find( key );
  if ( found != editvars_.end() )
  {
    var = (*found).second;
    
    CONSOLE_DEBUG( "Found EditVar [%s]", name );
  }
  else
  {
    CONSOLE_DEBUG( "Could not find EditVar [%s]", name );
  }
  
  return var;
}

//------------------------------------------------------------------------------------------------------------------------------
IConsoleFunction* ConsoleSystem::FindFunction( const char* name )
{
  IConsoleFunction* func = NULL;
  
  const StringID key = GetSearchKey( name );
  FunctionDatabase::iterator found = consolefunctions_.find( key );
  if ( found != consolefunctions_.end() )
  {
    func = (*found).second;
    CONSOLE_DEBUG( "Found Console Function [%s]", name );
  }
  else
  {
    CONSOLE_DEBUG( "Could not find Console Function [%s]", name );
  }
  
  return func;
}

//------------------------------------------------------------------------------------------------------------------------------
bool ConsoleSystem::Eval(const char *text, IConsoleChannel& channel )
{
  bool success = false;
  
  CONSOLE_DEBUG( "Received text [%s]", text );
  
  // Allocate our editable string, copy in the const text.
  char input[TEXT_PARSING_MAX_LENGTH];
  strncpy( input, text, TEXT_PARSING_MAX_LENGTH );
  input[TEXT_PARSING_MAX_LENGTH-1] = '\0'; // Ensure the string is null terminated.
  
  // The first token is assumed to be the request type.
  char* token = strtok( input, TEXT_PARSING_TOKENS );
  if ( token != NULL )
  {
    ToLower( token );
    if ( strcmp( token, "set" ) == 0  )
    {
      token = strtok( NULL, TEXT_PARSING_TOKENS );
      success = ParseConsoleVariableText( token, channel, false );
    }
    else if ( strcmp( token, "get" ) == 0 )
    {
      token = strtok( NULL, TEXT_PARSING_TOKENS );
      success = ParseConsoleVariableText( token, channel, true );
    }
    else
    {
      success = ParseConsoleFunctionText( token, channel );
    }
  }
  
  return success;
}
//------------------------------------------------------------------------------------------------------------------------------
// Parse the text with the assumption this is a console var.
bool ConsoleSystem::ParseConsoleVariableText( char* token, IConsoleChannel& channel, bool readonly )
{
  const char* varName = token;
  IConsoleVariable* var = varName ? FindVariable( varName ) : nullptr;
  if ( var )
  {
    if ( !readonly )
    {
      // If we found a matching edit var, update the value via the interface.
      // The next token should be the value;
      token = strtok( NULL, TEXT_PARSING_TOKENS );
      ToLower( token );
      
      std::string oldValue = var->ToString();
      const bool parsedOK = var->ParseText( token );
      
      if (!parsedOK)
      {
        channel.WriteLog("Error parsing '%s' into var '%s': was = '%s', now = '%s'", token, varName, oldValue.c_str(), var->ToString().c_str());
      }
      
      CONSOLE_DEBUG( "New value for [%s] -> %s", var->GetID().c_str(), var->ToString().c_str() );
    }
    
    channel.WriteLog("[%s] = %s", var->GetID().c_str(), var->ToString().c_str());
    return true;
  }

  channel.WriteLog("Cound not find variable [%s]", varName);
  return false;
}

//------------------------------------------------------------------------------------------------------------------------------
// Parse the text with the assumption this is a console funtion.
bool ConsoleSystem::ParseConsoleFunctionCall( IConsoleFunction* functor, const char* token, IConsoleChannel& channel )
{
  bool success = false;
  
  if ( functor != NULL )
  {
    CONSOLE_DEBUG( "Calling Console Function [%s]", functor->GetID().c_str() );
    
    // Format our arguments so there is a single space between each argument.
    // Allocate our editable string, copy in the const text.
    string input;
    char funcArgs[TEXT_PARSING_MAX_LENGTH];
    strncpy( funcArgs, token, TEXT_PARSING_MAX_LENGTH );
    funcArgs[TEXT_PARSING_MAX_LENGTH-1] = '\0'; // Ensure the string is null terminated.
    char* arg;
    char* lastPtr;
    
    for (arg = strtok_r( funcArgs, ARGS_PARSING_TOKENS, &lastPtr ); arg != NULL;
         arg = strtok_r(NULL, ARGS_PARSING_TOKENS, &lastPtr) ) {
      if ( !input.empty() ) {
        input += " ";
      }
      input += arg;
    }

    IConsoleFunction::ERROR_CODE error = functor->ParseText( input );
    if ( error == IConsoleFunction::ERROR_OK )
    {
      // Call our console function
      ConsoleFunctionContext context = {
        .function = functor->GetID().c_str(),
        .arguments = input.c_str(),
        .channel = &channel
      };
      (*functor)( context );
      
      success = true;
    }
    else
    {
      channel.WriteLog("Invalid Arguments, expected '%s' got '%s'", functor->GetSignature().c_str(), input.c_str());
    }
    
    functor->Reset();
  }
  else
  {
    channel.WriteLog("ParseConsoleFunctionCall given a null functor?! [%s]", token);
  }
  
  return success;
}
  
//------------------------------------------------------------------------------------------------------------------------------
// Parse the text with the assumption this is a console funtion.
bool ConsoleSystem::ParseConsoleFunctionText( char* token, IConsoleChannel& channel )
{
  IConsoleFunction* functor = FindFunction( token );
  if ( functor != NULL )
  {
    token = strtok( NULL, ARGS_PARSING_TOKENS ); // Skip past the function name
    return ParseConsoleFunctionCall(functor, token, channel);
  }
  else
  {
    channel.WriteLog("Did not recognize command [%s]", token);
  }
  
  return false;
}

//------------------------------------------------------------------------------------------------------------------------------
void ConsoleSystem::AppendConsoleVariables( std::string& output ) const
{
  VariableDatabase::const_iterator it = editvars_.begin();
  VariableDatabase::const_iterator end = editvars_.end();
  
  for ( ; it != end; ++it )
  {
    const IConsoleVariable* var = (*it).second;
    output += '\n';
    output += var->GetID();
    output += " [" + var->GetCategory() + "]";
    output += " = ";
    output += var->ToString().c_str();
  }
}
  
//------------------------------------------------------------------------------------------------------------------------------
void ConsoleSystem::AppendConsoleFunctions( std::string& output ) const
{
  FunctionDatabase::const_iterator it = consolefunctions_.begin();
  FunctionDatabase::const_iterator end = consolefunctions_.end();
  
  for ( ; it != end; ++it )
  {
    output += '\n';
    output += (*it).first.c_str();
  }
}

//------------------------------------------------------------------------------------------------------------------------------
ConsoleSystem& ConsoleSystem::Instance()
{
  static ConsoleSystem editvarsystem_;
  return editvarsystem_;
}
  

//==============================================================================================================================
#if REMOTE_CONSOLE_ENABLED

void CrashTheApp( ConsoleFunctionContextRef context )
{
  volatile int* a = reinterpret_cast<volatile int*>(0);
  *a = 42;
}
CONSOLE_FUNC( CrashTheApp, "Debug" );
  
void ResetConsoleVars( ConsoleFunctionContextRef context )
{
  NativeAnkiUtilConsoleResetAllToDefault();
}
CONSOLE_FUNC( ResetConsoleVars, "Console" );

void LoadConsoleVars( ConsoleFunctionContextRef context )
{
  NativeAnkiUtilConsoleLoadVarsWithContext( context );
}
CONSOLE_FUNC( LoadConsoleVars, "Console" );

void SaveConsoleVars( ConsoleFunctionContextRef context )
{
  NativeAnkiUtilConsoleSaveVarsWithContext( context );
}
CONSOLE_FUNC( SaveConsoleVars, "Console" );

void DeleteConsoleVars( ConsoleFunctionContextRef context )
{
  NativeAnkiUtilConsoleDeleteVarsWithContext( context );
}
CONSOLE_FUNC( DeleteConsoleVars, "Console" );
  
//------------------------------------------------------------------------------------------------------------------------------
void List_Variables( ConsoleFunctionContextRef context )
{
  context->channel->WriteLog("== Variables ==");
  std::string output;
  ConsoleSystem::Instance().AppendConsoleVariables(output);
  context->channel->WriteLog("%s", output.c_str());
  context->channel->WriteLog("===============");
}
CONSOLE_FUNC( List_Variables, "Console" );
  
//------------------------------------------------------------------------------------------------------------------------------
void List_Functions( ConsoleFunctionContextRef context )
{
  context->channel->WriteLog("== Functions ==");
  std::string output;
  ConsoleSystem::Instance().AppendConsoleFunctions(output);
  context->channel->WriteLog("%s", output.c_str());
  context->channel->WriteLog("===============");
}
CONSOLE_FUNC( List_Functions, "Console" );
  
void Console_Help( ConsoleFunctionContextRef context )
{
  const string command = ConsoleArg_GetOptional_String( context, "command", "" );

  if (command.length() == 0) {
    std::string variables;
    context->channel->WriteLog("== Variables ==");
    ConsoleSystem::Instance().AppendConsoleVariables(variables);
    context->channel->WriteLog("%s", variables.c_str());
    context->channel->WriteLog("===============");
    
    std::string functions;
    context->channel->WriteLog("== Functions ==");
    ConsoleSystem::Instance().AppendConsoleFunctions(functions);
    context->channel->WriteLog("%s", functions.c_str());
    context->channel->WriteLog("===============");
  } else {
    // lookup function
    bool found = false;
    IConsoleFunction* functor = ConsoleSystem::Instance().FindFunction(command.c_str());
    if (functor != NULL) {
      found = true;
      context->channel->WriteLog("function:");
      context->channel->WriteLog("  %s <%s>", command.c_str(), functor->GetSignature().c_str());
    } else {
      IConsoleVariable *var = ConsoleSystem::Instance().FindVariable(command.c_str());
      if (var != NULL) {
        found = true;
        context->channel->WriteLog("variable:");
        context->channel->WriteLog("  %s = %s", var->GetID().c_str(), var->ToString().c_str());
        context->channel->WriteLog("To change:");
        context->channel->WriteLog("  set %s <value>");
      }
    }
    if (!found) {
      context->channel->WriteLog("Unknown function or variable: %s", command.c_str());
    }
  }
}
CONSOLE_FUNC( Console_Help, "Console", optional const char* command);
  
#endif // REMOTE_CONSOLE_ENABLED
  
} // namespace Anki
} //namespace Util


// Exported C Interface (for Unity/C#)
// Note: These must match up with the externs in ConsoleVarMenu.cs


uint32_t NativeAnkiUtilConsoleGetVarCount()
{
  const Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();
  const Anki::Util::ConsoleSystem::VariableDatabase& varDatabase = consoleSystem.GetVariableDatabase();
  return Anki::Util::numeric_cast<uint32_t>(varDatabase.size());
}


uint32_t NativeAnkiUtilConsoleGetFunctionCount()
{
  const Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();
  const Anki::Util::ConsoleSystem::FunctionDatabase& funcDatabase = consoleSystem.GetFunctionDatabase();
  return Anki::Util::numeric_cast<uint32_t>(funcDatabase.size());
}


uint32_t NativeAnkiUtilConsoleGetMaxVarNameLen()
{
  const Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();
  const Anki::Util::ConsoleSystem::VariableDatabase& varDatabase = consoleSystem.GetVariableDatabase();
  
  uint32_t maxStringLength = 0;
  for (const auto& entry : varDatabase)
  {
    const Anki::Util::IConsoleVariable* consoleVar = entry.second;
      
    const uint32_t nameLength = Anki::Util::numeric_cast<uint32_t>(strlen(consoleVar->GetID().c_str()));
    maxStringLength = Anki::Util::Max(nameLength, maxStringLength);
  }
  
  return maxStringLength;
}


uint32_t NativeAnkiUtilConsoleGetMaxFunctionNameLen()
{
  const Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();
  const Anki::Util::ConsoleSystem::FunctionDatabase& funcDatabase = consoleSystem.GetFunctionDatabase();
  
  uint32_t maxStringLength = 0;
  for (const auto& entry : funcDatabase)
  {
    const Anki::Util::IConsoleFunction* consoleFunc = entry.second;
    
    const uint32_t nameLength = Anki::Util::numeric_cast<uint32_t>(strlen(consoleFunc->GetID().c_str()));
    maxStringLength = Anki::Util::Max(nameLength, maxStringLength);
  }
  
  return maxStringLength;
}


uint32_t NativeAnkiUtilConsoleGetMaxCategoryNameLen()
{
  const Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();
  
  uint32_t maxStringLength = 0;
 
  // Test all console vars
  {
    const Anki::Util::ConsoleSystem::VariableDatabase& varDatabase = consoleSystem.GetVariableDatabase();
    
    for (const auto& entry : varDatabase)
    {
      const Anki::Util::IConsoleVariable* consoleVar = entry.second;
      
      const uint32_t categoryLength = Anki::Util::numeric_cast<uint32_t>(strlen(consoleVar->GetCategory().c_str()));
      maxStringLength = Anki::Util::Max(categoryLength, maxStringLength);
    }
  }
  // Test all console functions
  {
    const Anki::Util::ConsoleSystem::FunctionDatabase& funcDatabase = consoleSystem.GetFunctionDatabase();

    for (const auto& entry : funcDatabase)
    {
      const Anki::Util::IConsoleFunction* consoleFunc = entry.second;
      
      const uint32_t categoryLength = Anki::Util::numeric_cast<uint32_t>(strlen(consoleFunc->GetCategory()));
      maxStringLength = Anki::Util::Max(categoryLength, maxStringLength);
    }
  }
  
  return maxStringLength;
}


uint32_t NativeAnkiUtilConsoleGetMaxFunctionSignatureLen()
{
  const Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();
  const Anki::Util::ConsoleSystem::FunctionDatabase& funcDatabase = consoleSystem.GetFunctionDatabase();
  
  uint32_t maxStringLength = 0;
  for (const auto& entry : funcDatabase)
  {
    const Anki::Util::IConsoleFunction* consoleFunc = entry.second;
    const uint32_t sigLength = Anki::Util::numeric_cast<uint32_t>(strlen(consoleFunc->GetSignature().c_str()));
    maxStringLength = Anki::Util::Max(sigLength, maxStringLength);
  }
  
  return maxStringLength;
}

uint8_t NativeAnkiUtilConsoleGetVar(int varIndex,
                                    int* outNameLength, const char** outName,
                                    int* outCategoryNameLength, const char** outCategoryName,
                                    double* outMinValue, double* outMaxValue,
                                    uint8_t* outIsToggleable, uint8_t* outIsIntType, uint8_t* outIsSigned)
{

  const Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();
  const Anki::Util::ConsoleSystem::VariableDatabase& varDatabase = consoleSystem.GetVariableDatabase();

  assert(varIndex < consoleSystem.GetVariableIds().size());
  const Anki::Util::StringID variableId = consoleSystem.GetVariableIds()[varIndex];
  
  const auto& found = varDatabase.find(variableId);
  if( found != varDatabase.end()) {
    const Anki::Util::IConsoleVariable* consoleVar = found->second;
    
    // name
    *outNameLength = Anki::Util::numeric_cast<int>(consoleVar->GetID().size() + 1);
    *outName = consoleVar->GetID().c_str();
    
    // category
    *outCategoryNameLength = Anki::Util::numeric_cast<int>(consoleVar->GetCategory().size() + 1);
    *outCategoryName = consoleVar->GetCategory().c_str();

    // value properties
    *outMinValue = consoleVar->GetMinAsDouble();
    *outMaxValue = consoleVar->GetMaxAsDouble();
    *outIsToggleable = consoleVar->IsToggleable();
    *outIsIntType = consoleVar->IsIntegerType();
    *outIsSigned = consoleVar->IsSignedType();

    return 1;
  }
  
  // Variable not found!
  LOG_WARNING("Util.Console.GetVar.NotFound", "Index = %d, Variable = %s", varIndex, variableId.c_str());
  return 0;
}

uint8_t NativeAnkiUtilConsoleGetFunction(int functIndex,
                                         int* outNameLength,      const char** outName,
                                         int* outCategoryLength,  const char** outCategoryName,
                                         int* outSignatureLength, const char** outSignature)
{
  const Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();
  const Anki::Util::ConsoleSystem::FunctionDatabase& functDatabase = consoleSystem.GetFunctionDatabase();
  
  assert(functIndex < consoleSystem.GetFunctionIds().size());
  const Anki::Util::StringID functionId = consoleSystem.GetFunctionIds()[functIndex];
  
  const auto& found = functDatabase.find(functionId);
  if ( found != functDatabase.end())
  {
    const Anki::Util::IConsoleFunction* consoleFunct = found->second;
    
    // name
    *outNameLength = Anki::Util::numeric_cast<int>(consoleFunct->GetID().size() + 1);
    *outName = consoleFunct->GetID().c_str();

    // category
    *outCategoryLength = Anki::Util::numeric_cast<int>(strlen(consoleFunct->GetCategory()) + 1);
    *outCategoryName = consoleFunct->GetCategory();
    
    // signature
    *outSignatureLength = Anki::Util::numeric_cast<int>(consoleFunct->GetSignature().size() + 1);
    *outSignature = consoleFunct->GetSignature().c_str();
    
    return 1;
  }
  
  return 0;
}


uint32_t NativeAnkiUtilConsoleGetVarValueString(const char* varName, char* outBuffer, uint32_t outBufferLen)
{
  const Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(varName);
  return AnkiCopyStringIntoOutBuffer(consoleVar ? consoleVar->ToString().c_str() : "", outBuffer, outBufferLen);
}


void NativeAnkiUtilConsoleToggleValue(const char* varName)
{
  Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(varName);
  if (consoleVar)
  {
    consoleVar->ToggleValue();
  }
  else
  {
    LOG_WARNING("NativeAnkiUtilConsoleToggleValue", "No var named '%s'!", varName);
  }
}


void NativeAnkiUtilConsoleResetValueToDefault(const char* varName)
{
  Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(varName);
  if (consoleVar)
  {
    consoleVar->ResetToDefault();
  }
  else
  {
    LOG_WARNING("NativeAnkiUtilConsoleResetValueToDefault", "No var named '%s'!", varName);
  }
}


uint32_t NativeAnkiUtilConsoleIsDefaultValue(const char* varName)
{
  Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(varName);
  if (consoleVar)
  {
    return consoleVar->IsDefaultValue() ? 1u : 0u;
  }
  else
  {
    LOG_WARNING("NativeAnkiUtilConsoleIsDefaultValue", "No var named '%s'!", varName);
    return 0;
  }
}


void NativeAnkiUtilConsoleResetAllToDefault()
{
  const Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();
  const Anki::Util::ConsoleSystem::VariableDatabase& varDatabase = consoleSystem.GetVariableDatabase();
  
  for (auto& entry : varDatabase)
  {
    Anki::Util::IConsoleVariable* consoleVar = entry.second;
    consoleVar->ResetToDefault();
  }
}


void NativeAnkiUtilConsoleSetValueWithString(const char* varName, const char* inString)
{
  Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(varName);
  if (consoleVar)
  {
    std::string oldValue = consoleVar->ToString();
    const bool parsedOK = consoleVar->ParseText(inString);
    if (!parsedOK)
    {
      LOG_WARNING("NativeAnkiUtilConsoleSetValueWithString", "Error parsing '%s' into var '%s': was = '%s', now = '%s'", inString, varName, oldValue.c_str(), consoleVar->ToString().c_str());
    }
  }
  else
  {
    LOG_WARNING("NativeAnkiUtilConsoleSetValueWithString", "No var named '%s'!", varName);
  }
}


double NativeAnkiUtilConsoleGetVarValueAsDouble(const char* varName)
{
  const Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(varName);
  return consoleVar ? consoleVar->GetAsDouble() : 0.0;
}


int64_t NativeAnkiUtilConsoleGetVarValueAsInt64(const char* varName)
{
  const Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(varName);
  return consoleVar ? consoleVar->GetAsInt64() : 0;
}


uint64_t NativeAnkiUtilConsoleGetVarValueAsUInt64(const char* varName)
{
  const Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(varName);
  return consoleVar ? consoleVar->GetAsUInt64() : 0;
}


// (So we can call ConsoleFunctions from the menu - it directs Console output to TTY using printf)
class MenuConsoleChannel : public Anki::Util::IConsoleChannel
{
public:
  
  MenuConsoleChannel(char* outText, uint32_t outTextLength)
    : _outText(outText)
    , _channelName(DEFAULT_CHANNEL_NAME)
    , _outTextLength(outText ? outTextLength : 0u)
    , _outTextPos(0)
    , _requiredCapacity(0)
    , _ttyLoggingEnabled(true)
  {
    assert((_outText==nullptr) || (_outTextLength > 0));
    
    _tempBuffer = new char[kTempBufferSize];
  }
  
  virtual ~MenuConsoleChannel()
  {
    if (_outText != nullptr)
    {
      // insure out buffer is null terminated
      
      if (_outTextPos < _outTextLength)
      {
        _outText[_outTextPos] = 0;
      }
      else
      {
        _outText[_outTextLength - 1] = 0;
      }
    }
    
    if ((_outText == nullptr) || (_requiredCapacity > _outTextLength))
    {
      PRINT_CH_INFO(_channelName, "MenuConsoleChannel", "Ran out of room - needed %u chars, only given %u", _requiredCapacity, _outTextLength);
    }
    
    delete[] _tempBuffer;
  }
  
  bool IsOpen() override { return true; }
  
  int WriteData(uint8_t *buffer, int len) override
  {
    assert(0); // not implemented (and doesn't seem to ever be called?)
    return len;
  }
  
  int WriteLogv(const char *format, va_list args) override
  {
    // Print to a temporary buffer first so we can use that for any required logs
    const int printRetVal = vsnprintf(_tempBuffer, kTempBufferSize, format, args);
    
    if (printRetVal > 0)
    {
      if ((_outText != nullptr) && (_outTextLength > _outTextPos))
      {
        const uint32_t remainingRoom = _outTextLength - _outTextPos;
        // new line is implicit in all log calls
        const int outPrintRetVal = snprintf(&_outText[_outTextPos], remainingRoom, "%s\n", _tempBuffer);
        // note outPrintRetVal can be >remainingRoom (snprintf returns size required, not size used)
        // so this can make _outTextPos > _outTextLength, but this is ok as we only use it when < _outTextLength
        _outTextPos += (outPrintRetVal > 0) ? outPrintRetVal : 0;
      }
      
      if (_ttyLoggingEnabled)
      {
        PRINT_CH_INFO(_channelName, "Console", "%s", _tempBuffer);
      }
      
      _requiredCapacity += printRetVal;
    }
    
    return printRetVal;
  }
  
  int WriteLog(const char *format, ...) override
  {
    va_list ap;
    va_start(ap, format);
    int result = WriteLogv(format, ap);
    va_end(ap);
    return result;
  }
  
  bool Flush() override
  {
    // already flushed
    return true;
  }
  
  void SetTTYLoggingEnabled(bool newVal) override
  {
    _ttyLoggingEnabled = newVal;
  }
  
  bool IsTTYLoggingEnabled() const override
  {
    return _ttyLoggingEnabled;
  }
  
  const char* GetChannelName() const override { return _channelName; }
  void SetChannelName(const char* newName) override { _channelName = newName; }
  
private:
  
  static const size_t kTempBufferSize = 1024;
  
  char*     _tempBuffer;
  char*     _outText;
  const char* _channelName;
  uint32_t  _outTextLength;
  uint32_t  _outTextPos;
  uint32_t  _requiredCapacity;
  bool      _ttyLoggingEnabled;
};


uint32_t NativeAnkiUtilConsoleCallFunction(const char* funcName, const char* funcArgs, uint32_t outTextLength, char* outText)
{
  Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();
  Anki::Util::IConsoleFunction* consoleFunc = consoleSystem.FindFunction(funcName);
  if (consoleFunc)
  {
    MenuConsoleChannel consoleChannel(outText, outTextLength);
    bool success = consoleSystem.ParseConsoleFunctionCall(consoleFunc, funcArgs, consoleChannel);
    return success ? 1 : 0;
  }
  else
  {
    LOG_WARNING("NativeAnkiUtilConsoleCallFunction", "No func named '%s'!", funcName);
    return 0;
  }
}


static bool IsWhitespaceChar(char inChar)
{
  return((inChar == ' ') || (inChar == '\t') || (inChar == '\n'));
}


static char* SkipWhitespace(char* inString)
{
  char* retString = inString;
  while (IsWhitespaceChar(*retString))
  {
    ++retString;
  }
  return retString;
}


static void TrimTrailingWhitespace(char* inString)
{
  size_t stringLength = strlen(inString);
  while ((stringLength > 0) && IsWhitespaceChar(inString[stringLength-1]))
  {
    --stringLength;
    inString[stringLength] = 0;
  }
}


void NativeAnkiUtilConsoleLoadVarsWithContext(ConsoleFunctionContextRef context)
{
  const char* k_ConsoleVarIniFilePath = g_ConsoleVarIniFilePath.c_str();
  FILE* inputFile = fopen(k_ConsoleVarIniFilePath, "r");
  if (inputFile)
  {
    char tempBuffer[256];
    
    bool isAtEOF = false;
    while (!isAtEOF)
    {
      if (fgets(tempBuffer, sizeof(tempBuffer), inputFile) != nullptr)
      {
        char* lineFromFile = SkipWhitespace(tempBuffer);
        TrimTrailingWhitespace(lineFromFile);
        
        size_t lineLength = strlen(lineFromFile);
        if (lineLength > 0)
        {
          if (lineFromFile[0] == ';')
          {
            // Comment - skip it
          }
          else if (lineFromFile[0] == '[')
          {
            // Section name - skip it
          }
          else if (lineFromFile[0] == '\n')
          {
            // Empty new line - skip it
          }
          else
          {
            // split into VAR_NAME...=...VAR_VAL
            char* eqLoc = strchr(lineFromFile, '=');
            if (eqLoc != nullptr)
            {
              // replace '=' with null terminator and then trim the strings either side of it
              *eqLoc = 0;
              char* varName = lineFromFile;
              TrimTrailingWhitespace(varName);
              char* valueString = SkipWhitespace( &eqLoc[1] );
              TrimTrailingWhitespace(valueString);
              
              Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(varName);
              if (consoleVar != nullptr)
              {
                std::string oldValue = consoleVar->ToString();
                const bool parsedOK  = consoleVar->ParseText(valueString);
                std::string newValue = consoleVar->ToString();
                
                if (!parsedOK)
                {
                  if (context)
                  {
                    context->channel->WriteLog("Error parsing '%s' into var '%s': was = '%s' now = '%s'", valueString, varName, oldValue.c_str(), newValue.c_str());
                  }
                  else
                  {
                    LOG_WARNING("ConsoleSystem.LoadVars", "Error parsing '%s' into var '%s': was = '%s' now = '%s'", valueString, varName, oldValue.c_str(), newValue.c_str());
                  }
                }

                if (oldValue != newValue)
                {
                  if (context)
                  {
                    context->channel->WriteLog("Updated var '%s' from '%s' to '%s'", varName, oldValue.c_str(), newValue.c_str());
                  }
                  else
                  {
                    LOG_INFO("ConsoleSystem.LoadVars", "Updated var '%s' from '%s' to '%s'",
                             varName, oldValue.c_str(), newValue.c_str());
                  }
                }
              }
              else
              {
                LOG_WARNING("ConsoleSystem.LoadVars", "Var '%s' from line '%s' NOT FOUND", varName, lineFromFile);
              }
            }
            else
            {
              LOG_WARNING("ConsoleSystem.LoadVars", "Invalid line '%s' has no '=' to split 'VarName = Value' on!", lineFromFile);
            }
          }
        }
      }
      else
      {
        isAtEOF = true;
      }
    }
    
    int closeRet = fclose(inputFile);
    if (closeRet != 0)
    {
      LOG_WARNING("ConsoleSystem.LoadVars", "fclose '%s' error: returned %d", k_ConsoleVarIniFilePath, closeRet);
    }
  }
  else
  {
    if (context)
    {
      context->channel->WriteLog("Warning: No ini file '%s' to open", k_ConsoleVarIniFilePath);
    }
    else
    {
      LOG_INFO("ConsoleSystem.LoadVars", "No ini file '%s' to open", k_ConsoleVarIniFilePath);
    }
  }
}


void NativeAnkiUtilConsoleLoadVars()
{
  NativeAnkiUtilConsoleLoadVarsWithContext( nullptr );
}


void WriteStringToFile(FILE* outputFile, const char* data)
{
  size_t len = strlen(data);
  size_t ret = fwrite(data, sizeof(*data), len, outputFile);
  if (ret != len)
  {
    LOG_WARNING("ConsoleSystem.WriteStringToFile", "fwrite returned %zu not %zu", ret, len);
  }
}


struct SortConsoleVarsByCategory
{
  bool operator()(const Anki::Util::IConsoleVariable* a, const Anki::Util::IConsoleVariable* b) const
  {
    // Sort on category first, then on id
    if (a->GetCategory() != b->GetCategory())
    {
      return (a->GetCategory() < b->GetCategory());
    }
    else
    {
      return (a->GetID() < b->GetID());
    }
  }
};


void NativeAnkiUtilConsoleSaveVarsWithContext(ConsoleFunctionContextRef context)
{
  const char* k_ConsoleVarIniFilePath = g_ConsoleVarIniFilePath.c_str();
  
  bool saveSucceeded = false;
  FILE* outputFile = fopen(k_ConsoleVarIniFilePath, "w");
  if (outputFile)
  {
    WriteStringToFile(outputFile, "; Console Var Ini Settings\n; Edit Manually or Save/Load from Menu\n");
    
    // Create a list of all console vars, sorted by category
    
    const Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();
    const Anki::Util::ConsoleSystem::VariableDatabase& varDatabase = consoleSystem.GetVariableDatabase();
    
    std::vector<const Anki::Util::IConsoleVariable*> sortedVars;
    for (const auto& entry : varDatabase)
    {
      const Anki::Util::IConsoleVariable* consoleVar = entry.second;
      sortedVars.push_back(consoleVar);
    }
    std::sort(sortedVars.begin(), sortedVars.end(), SortConsoleVarsByCategory());

    std::string lastCategory = "";
    std::string tempStr;
    for (const Anki::Util::IConsoleVariable* consoleVar : sortedVars)
    {
      bool writeConsoleVar = !kSaveModifiedConsoleVarsOnly || !consoleVar->IsDefaultValue();
      if (writeConsoleVar)
      {
        if (lastCategory != consoleVar->GetCategory())
        {
          tempStr = std::string("\n[") + consoleVar->GetCategory() + std::string("]\n\n");
          WriteStringToFile(outputFile, tempStr.c_str());
          lastCategory = consoleVar->GetCategory();
        }
        
        tempStr = consoleVar->GetID() + std::string(" = ") + consoleVar->ToString() + std::string("\n");
        WriteStringToFile(outputFile, tempStr.c_str());
      }
    }
  
    int closeRet = fclose(outputFile);
    if (closeRet == 0)
    {
      saveSucceeded = true;
    }
    else
    {
      LOG_WARNING("ConsoleSystem.SaveVars", "fclose '%s' error: returned %d", k_ConsoleVarIniFilePath, closeRet);
    }
  }
  else
  {
    LOG_WARNING("ConsoleSystem.SaveVars", "failedToOpen file '%s' to write", k_ConsoleVarIniFilePath);
  }
  
  if (context)
  {
    context->channel->WriteLog("Saving Console Var Ini file '%s' %s", k_ConsoleVarIniFilePath, saveSucceeded ? "Succeeded" : "Failed!");
  }
}


void NativeAnkiUtilConsoleSaveVars()
{
  NativeAnkiUtilConsoleSaveVarsWithContext(nullptr);
}


void NativeAnkiUtilConsoleDeleteVarsWithContext(ConsoleFunctionContextRef context)
{
  const char* k_ConsoleVarIniFilePath = g_ConsoleVarIniFilePath.c_str();

  int result = std::remove(k_ConsoleVarIniFilePath);
  if (result != 0)
  {
      LOG_ERROR("ConsoleSystem.DeleteVars", "Error trying to delete console vars file %s", k_ConsoleVarIniFilePath);
  }
}

