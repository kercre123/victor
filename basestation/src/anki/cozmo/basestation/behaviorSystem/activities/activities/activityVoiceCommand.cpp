/**
* File: activityVoiceCommand.cpp
*
* Author: Kevin M. Karol
* Created: 04/27/17
*
* Description: Activity for responding to voice commands from the user
*
* Copyright: Anki, Inc. 2017
*
**/


#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityVoiceCommand.h"

#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/voiceCommandUtils/doATrickSelector.h"
#include "anki/cozmo/basestation/behaviorSystem/voiceCommandUtils/requestGameSelector.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/voiceCommands/voiceCommandComponent.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  

  
ActivityVoiceCommand::~ActivityVoiceCommand() = default;
  
#include "clad/types/voiceCommandTypes.h"
#include "clad/types/unlockTypes.h"

using namespace ExternalInterface;
using namespace ::Anki::Cozmo::VoiceCommand;

#define LOG_CHANNEL "VoiceCommands"
#define LOG_INFO(...) PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityVoiceCommand::ActivityVoiceCommand(Robot& robot, const Json::Value& config)
:IActivity(robot, config)
, _context(robot.GetContext())
, _voiceCommandBehavior(nullptr)
, _doATrickSelector(new DoATrickSelector(robot))
, _requestGameSelector(new RequestGameSelector(robot))
{
  // TODO: Will need to change how this works should we add more dance behaviors
  _danceBehavior = robot.GetBehaviorFactory().FindBehaviorByID(BehaviorID::Dance_Mambo);
  DEV_ASSERT(_danceBehavior != nullptr &&
             _danceBehavior->GetClass() == BehaviorClass::Dance,
             "VoiceCommandBehaviorChooser.PounceOnMotion.ImproperClassRetrievedForName");
  
  DEV_ASSERT(nullptr != _context, "ActivityVoiceCommand.Constructor.NullContext");
}

IBehavior* ActivityVoiceCommand::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  auto* voiceCommandComponent = _context->GetVoiceCommandComponent();
  if (!ANKI_VERIFY(voiceCommandComponent != nullptr, "ActivityVoiceCommand.ChooseNextBehavior", "VoiceCommandComponent invalid"))
  {
    return _behaviorNone;
  }
  
  // If we already have a behavior assigned we should keep using it, if it's running
  if (_voiceCommandBehavior)
  {
    if (_voiceCommandBehavior->IsRunning())
    {
      return _voiceCommandBehavior;
    }
    
    voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandEnd(_respondingToCommandType));
    // Otherwise this behavior isn't running anymore so clear it and potentially get a new one to use below
    _voiceCommandBehavior = nullptr;
  }
  
  _respondingToCommandType = voiceCommandComponent->GetPendingCommand();
  switch (_respondingToCommandType)
  {
    case VoiceCommandType::LetsPlay:
    {
      voiceCommandComponent->ClearHeardCommand();
      _voiceCommandBehavior = _requestGameSelector->
                        GetNextRequestGameBehavior(robot, currentRunningBehavior);
      
      voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandStart(_respondingToCommandType));
      
      return _voiceCommandBehavior;
    }
    case VoiceCommandType::DoADance:
    {
      voiceCommandComponent->ClearHeardCommand();
      _voiceCommandBehavior = _danceBehavior;
      
      voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandStart(_respondingToCommandType));
      
      return _voiceCommandBehavior;
      
    }
    case VoiceCommandType::DoATrick:
    {
      voiceCommandComponent->ClearHeardCommand();
      _doATrickSelector->RequestATrick(robot);
      voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandStart(_respondingToCommandType));
      return _behaviorNone;
    }

    // TODO: Handle these two commands appropriately
    case VoiceCommandType::YesPlease:
    case VoiceCommandType::NoThankYou:
    
    // These two commands will never be handled by this chooser:
    case VoiceCommandType::HeyCozmo:
    case VoiceCommandType::Count:
    {
      // We're intentionally not handling these types in ActivityVoiceCommand
      return _behaviorNone;
    }
  }
}

} // namespace Cozmo
} // namespace Anki
