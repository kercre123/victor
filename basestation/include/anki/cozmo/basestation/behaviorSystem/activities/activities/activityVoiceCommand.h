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

#include <functional>
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
  

  virtual Result Update(Robot& robot) override;
  
protected:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Checks if the voice command should be refused due to severe needs states
  // Returns true if the outputBehavior has been set to the refuse behavior due to our needs state
  bool CheckRefusalDueToNeeds(Robot& robot, IBehaviorPtr& outputBehavior) const;
  
  // Returns true if the command should be handled
  bool IsCommandValid(VoiceCommand::VoiceCommandType command) const;
  
  // Returns true if we should check needs state for this command
  bool ShouldCheckNeeds(VoiceCommand::VoiceCommandType command) const;
  
  virtual IBehaviorPtr ChooseNextBehaviorInternal(Robot& robot, const IBehaviorPtr currentRunningBehavior) override;
  
private:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  const CozmoContext*   _context;

  IBehaviorPtr          _voiceCommandBehavior;
  IBehaviorPtr          _danceBehavior;
  IBehaviorPtr          _comeHereBehavior;
  IBehaviorPtr          _fistBumpBehavior;
  IBehaviorPtr          _peekABooBehavior;
  
  std::unique_ptr<DoATrickSelector>    _doATrickSelector;
  std::unique_ptr<RequestGameSelector> _requestGameSelector;
  std::function<void()> _doneRespondingTask;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Returns whether or not we have enough sparks to execute the command
  bool HasEnoughSparksForCommand(Robot& robot, VoiceCommand::VoiceCommandType command) const;
  
  void RemoveSparksForCommand(Robot& robot, VoiceCommand::VoiceCommandType command);
  
  // Setups up the refuse behavior to play the animTrigger
  // Returns true if the outputBehavior has been set to the refuse behavior
  bool CheckAndSetupRefuseBehavior(Robot& robot, BehaviorID whichRefuse, IBehaviorPtr& outputBehavior) const;
  
  // Sends the event indicating we're beginning running a behavior in response to command, and stores the task to
  // do once we're done responding to the command
  void BeginRespondingToCommand(VoiceCommand::VoiceCommandType command, bool trackResponseLifetime = false);
  
  
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityVoiceCommand_H__
