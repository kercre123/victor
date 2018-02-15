/*******************************************************************************************************************************
 *
 *  ConsoleSystem.h
 *  Anki::Util
 *
 *  Created by Jarrod Hatfield on 4/7/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *  Description:
 *  - The Console System allows users to register variables so that they can be edited remotely via console/terminal
 *  - Users can also register functions with the system to be called remotely via console/terminal
 *  - It handles all of the text parsing from the console
 *
 *******************************************************************************************************************************/

#ifndef ANKIUTIL_CONSOLE_SYSTEM
#define ANKIUTIL_CONSOLE_SYSTEM

#include "util/console/consoleFunction.h"
#include "util/console/consoleVariable.h"
#include "util/console/consoleTypes.h"
#include "util/global/globalDefinitions.h"
#include "util/export/export.h"
#include "util/helpers/includeSstream.h"
#include "util/stringTable/stringID.h"

#include <map>
#include <vector>
#include <string>



#if ANKI_DEV_CHEATS
#define ANKI_CONSOLE_SYSTEM_ENABLED   1
#else
#define ANKI_CONSOLE_SYSTEM_ENABLED   0
#endif


namespace Anki{ namespace Util {

//******************************************************************************************************************************
struct ConsoleParseResult
{
  std::string response;
};

class IConsoleChannel;
  
//******************************************************************************************************************************
// The system that manages all of our EditableVars.
class ConsoleSystem
{
  //----------------------------------------------------------------------------------------------------------------------------
public:
  ConsoleSystem();
  ~ConsoleSystem();
  
  void FinishInitialization(const char* iniPath);
  
  template <typename T>
  void Register( T& value, const std::string& keystring, const std::string& category );
  void Register( const std::string& keystring, IConsoleVariable* variable );
  void Register( const std::string& keystring, ConsoleFunc function, const char* categoryName, const std::string& args );
  void Register( const std::string& keystring, IConsoleFunction* function );
  void Unregister( const std::string& keystring );

  bool Eval( const char *text, IConsoleChannel& channel );
  
  template< typename T>
  bool GetArgumentValue( const ConsoleFunctionContext& functiondata, const char* argument, T& value );
  
  //----------------------------------------------------------------------------------------------------------------------------
  // Access
public:
  size_t GetNumConsoleVariables() const { return editvars_.size(); }
  size_t GetNumConsoleFunctions() const { return consolefunctions_.size(); }
  IConsoleVariable* FindVariable( const char* name );
  IConsoleFunction* FindFunction( const char* name );
  
  const IConsoleVariable* FindVariable( const char* name ) const
  {
    return const_cast<ConsoleSystem*>(this)->FindVariable(name);
  }
  const IConsoleFunction* FindFunction( const char* name ) const
  {
    return const_cast<ConsoleSystem*>(this)->FindFunction(name);
  }

  void AppendConsoleVariables( std::string& output ) const;
  void AppendConsoleFunctions( std::string& output ) const;
  
  bool ParseConsoleFunctionCall( IConsoleFunction* functor, const char* token, IConsoleChannel& channel );

private:
  bool ParseConsoleVariableText( char* token, IConsoleChannel& channel, bool readonly );
  bool ParseConsoleFunctionText( char* token, IConsoleChannel& channel );

  //----------------------------------------------------------------------------------------------------------------------------
  // Helpers
public:
  void ToLower( char* text );
   
  //----------------------------------------------------------------------------------------------------------------------------
  // EditVar Mapping
public:
  
  typedef std::vector<const StringID> VariableIdList;
  typedef std::map<const StringID, IConsoleVariable*> VariableDatabase;

  typedef std::vector<const StringID> FunctionIdList;
  typedef std::map<const StringID, IConsoleFunction*> FunctionDatabase;
  
  const VariableIdList& GetVariableIds() const { return varIds_; }
  const VariableDatabase& GetVariableDatabase() const { return editvars_; }
  
  const FunctionIdList& GetFunctionIds() const { return functIds_; }
  const FunctionDatabase& GetFunctionDatabase() const { return consolefunctions_; }
  
private:
  StringID GetSearchKey( const std::string& key ) const;
  
  VariableIdList   varIds_;
  VariableDatabase editvars_;
  
  FunctionIdList   functIds_;
  FunctionDatabase consolefunctions_;
  
  std::vector<IConsoleVariable*> allocatedVariables_;
  std::vector<IConsoleFunction*> allocatedFunctions_;
  
  bool            _isInitializationComplete;
  
