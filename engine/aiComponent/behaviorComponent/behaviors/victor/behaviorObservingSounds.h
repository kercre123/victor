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

  virtual bool WantsToBeActivatedBehavior( BehaviorExternalInterface& behaviorExternalInterface ) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false; }


protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Behavior Related Functions

  virtual void GetAllDelegates( std::set<IBehavior*>& delegates ) const override { }
  virtual void InitBehavior( BehaviorExternalInterface& behaviorExternalInterface ) override;
  
  virtual Result OnBehaviorActivated( BehaviorExternalInterface& behaviorExternalInterface ) override;

  virtual void BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning( BehaviorExternalInterface& behaviorExternalInterface ) override;



private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper Functions

  void UpdateSoundDirection( const BehaviorExternalInterface& behaviorExternalInterface );
  void TurnTowardsSound( MicDirectionHistory::DirectionIndex soundDirection );

  MicDirectionHistory::DirectionIndex GetFocusDirection( const BehaviorExternalInterface& behaviorExternalInterface ) const;
  bool IsSameDirection( MicDirectionHistory::DirectionIndex lhs, MicDirectionHistory::DirectionIndex rhs ) const;

  TimeStamp_t GetCurrentTime() const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Scoring Functions

  struct DirectionScore
  {
    MicDirectionHistory::DirectionIndex       direction;
    float                                     score;

    MicDirectionHistory::DirectionConfidence  scoreConfidence;
    TimeStamp_t                               scoreDuration;
    TimeStamp_t                               scoreRecent;
  };

  std::vector<DirectionScore> GetDirectionScores( const BehaviorExternalInterface& behaviorExternalInterface ) const;
  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Data

  MicDirectionHistory::DirectionIndex   _focusDirection;
  TimeStamp_t                           _focusLastChangedTime   = 0;
  bool                                  _shouldProcessSound     = true;
};

}
}

#endif // __Cozmo__BehaviorObservingSounds_h__
