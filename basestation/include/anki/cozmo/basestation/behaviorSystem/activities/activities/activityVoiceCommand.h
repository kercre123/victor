/**
 * File: activityVoiceCommand.h
 *
 * Author: Kevin M. Karol
 * Created: 04/27/17
 *
 * Description: Activity for responding to voice commands from the user
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityVoiceCommand_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityVoiceCommand_H__

#include "anki/cozmo/basestation/behaviorSystem/activities/activities/iActivity.h"
#include "clad/types/voiceCommandTypes.h"
#include "util/signals/simpleSignal_fwd.h"
#include <vector>

namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {
  
//Forward declarations
class IBehavior;
class MoodManager;
class Robot;
class CozmoContext;
class DoATrickSelector;
class RequestGameSelector;

template <typename Type> class AnkiEvent;

class ActivityVoiceCommand : public IActivity
{
public:
  ActivityVoiceCommand(Robot& robot, const Json::Value& config);
  virtual ~ActivityVoiceCommand();
  
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;
  
  
protected:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  static constexpr IBehavior* _behaviorNone = nullptr;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  
  
private:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  const CozmoContext*               _context;
  IBehavior*                        _voiceCommandBehavior = nullptr;
  IBehavior*                        _danceBehavior           = nullptr;
  std::unique_ptr<DoATrickSelector>    _doATrickSelector;
  std::unique_ptr<RequestGameSelector> _requestGameSelector;
  VoiceCommand::VoiceCommandType       _respondingToCommandType = VoiceCommand::VoiceCommandType::Count;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityVoiceCommand_H__
