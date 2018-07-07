/***********************************************************************************************************************
 *
 *  BehaviorReactToSound
 *  Cozmo
 *
 *  Created by Jarrod Hatfield on 12/06/17.
 *
 *  Description
 *  + This behavior will listen for directional audio and will have Victor face/interact with it in response
 *
 **********************************************************************************************************************/

#ifndef __Cozmo__BehaviorReactToSound_h__
#define __Cozmo__BehaviorReactToSound_h__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/mics/micDirectionTypes.h"

#include <vector>


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorReactToSound : public ICozmoBehavior
{
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToSound( const Json::Value& config );


public:

  // data corresponding to what sounds will trigger a reaction
  struct DirectionTrigger
  {
    MicDirectionConfidence  threshold;
  };

  // data corresponding to our response to a valid trigger
  struct DirectionResponse
  {
    AnimationTrigger                          animation;
    Radians                                   facing; // note: temp until we get anims
  };


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const override;
  virtual void GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const override;

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

  MicDirectionNodeList GetLatestMicDirectionData() const;
  DirectionTrigger GetTriggerData( MicDirectionIndex index ) const;
  DirectionResponse GetResponseData( MicDirectionIndex index ) const;

  void OnHeardValidSound( MicDirectionIndex index );
  bool HeardValidSound( MicDirectionIndex& outIndex ) const;

  void RespondToSound();
  void OnResponseComplete();

  bool CanReactToSound() const;

  TimeStamp_t GetCurrentTimeMS() const;
  TimeStamp_t GetCooldownBeginTime() const;
  TimeStamp_t GetCooldownEndTime() const;
  // the earliest timestamp that we'll respond to a sound
  TimeStamp_t GetReactionWindowBeginTime() const;
  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Internal Data Structure Definitions ...

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
  // Member Data


  static const MicDirectionIndex kInvalidDirectionIndex;

  EObservationStatus                    _observationStatus        = EObservationStatus::EObservationStatus_Awake;
  MicDirectionIndex                     _triggeredDirection       = kInvalidDirectionIndex;

  TimeStamp_t                           _reactionTriggeredTime    = 0;
  TimeStamp_t                           _reactionEndedTime        = 0;
};

}
}

#endif // __Cozmo__BehaviorReactToSound_h__
