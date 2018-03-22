/**
* File: behaviorReactToCubeTap.h
*
* Author: Jarboo
* Created: 3/15/2018
*
* Description: Reaction behavior when Victor detects a cube being tapped
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Victor_Basestation_Behaviors_BehaviorReactToCubeTap_H__
#define __Victor_Basestation_Behaviors_BehaviorReactToCubeTap_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "coretech/common/engine/objectIDs.h"


namespace Anki {
namespace Cozmo {

class BehaviorDriveOffCharger;

class BehaviorReactToCubeTap : public ICozmoBehavior
{
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  friend class BehaviorFactory;
  BehaviorReactToCubeTap( const Json::Value& config );


public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;


protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  virtual void InitBehavior() override;
  virtual void GetAllDelegates( std::set<IBehavior*>& delegates ) const override;
  virtual void AlwaysHandleInScope( const EngineToGameEvent& event ) override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  struct CubeInfo
  {
    ObjectID      id;
    float         lastTappedTime;
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State Functions

  enum class EState : uint8_t
  {
    Invalid,
    GetOffCharger,
    FindCube,
    GoToCube,
    InteractWithCube,
  };

  void TransitionToGetOffCharger();
  void TransitionToFindCube();
  void TransitionToGoToCube( bool playFoundCubeAnimation );
  void TransitionToInteractWithCube();

  bool IsCubeLocated() const;
  void OnCubeNotFound();
  void DriveToCube( unsigned int attempt = 1 );

private:


  // note: REMOVE
  void DebugFakeCubeTap();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  struct InstanceConfig
  {
    InstanceConfig();

    float                                       cubeInteractionDuration;

    // behavior to drive off of the charger
    std::string                                 chargerBehaviorString;
    std::shared_ptr<BehaviorDriveOffCharger>    chargerBehavior;

    CubeInfo                                    targetCube;

  } _iVars;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  struct DynamicVariables
  {
    DynamicVariables();

    EState                                      state;

  } _dVars;
  
}; // class BehaviorReactToCubeTap

} // namespace Cozmo
} // namespace Anki

#endif // __Victor_Basestation_Behaviors_BehaviorReactToCubeTap_H__
