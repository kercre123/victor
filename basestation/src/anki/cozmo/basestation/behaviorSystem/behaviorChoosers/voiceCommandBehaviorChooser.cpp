/**
* File: voiceCommandBehaviorChooser.cpp
*
* Author: Lee Crippen
* Created: 03/15/17
*
* Description: Class for handling picking of behaviors in response to voice commands
*
* Copyright: Anki, Inc. 2017
*
**/

#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/voiceCommandBehaviorChooser.h"

#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
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

VoiceCommandBehaviorChooser::~VoiceCommandBehaviorChooser() = default;

#if (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
  
#include "clad/types/voiceCommandTypes.h"

using namespace ExternalInterface;

#define LOG_CHANNEL "VoiceCommands"
#define LOG_INFO(...) PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)

VoiceCommandBehaviorChooser::VoiceCommandBehaviorChooser(Robot& robot, const Json::Value& config)
:IBehaviorChooser(robot, config)
, _context(robot.GetContext())
{
  _pounceOnMotionBehavior = robot.GetBehaviorFactory().FindBehaviorByName("pounceOnMotion_voiceCommand");
  DEV_ASSERT(_pounceOnMotionBehavior != nullptr &&
             _pounceOnMotionBehavior->GetClass() == BehaviorClass::PounceOnMotion,
             "VoiceCommandBehaviorChooser.PounceOnMotion.ImproperClassRetrievedForName");
  
  
  DEV_ASSERT(nullptr != _context, "VoiceCommandBehaviorChooser.Constructor.NullContext");
}

IBehavior* VoiceCommandBehaviorChooser::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  auto* voiceCommandComponent = _context->GetVoiceCommandComponent();
  if (!ANKI_VERIFY(voiceCommandComponent != nullptr, "VoiceCommandBehaviorChooser.ChooseNextBehavior", "VoiceCommandComponent invalid"))
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
    
    // Otherwise this behavior isn't running anymore so clear it and potentially get a new one to use below
    _voiceCommandBehavior = nullptr;
  }
  
  using namespace Anki::Cozmo::VoiceCommand;
  const auto& currentCommand = voiceCommandComponent->GetPendingCommand();
  switch (currentCommand)
  {
    case VoiceCommandType::LetsPlay:
    {
      voiceCommandComponent->ClearHeardCommand();
      _voiceCommandBehavior = _pounceOnMotionBehavior;
      return _voiceCommandBehavior;
    }
    // TODO: Handle these two commands appropriately
    case VoiceCommandType::YesPlease:
    case VoiceCommandType::NoThankYou:
      
    // These two commands will never be handled by this chooser:
    case VoiceCommandType::HeyCozmo:
    case VoiceCommandType::Count:
    {
      // We're intentionally not handling these types in VoiceCommandBehaviorChooser
      return _behaviorNone;
    }
  }
}

// TODO This stub implementation intentionally returns no behavior when there is no voice recognition enabled.
#else // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)

VoiceCommandBehaviorChooser::VoiceCommandBehaviorChooser(Robot& robot, const Json::Value& config)
:IBehaviorChooser(robot, config)
, _context(robot.GetContext())
{
  (void) _voiceCommandBehavior;
  (void) _pounceOnMotionBehavior;
}

IBehavior* VoiceCommandBehaviorChooser::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  return _behaviorNone;
}

#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)

} // namespace Cozmo
} // namespace Anki
