/**
 * File: DebugConsoleManager
 *
 * Author: Molly Jameson
 * Created: 11/17/15
 *
 * Description: A lightweight wrapper around the console so that it can use CLAD at 
 * the game level instead of util
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/debug/debugConsoleManager.h"
#include "util/console/consoleSystem.h"
#include "clad/types/debugConsoleTypes.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/engineImpl/cozmoEngineHostImpl.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/console/consoleChannelFile.h"

#include <assert.h>


namespace Anki {
namespace Cozmo {
  
  void DebugConsoleManager::Init( IExternalInterface* externalInterface )
  {
    _externalInterface = externalInterface;
    
    auto debugConsoleEventCallback = std::bind(&DebugConsoleManager::HandleEvent, this, std::placeholders::_1);
    std::vector<ExternalInterface::MessageGameToEngineTag> tagList =
    {
      ExternalInterface::MessageGameToEngineTag::GetAllDebugConsoleVarMessage,
      ExternalInterface::MessageGameToEngineTag::SetDebugConsoleVarMessage,
      ExternalInterface::MessageGameToEngineTag::RunDebugConsoleFuncMessage,
      ExternalInterface::MessageGameToEngineTag::GetDebugConsoleVarMessage,
    };
    
    // Subscribe to desired events
    for (auto tag : tagList)
    {
      _signalHandles.push_back(_externalInterface->Subscribe(tag, debugConsoleEventCallback));
    }
  }
  
  // Used for init of window.
  void DebugConsoleManager::SendAllDebugConsoleVars()
  {
    const Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();

    const Anki::Util::ConsoleSystem::VariableDatabase& varDatabase = consoleSystem.GetVariableDatabase();
    std::vector<ExternalInterface::DebugConsoleVar> dataVals;
    for ( auto& entry : varDatabase )
    {
      const Anki::Util::IConsoleVariable* consoleVar = entry.second;
      ExternalInterface::DebugConsoleVar varObject;
      varObject.varName = consoleVar->GetID();
      varObject.category = consoleVar->GetCategory();
      if( consoleVar->IsToggleable())
      {
        varObject.varValue.Set_varBool( consoleVar->GetAsUInt64() );
      }
      else if( consoleVar->IsIntegerType() )
      {
        if( consoleVar->IsSignedType())
        {
          varObject.varValue.Set_varInt( consoleVar->GetAsInt64() );
        }
        else
        {
          varObject.varValue.Set_varUint( consoleVar->GetAsUInt64() );
        }
      }
      else
      {
        varObject.varValue.Set_varDouble( consoleVar->GetAsDouble() );
      }
      
      varObject.maxValue = consoleVar->GetMaxAsDouble();
      varObject.minValue = consoleVar->GetMinAsDouble();
      
      dataVals.push_back(varObject);
    }
    const Anki::Util::ConsoleSystem::FunctionDatabase& funcDatabase = consoleSystem.GetFunctionDatabase();
    for ( auto& entry : funcDatabase )
    {
      const Anki::Util::IConsoleFunction* consoleVar = entry.second;
      ExternalInterface::DebugConsoleVar varObject;
      varObject.varName = consoleVar->GetID();
      varObject.category = consoleVar->GetCategory();
      varObject.varValue.Set_varFunction(consoleVar->GetSignature());
      dataVals.push_back(varObject);
    }
    
    ExternalInterface::InitDebugConsoleVarMessage message;
    message.varData = dataVals;
    
    _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
  }
  
  void DebugConsoleManager::HandleEvent(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
  {
    const auto& eventData = event.GetData();
    switch (eventData.GetTag())
    {
      case ExternalInterface::MessageGameToEngineTag::GetAllDebugConsoleVarMessage:
      {
        // Shoot back all the init messages
        SendAllDebugConsoleVars();
      }
      break;
      case ExternalInterface::MessageGameToEngineTag::RunDebugConsoleFuncMessage:
      {
        const Anki::Cozmo::ExternalInterface::RunDebugConsoleFuncMessage& msg = eventData.Get_RunDebugConsoleFuncMessage();
        Anki::Util::IConsoleFunction* consoleFunc = Anki::Util::ConsoleSystem::Instance().FindFunction(msg.funcName.c_str());
        if( consoleFunc )
        {
          enum { kBufferSize = 512 };
          char buffer[kBufferSize];
          NativeAnkiUtilConsoleCallFunction( msg.funcName.c_str(), msg.funcArgs.c_str(), kBufferSize, buffer);
          
          ExternalInterface::VerifyDebugConsoleVarMessage message;
          message.varName = msg.funcName;
          message.statusMessage = buffer;
          _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
        }
      }
      break;
      case ExternalInterface::MessageGameToEngineTag::SetDebugConsoleVarMessage:
      {
        const Anki::Cozmo::ExternalInterface::SetDebugConsoleVarMessage& msg = eventData.Get_SetDebugConsoleVarMessage();
        Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(msg.varName.c_str());
        
        if( consoleVar && !consoleVar->ParseText(msg.tryValue.c_str()) )
        {
          // Error message that doesn't wig out when entering decimals etcs?
          PRINT_NAMED_ERROR("DebugConsoleManager.HandleEvent.SetDebugConsoleVarMessage", "Unparsable text when setting var %s",msg.varName.c_str());
        }
      }
      break;
      default:
        PRINT_NAMED_ERROR("DebugConsoleManager.HandleEvent.UnhandledMessageGameToEngineTag", "Unexpected tag %u", (uint32_t)eventData.GetTag());
        assert(0);
      break;
    }
  }
  
} // namespace Cozmo
} // namespace Anki