  //----------------------------------------------------------------------------------------------------------------------------
public:
  static ConsoleSystem& Instance();
};

//------------------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void ConsoleSystem::Register( T& value, const std::string& keystring, const std::string& category )
{
  // The IConsoleVariable will register itself within its constructor.
  IConsoleVariable* var = new ConsoleVar<T>( value, keystring.c_str(), category.c_str(), false );
  allocatedVariables_.push_back( var );
}

//------------------------------------------------------------------------------------------------------------------------------
template< typename T>
inline bool ConsoleSystem::GetArgumentValue( const ConsoleFunctionContext& functiondata, const char* argument, T& value )
{
  IConsoleFunction* function = FindFunction( functiondata.function );
  if ( function != NULL )
  {
    const IConsoleFunctionArg* arg = function->FindArgument( argument );
    if ( arg != NULL )
    {
      return arg->GetArgumentValue( value );
    }
  }
  
  return false;
}

} // namespace Anki
} //namespace Util


#if ANKI_CONSOLE_SYSTEM_ENABLED
  #define ANKI_CONSOLE_SYSTEM_INIT(iniPath)  Anki::Util::ConsoleSystem::Instance().FinishInitialization(iniPath);
#else
  #define ANKI_CONSOLE_SYSTEM_INIT(iniPath)
#endif

// Exported C Interface (for Unity/C#)
// Note: These must match up with the externs in ConsoleVarMenu.cs

ANKI_C_BEGIN

#pragma mark - Console Variables
/*
 *  Console Variables
 */
ANKI_EXPORT uint32_t NativeAnkiUtilConsoleGetVarCount() ANKI_VISIBLE;
ANKI_EXPORT uint32_t NativeAnkiUtilConsoleGetMaxVarNameLen() ANKI_VISIBLE;
ANKI_EXPORT uint32_t NativeAnkiUtilConsoleGetMaxCategoryNameLen() ANKI_VISIBLE;

// Variable descriptions
ANKI_EXPORT uint8_t NativeAnkiUtilConsoleGetVar(int varIndex,
                                                int* ioNameLength, const char** outName,
                                                int* ioCategoryNameLength, const char** outCategoryName,
                                                double* outMinValue, double* outMaxValue,
                                                uint8_t* outIsToggleable, uint8_t* outIsIntType, uint8_t* outIsSigned) ANKI_VISIBLE;

// Value Getters
ANKI_EXPORT uint32_t NativeAnkiUtilConsoleGetVarValueString(const char* varName, char* outBuffer, uint32_t outBufferLen) ANKI_VISIBLE;
ANKI_EXPORT double   NativeAnkiUtilConsoleGetVarValueAsDouble(const char* varName) ANKI_VISIBLE;
ANKI_EXPORT int64_t  NativeAnkiUtilConsoleGetVarValueAsInt64(const char* varName) ANKI_VISIBLE;
ANKI_EXPORT uint64_t NativeAnkiUtilConsoleGetVarValueAsUInt64(const char* varName) ANKI_VISIBLE;

// Value Setters
ANKI_EXPORT void     NativeAnkiUtilConsoleToggleValue(const char* varName) ANKI_VISIBLE;
ANKI_EXPORT void     NativeAnkiUtilConsoleSetValueWithString(const char* varName, const char* inString) ANKI_VISIBLE;

// Value Defaults
ANKI_EXPORT void     NativeAnkiUtilConsoleResetAllToDefault() ANKI_VISIBLE;
ANKI_EXPORT void     NativeAnkiUtilConsoleResetValueToDefault(const char* varName) ANKI_VISIBLE;
ANKI_EXPORT uint32_t NativeAnkiUtilConsoleIsDefaultValue(const char* varName) ANKI_VISIBLE;

// Forced load and save
ANKI_EXPORT void     NativeAnkiUtilConsoleLoadVars() ANKI_VISIBLE;
ANKI_EXPORT void     NativeAnkiUtilConsoleSaveVars() ANKI_VISIBLE;


#pragma mark - Console Functions
/*
 *  Console Functions
 */
ANKI_EXPORT uint32_t NativeAnkiUtilConsoleGetFunctionCount() ANKI_VISIBLE;
ANKI_EXPORT uint32_t NativeAnkiUtilConsoleGetMaxFunctionNameLen() ANKI_VISIBLE;
ANKI_EXPORT uint32_t NativeAnkiUtilConsoleGetMaxFunctionSignatureLen() ANKI_VISIBLE;

// Function descriptions
ANKI_EXPORT uint8_t NativeAnkiUtilConsoleGetFunction(int varIndex,
                                                     int* outNameLength,      const char** outName,
                                                     int* outCategoryLength,  const char** outCategory,
                                                     int* outSignatureLength, const char** outSignature) ANKI_VISIBLE;
// Call function
ANKI_EXPORT uint32_t NativeAnkiUtilConsoleCallFunction(const char* funcName, const char* funcArgs, uint32_t outTextLength, char* outText) ANKI_VISIBLE;

ANKI_C_END


#endif // ANKIUTIL_CONSOLE_SYSTEM
