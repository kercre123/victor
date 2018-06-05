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
#include "util/helpers/fullEnumToValueArrayChecker.h"


namespace Anki {
namespace Cozmo {

class BehaviorDriveOffCharger;
enum class AnimationTrigger : int32_t;

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


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  struct CubeInfo
  {
    ObjectID      id;
    float         lastTappedTime;
  };

  enum class EState : uint8_t
  {
    Invalid,
    GetOffCharger,
    FindCube,
    GoToCube,
    InteractWithCube,
  };

  enum EIntensityLevel
  {
    Level1,
    Level2,
    Level3,

    Count,
    FirstLevel  = Level1,
    LastLevel   = Level3,
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State Functions

  void ReactToCubeTapped();
  void OnCubeTappedWhileActive();

  void TransitionToGetOffCharger();
  void TransitionToFindCube();
  void TransitionToGoToCube( bool playFoundCubeAnimation );
  void TransitionToInteractWithCube();

  bool IsCubeLocated() const;
  void OnCubeNotFound();
  void DriveToTargetCube( unsigned int attempt = 1 );

  AnimationTrigger GetCubeReactionAnimation() const;
  AnimationTrigger GetSearchForCubeAnimation() const;
  

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Static Members

  using CubeIntensityAnimations = Util::FullEnumToValueArrayChecker::FullEnumToValueArray<EIntensityLevel, AnimationTrigger, EIntensityLevel::Count>;
  static const CubeIntensityAnimations kReactToCubeAnimations;
  static const CubeIntensityAnimations kSearchForCubeAnimations;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  struct InstanceConfig
  {
    InstanceConfig();

    float                                       cubeInteractionDuration;

    // behavior to drive off of the charger
    std::string                                 chargerBehaviorString;
    ICozmoBehaviorPtr                           chargerBehavior;

    CubeInfo                                    targetCube;

  } _iVars;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  struct DynamicVariables
  {
    DynamicVariables();

    EState                                      state;
    EIntensityLevel                             intensity;

  } _dVars;
  
}; // class BehaviorReactToCubeTap

} // namespace Cozmo
} // namespace Anki

#endif // __Victor_Basestation_Behaviors_BehaviorReactToCubeTap_H__
