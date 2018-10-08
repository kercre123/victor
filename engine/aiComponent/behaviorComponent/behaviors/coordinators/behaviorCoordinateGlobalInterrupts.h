/**
* File: behaviorCoordinateGlobalInterrupts.h
*
* Author: Kevin M. Karol
* Created: 2/22/18
*
* Description: Behavior responsible for handling special case needs 
* that require coordination across behavior global interrupts
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_Behaviors_BehaviorCoordinateGlobalInterrupts_H__
#define __Engine_Behaviors_BehaviorCoordinateGlobalInterrupts_H__


#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherPassThrough.h"
#include "engine/aiComponent/behaviorComponent/behaviorTreeStateHelpers.h"

namespace Anki {
namespace Vector {

// forward declarations
class BehaviorDriveToFace;
class BehaviorHighLevelAI;
class BehaviorReactToVoiceCommand;
class BehaviorTimerUtilityCoordinator;
class BehaviorCoordinateWeather;


class BehaviorCoordinateGlobalInterrupts : public BehaviorDispatcherPassThrough
{
public:
  virtual ~BehaviorCoordinateGlobalInterrupts();
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorCoordinateGlobalInterrupts(const Json::Value& config);

  virtual void InitPassThrough() override;
  virtual void OnPassThroughActivated() override;
  virtual void PassThroughUpdate() override;
  virtual void OnPassThroughDeactivated() override;
  
  virtual void AlwaysHandleInScope(const RobotToEngineEvent& event) override;

private:
  
  void CreateConsoleVars();
  
  struct InstanceConfig{
    InstanceConfig();
    IBEIConditionPtr  triggerWordPendingCond;
    ICozmoBehaviorPtr wakeWordBehavior;
    std::shared_ptr<BehaviorTimerUtilityCoordinator> timerCoordBehavior;
    AreBehaviorsActivatedHelper behaviorsThatShouldSuppressTimerAntics;
    
    std::shared_ptr<BehaviorCoordinateWeather> weatherBehavior;
    ICozmoBehaviorPtr alexaBehavior;

    std::shared_ptr<BehaviorReactToVoiceCommand> reactToVoiceCommandBehavior;
    ICozmoBehaviorPtr reactToObstacleBehavior;

    ICozmoBehaviorPtr meetVictorBehavior;
    std::vector<ICozmoBehaviorPtr> toSuppressWhenMeetVictor;
    
    ICozmoBehaviorPtr danceToTheBeatBehavior;
    std::vector<ICozmoBehaviorPtr> toSuppressWhenDancingToTheBeat;
    
    AreBehaviorsActivatedHelper behaviorsThatShouldntReactToUnexpectedMovement;
    ICozmoBehaviorPtr reactToUnexpectedMovementBehavior;

    AreBehaviorsActivatedHelper behaviorsThatShouldntReactToSoundAwake;
    ICozmoBehaviorPtr reactToSoundAwakeBehavior;

    AreBehaviorsActivatedHelper behaviorsThatShouldntReactToTouch;
    ICozmoBehaviorPtr reactToTouchPettingBehavior;

    AreBehaviorsActivatedHelper behaviorsThatShouldntReactToCliff;
    ICozmoBehaviorPtr reactToCliffBehavior;
    std::vector<std::shared_ptr<BehaviorDriveToFace>> driveToFaceBehaviors;

    std::vector<ICozmoBehaviorPtr> toSuppressWhenGoingHome;
    
    std::unordered_map<ICozmoBehaviorPtr, bool> devActivatableOverrides;
  };

  struct DynamicVariables{
    DynamicVariables();

    bool suppressProx;
    
    int lastReceivedWeather = 0;
    int lastReceivedSpeak = 0;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  bool ShouldSuppressProxReaction();
  
};

} // namespace Vector
} // namespace Anki


#endif // __Engine_Behaviors_BehaviorCoordinateGlobalInterrupts_H__
