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

#include <assert.h>


namespace Anki {
namespace Cozmo {
  
  void DebugConsoleManager::Init( CozmoEngineHostImpl* engine )
  {
    _engine = engine;
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
      if( consoleVar->IsIntegerType() )
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
      
      
      dataVals.push_back(varObject);
    }
    ExternalInterface::InitDebugConsoleVarMessage message;
    message.varData = dataVals;
    
    _engine->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
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
      case ExternalInterface::MessageGameToEngineTag::SetDebugConsoleVarMessage:
      {
        const Anki::Cozmo::ExternalInterface::SetDebugConsoleVarMessage& msg = eventData.Get_SetDebugConsoleVarMessage();
        Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(msg.varName.c_str());
        if( consoleVar && consoleVar->ParseText(msg.tryValue.c_str()) )
        {
          consoleVarUnion varValueUnion;
          varValueUnion.Set_varDouble(consoleVar->GetAsDouble());
          ExternalInterface::VerifyDebugConsoleVarMessage message;
          message.varName = consoleVar->GetID();
          message.varValue = varValueUnion;
          _engine->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
          
          printf("Setting var %s to %s \n",msg.varName.c_str(), msg.tryValue.c_str());
        }
        else
        {
          // TODO: send a set to get us back to something sane.
        }
      }
      break;
      default:
        PRINT_NAMED_ERROR("MoodManager.HandleEvent.UnhandledMessageGameToEngineTag", "Unexpected tag %u", (uint32_t)eventData.GetTag());
        assert(0);
      break;
    }
  }
  
} // namespace Cozmo
} // namespace Anki

