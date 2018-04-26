/***********************************************************************************************************************
 *
 *  PlaybackMessage
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 4/025/2018
 *
 *  Description
 *  + Parent behavior for allowing the user to playback messages that have been previously recorded to Victor's local
 *    storage.
 *
 **********************************************************************************************************************/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaybackMessage_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaybackMessage_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorPlaybackMessage : public ICozmoBehavior
{
  friend class BehaviorFactory;
  BehaviorPlaybackMessage( const Json::Value& config );


public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Overrides from ICozmoBehavior

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const override;
  virtual void GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const override;


protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Internal Structs


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Overrides from ICozmoBehavior

  virtual void InitBehavior() override;
  virtual void GetAllDelegates( std::set<IBehavior*>& delegates ) const override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Instance Vars are members that last the lifetime of the behavior and generally do not change (config vars)

  struct InstanceConfig
  {
    InstanceConfig();

  } _iVars;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Dynamic Vars are members that change over the lifetime of the behavior and are generally reset every activation

  struct DynamicVariables
  {
    DynamicVariables();

    float         startedRecordingTime; // temp
    std::string   messageRecipient;

  } _dVars;

}; // class BehaviorPlaybackMessage

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaybackMessage_H__
