/**
 * File: drivingAnimationHandler.h
 *
 * Author: Al Chaussee
 * Date:   5/6/2016
 *
 * Description: Handles playing animations while driving
 *              Whatever tracks are locked by the action will stay locked while the start and loop
 *              animations but the tracks will be unlocked while the end animation plays
 *              The end animation will always play and will cancel the start/loop animations if needed
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_DRIVING_ANIMATION_HANDLER_H
#define ANKI_COZMO_DRIVING_ANIMATION_HANDLER_H

#include "engine/robotComponents_fwd.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/animationTypes.h"
#include "clad/types/simpleMoodTypes.h"

#include "coretech/common/shared/types.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/signals/simpleSignal_fwd.h"

#include <vector>

namespace Anki {
namespace Cozmo {
  
class Robot;
    
class DrivingAnimationHandler : public IDependencyManagedComponent<RobotComponentID>
{
public:
      
  // Subscribes to ActionCompleted and SetDrivingAnimations messages
  DrivingAnimationHandler();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////
      
  // Container for the various driving animations
  struct DrivingAnimations
  {
    AnimationTrigger drivingStartAnim;
    AnimationTrigger drivingLoopAnim;
    AnimationTrigger drivingEndAnim;
  };
      
  // Sets the driving animations
  void PushDrivingAnimations(const DrivingAnimations& drivingAnimations, const std::string& lockName);
  void RemoveDrivingAnimations(const std::string& lockName);
      
  // Returns true if the drivingEnd animation is currently playing
  // Calling action should return ActionResult::RUNNING as long as this is true.
  bool IsPlayingDrivingEndAnim() const { return _state == AnimState::DrivingEnd; }
      
  // Returns true if the drivingEnd animation has finished.
  // Once this is true, action's CheckIfDone can return return a non-running ActionResult.
  bool HasFinishedDrivingEndAnim() const { return (_state == AnimState::FinishedDriving); }
      
  // Takes in the tag of the action that is calling this and whether or not it is suppressing track locking
  // If keepLoopingWithoutPath is false, endAnim is played automatically once no path is being followed.
  // If true, then calling action must call EndDrivingAnim.
  void Init(const u8 tracksToUnlock, const u32 tag, const bool isActionSuppressingLockingTracks,
            const bool keepLoopingWithoutPath = false);
  
  // Starts playing drivingStart or drivingLoop if drivingStart isn't specified
  void StartDrivingAnim();
      
  // Cancels drivingStart and drivingLoop animations and starts playing drivingEnd animation
  bool EndDrivingAnim();
      
  // Called when the Driving action is being destroyed
  void ActionIsBeingDestroyed();
      
private:
      
  // Listens for driving animations to complete and handles what animation to play next
  void HandleActionCompleted(const ExternalInterface::RobotCompletedAction& msg);
      
  void UpdateCurrDrivingAnimations();
  
  // Queues the respective driving animation
  void PlayDrivingStartAnim();
  void PlayDrivingLoopAnim();
  void PlayDrivingEndAnim();
      
  enum class AnimState
  {
    Waiting,         // State after Init() has been called
      DrivingStart,    // Currently playing the driving start anim
      DrivingLoop,     // Currently playing the driving loop anim
      DrivingEnd,      // Currently playing the driving end anim
      FinishedDriving, // End anim has finished but the action hasn't been destroyed yet
      ActionDestroyed, // The action has been destroyed so we are waiting for Init() to be called
      };
      
  // What state of playing driving animations we are in
  // Start in ActionDestroyed so that Init() needs to be called
  AnimState _state = AnimState::ActionDestroyed;
      
  Robot* _robot = nullptr;
      
  std::vector<std::pair<DrivingAnimations, std::string>> _drivingAnimationStack;
  DrivingAnimations _currDrivingAnimations;

  const std::map<SimpleMoodType, DrivingAnimations> _moodBasedDrivingAnims;
        
  u32 _actionTag;
  u8 _tracksToUnlock = (u8)AnimTrackFlag::NO_TRACKS;
  bool _isActionLockingTracks = true;
  bool _keepLoopingWithoutPath = false;
      
  std::vector<Signal::SmartHandle> _signalHandles;

  u32 _drivingStartAnimTag = ActionConstants::INVALID_TAG;
  u32 _drivingLoopAnimTag = ActionConstants::INVALID_TAG;
  u32 _drivingEndAnimTag = ActionConstants::INVALID_TAG;
};

}
}

#endif
