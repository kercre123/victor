/***********************************************************************************************************************
 *
 *  BehaviorObservingSounds
 *  Cozmo
 *
 *  Created by Jarrod Hatfield on 12/06/17.
 *
 *  Description
 *  + This behavior will listen for directional audio and will have Victor face/interact with it in response
 *
 **********************************************************************************************************************/

#ifndef __Cozmo__BehaviorObservingSounds_h__
#define __Cozmo__BehaviorObservingSounds_h__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/micDirectionHistory.h"
#include "clad/types/animationTrigger.h"

#include <vector>


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorObservingSounds : public ICozmoBehavior
{
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorObservingSounds( const Json::Value& config );


public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Data Structure Definitions ...

  struct DirectionTrigger
  {
    MicDirectionHistory::DirectionConfidence  threshold;
  };

  using DirectionTriggerList   = const DirectionTrigger*;

  struct DirectionResponse
  {
    AnimationTrigger                          animation;
    Radians                                   facing; // note: temp until we get anims
  };

  using DirectionResponseList   = const DirectionResponse*;

  enum EChargerStatus
  {
    EChargerStatus_OnCharger,
    EChargerStatus_OffCharger,
    EChargerStatus_Num
  };

  enum EObservationStatus
  {
    EObservationStatus_Asleep,
    EObservationStatus_Awake,
    EObservationStatus_Num
  };


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const override;


protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Behavior Related Functions

  virtual void GetAllDelegates( std::set<IBehavior*>& delegates ) const override { }
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper Functions

  MicDirectionHistory::DirectionNode GetLatestMicDirectionData() const;
  DirectionTrigger GetTriggerData( MicDirectionHistory::DirectionIndex index ) const;
  DirectionResponse GetResponseData( MicDirectionHistory::DirectionIndex index ) const;

  bool HeardValidSound( MicDirectionHistory::DirectionIndex& outIndex ) const;

  void RespondToSound();
  void OnResponseComplete();

  bool CanReactToSound() const;

  TimeStamp_t GetCurrentTime() const;
  TimeStamp_t GetCooldownBeginTime() const;
  TimeStamp_t GetCooldownEndTime() const;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Scoring Functions (not currently active)

//  struct DirectionScore
//  {
//    MicDirectionHistory::DirectionIndex       direction;
//    float                                     score;
//
//    MicDirectionHistory::DirectionConfidence  scoreConfidence;
//    TimeStamp_t                               scoreDuration;
//    TimeStamp_t                               scoreRecent;
//  };
//
//  std::vector<DirectionScore> GetDirectionScores() const;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Data

  static const MicDirectionHistory::DirectionIndex kInvalidDirectionIndex;

  EObservationStatus                    _observationStatus        = EObservationStatus::EObservationStatus_Awake;
  MicDirectionHistory::DirectionIndex   _triggeredDirection       = kInvalidDirectionIndex;

  TimeStamp_t                           _reactionTriggeredTime    = 0;
};

}
}

#endif // __Cozmo__BehaviorObservingSounds_h__
