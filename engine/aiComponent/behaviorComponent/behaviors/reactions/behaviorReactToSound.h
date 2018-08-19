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
namespace Vector {

class BehaviorReactToMicDirection;

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

  void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const override;
  virtual void GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const override;

protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Behavior Related Functions

  virtual void GetAllDelegates( std::set<IBehavior*>& delegates ) const override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void OnBehaviorEnteredActivatableScope() override;
  virtual void OnBehaviorLeftActivatableScope() override;


  virtual void BehaviorUpdate() override;


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper Functions

  void SetTriggerDirection( MicDirectionIndex direction );
  void ClearTriggerDirection();

  MicDirectionNodeList GetLatestMicDirectionData() const;
  DirectionTrigger GetTriggerData( MicDirectionIndex index ) const;

  // callback function for sound reaction
  bool OnMicPowerSampleRecorded( double, MicDirectionConfidence, MicDirectionIndex );

  void RespondToSound();
  void OnResponseComplete();

  bool CanReactToSound() const;

  EngineTimeStamp_t GetCurrentTimeMS() const;


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

  struct InstanceConfig
  {
    InstanceConfig();

    std::string reactionBehaviorString;
    std::shared_ptr<BehaviorReactToMicDirection> reactionBehavior;

    float absolutePowerThreshold;
    float minPowerThreshold;
    MicDirectionConfidence confidenceThresholdAtMinPower;

    SoundReactorId  reactorId;

  } _iVars;

  EObservationStatus                    _observationStatus        = EObservationStatus::EObservationStatus_Awake;

  // these are the values that we're reacting to
  MicDirectionIndex                     _triggeredDirection       = kInvalidMicDirectionIndex;
  double                                _triggeredMicPower        = 0.0;
  MicDirectionConfidence                _triggeredConfidence      = 0;

  EngineTimeStamp_t                     _triggerDetectedTime      = 0;
};

}
}

#endif // __Cozmo__BehaviorReactToSound_h__
