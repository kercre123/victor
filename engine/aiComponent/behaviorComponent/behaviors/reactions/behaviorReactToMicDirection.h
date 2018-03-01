/***********************************************************************************************************************
 *
 *  BehaviorReactToMicDirection
 *  Cozmo
 *
 *  Created by Jarrod Hatfield on 12/06/17.
 *
 *  Description
 *  + This behavior will listen for directional audio and will have Victor face/interact with it in response
 *
 **********************************************************************************************************************/

#ifndef __Cozmo__BehaviorReactToMicDirection_h__
#define __Cozmo__BehaviorReactToMicDirection_h__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/micDirectionHistory.h"

#include "util/helpers/fullEnumToValueArrayChecker.h"
#include "util/helpers/templateHelpers.h"
#include <vector>


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorReactToMicDirection : public ICozmoBehavior
{
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToMicDirection( const Json::Value& config );


public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Class specific types ...

  // data corresponding to our response to a valid trigger
  struct DirectionResponse
  {
    AnimationTrigger                          animation;
    float                                     facing; // fallback in case we want procedural turning
  };


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  virtual ~BehaviorReactToMicDirection();

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const override;

  void SetReactDirection( MicDirectionHistory::DirectionIndex dir );
  void ClearReactDirection();


protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Behavior Related Functions

  virtual void GetAllDelegates( std::set<IBehavior*>& delegates ) const override { }
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Internal Data Structure Definitions ...

  // Corresponds direction with MicDirectionHistory::DirectionIndex but gives data
  // a bit more readability
  enum EClockDirection
  {
    TwelveOClock,
    OneOClock,
    TwoOClock,
    ThreeOClock,
    FourOClock,
    FiveOClock,
    SixOClock,
    SevenOClock,
    EightOClock,
    NineOClock,
    TenOClock,
    ElevenOClock,

    NumDirections,
    Invalid = NumDirections,
  };

  // For each of these possible states, we need to be able to (possibly) react differently
  // eg. lock treads if being held
  enum EDynamicReactionState
  {
    OnSurface,
    OnCharger,
    OffTreads,

    Num,
  };

  struct DynamicStateReaction
  {
    DirectionResponse      directionalResponseList[EClockDirection::NumDirections];
  };

  static constexpr float kRadsPerDirection = ( ( 2.0f * M_PI_F ) / (float)EClockDirection::NumDirections );
  static const DynamicStateReaction kFallbackReaction;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Functions

  // Loading ...
  void LoadDynamicReactionStateData( const Json::Value& );
  void LoadDynamicStateReactions( const Json::Value&, DynamicStateReaction& );
  void LoadDirectionResponse( const Json::Value&, DirectionResponse& );

  const DirectionResponse& GetResponseData( EClockDirection dir ) const;

  // Helpers ...
  EClockDirection ConvertMicDirectionToClockDirection( MicDirectionHistory::DirectionIndex dir ) const;
  std::string ConvertReactionStateToString( EDynamicReactionState state ) const;
  std::string ConvertClockDirectionToString( EClockDirection dir ) const;

  void RespondToSound();
  void OnResponseComplete();


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Data

  struct InstanceConfig
  {
    // list of the reactions
    DynamicStateReaction defaultReaction;
    DynamicStateReaction* reactions[EDynamicReactionState::Num];
  } _instanceVars;

  struct DynamicVariables
  {
    DynamicVariables();
    
    // this is the direction we've been told to react to
    EClockDirection   reactionDirection;
  } _dynamicVars;
};

}
}

#endif // __Cozmo__BehaviorReactToMicDirection_h__
