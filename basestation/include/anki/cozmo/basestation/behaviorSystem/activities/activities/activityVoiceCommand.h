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
#include <queue>
#include <vector>

namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {
  
//Forward declarations
class BehaviorDriveToFace;
class BehaviorPlayArbitraryAnim;
class CozmoContext;
class IBehavior;
class MoodManager;
class Robot;

template <typename Type> class AnkiEvent;

class ActivityVoiceCommand : public IActivity
{
public:
  using ChooseNextBehaviorFunc = std::function<IBehaviorPtr(const IBehaviorPtr)>;
  using ChooseNextBehaviorQueue = std::queue<ChooseNextBehaviorFunc>;

  ActivityVoiceCommand(Robot& robot, const Json::Value& config);
  virtual ~ActivityVoiceCommand();
  
  virtual Result Update(Robot& robot) override;
  
  template<typename T>
  void HandleMessage(const T& msg);
  
protected:  
  virtual IBehaviorPtr ChooseNextBehaviorInternal(Robot& robot, const IBehaviorPtr currentRunningBehavior) override;
  

  
private:
  // Class for tracking the response queue and current behavior running as a result
  // of a voice command
  class VCResponseData{
  public:
    VCResponseData(const CozmoContext* context)
    : _context(context){}
    
    VoiceCommand::VoiceCommandType GetCommandType() const { return _currentResponseType;}

    // Setup the response queue for the specified VoiceCommandType which was received
    void SetNewResponseData(ChooseNextBehaviorQueue&& responseQueue,
                            VoiceCommand::VoiceCommandType commandType);
    // Reset VC data and notify the VoiceCommandComponent that the response to
    // this voice command has finished
    void ClearResponseData();
    
    // Clears the queue of behavior functions that have not been run yet, but
    // allows the current response behavior to finish
    void ClearResponseQueue();
    
    // Return the behavior to run based on the response queue
    IBehaviorPtr ChooseNextBehavior(const IBehaviorPtr currentBehavior);
    
  private:
    ChooseNextBehaviorQueue        _respondToVCQueue;
    IBehaviorPtr                   _currentResponseBehavior;
    VoiceCommand::VoiceCommandType _currentResponseType = VoiceCommand::VoiceCommandType::Count;
    const CozmoContext*            _context;
    // Returns true if ResponseData has not previously returned a behavior to run
    // but will return one this tick
    bool WillStartRespondingToCommand();
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  const CozmoContext*   _context;
  
  // Track the response queue and current response behavior
  std::unique_ptr<VCResponseData> _vcResponseData;
  
  // Behaviors VC activity runs directly
  IBehaviorPtr                         _danceBehavior;
  std::shared_ptr<BehaviorDriveToFace> _driveToFaceBehavior;
  IBehaviorPtr                         _searchForFaceBehavior;
  IBehaviorPtr                         _fistBumpBehavior;
  IBehaviorPtr                         _peekABooBehavior;
  IBehaviorPtr                         _laserBehavior;
  IBehaviorPtr                         _goToSleepBehavior;
  IBehaviorPtr                         _pounceBehavior;
  IBehaviorPtr                         _alrightyBehavior;
  
  std::map<UnlockId, IBehaviorPtr> _letsPlayMap;
  
  std::shared_ptr<BehaviorPlayArbitraryAnim> _playAnimBehavior;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  // Track
  bool _lookDownObjectiveAchieved;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Returns true if the command should be handled
  bool ShouldActivityRespondToCommand(VoiceCommand::VoiceCommandType command) const;
  
  // Returns true if we should check needs state for this command
  bool ShouldCheckNeeds(VoiceCommand::VoiceCommandType command) const;
  
  // Returns whether or not we have enough sparks to execute the command
  bool HasEnoughSparksForCommand(Robot& robot, VoiceCommand::VoiceCommandType command) const;
  
  // Returns whether or not the unlockID a behavior requires for a voice command is unlocked
  bool HasAppropriateUnlocksForCommand(Robot& robot, VoiceCommand::VoiceCommandType command) const;
  
  void RemoveSparksForCommand(Robot& robot, VoiceCommand::VoiceCommandType command);
  
  // Checks if the voice command should be refused due to severe needs states
  // Returns true if the outputBehavior has been set to the refuse behavior due to our needs state
  bool CheckRefusalDueToNeeds(Robot& robot, ChooseNextBehaviorQueue& responseQueue) const;
  
  // Setups up the refuse behavior to play the animTrigger
  // Returns true if the outputBehavior has been set to the refuse behavior
  bool CheckAndSetupRefuseBehavior(Robot& robot, BehaviorID whichRefuse, ChooseNextBehaviorQueue& responseQueue) const;
  
  // Handles what response to do for the "How are you doing" command based on Cozmo's current needs
  void HandleHowAreYouDoingCommand(Robot& robot, ChooseNextBehaviorQueue& responseQueue);
  
  // Sends the event indicating we're beginning running a behavior in response to command, and stores the task to
  // do once we're done responding to the command
  static void BeginRespondingToCommand(const CozmoContext* context, VoiceCommand::VoiceCommandType command);
  
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityVoiceCommand_H__
