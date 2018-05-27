/**
 * File: BehaviorMakeNoise.cpp
 *
 * Author: ross
 * Created: 2018-05-25
 *
 * Description: moves all motors
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorMakeNoise.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
CONSOLE_VAR_ENUM(int, kFunction, "AAA", 0, "NoNoise,OnlyLift,OnlyBody,LiftAndBody");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorMakeNoise::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorMakeNoise::DynamicVariables::DynamicVariables()
{
  function = static_cast<Function>(kFunction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorMakeNoise::BehaviorMakeNoise(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorMakeNoise::~BehaviorMakeNoise()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorMakeNoise::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMakeNoise::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false; // if NoNoise is function, this behavior is like Wait.
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMakeNoise::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMakeNoise::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
//  const char* list[] = {
//    // TODO: insert any possible root-level json keys that this class is expecting.
//    // TODO: replace this method with a simple {} in the header if this class doesn't use the ctor's "config" argument.
//  };
//  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMakeNoise::OnBehaviorActivated()
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  MakeNoise();
}
  
void BehaviorMakeNoise::MakeNoise()
{
  if( _dVars.function != Function::NoNoise ) {
    
    // use unique_ptrs in case the _dVars.function doesnt need this action
    
    std::unique_ptr<CompoundActionSequential> bodyAction;
    bodyAction.reset(new CompoundActionSequential());
    {
      for( int i=0; i<100; ++i ) {
        float angle = 0.99*M_PI_F;
        if( i & 1 ) {
          angle = -angle;
        }
        bodyAction->AddAction( new TurnInPlaceAction( angle, false ) );
      }
    }
    
    std::unique_ptr<CompoundActionSequential> liftAction;
    liftAction.reset(new CompoundActionSequential());
    {
      for( int i=0; i<100; ++i ) {
        MoveLiftToHeightAction::Preset preset = MoveLiftToHeightAction::Preset::HIGH_DOCK;
        if( i & 1 ) {
          preset = MoveLiftToHeightAction::Preset::LOW_DOCK;
        }
        liftAction->AddAction( new MoveLiftToHeightAction( preset ) );
      }
    }
    
    auto* action = new CompoundActionParallel();
    
    if( _dVars.function == Function::OnlyLift || _dVars.function == Function::LiftAndBody ) {
      action->AddAction(liftAction.release());
    }
    
    if( _dVars.function == Function::OnlyBody || _dVars.function == Function::LiftAndBody ) {
      action->AddAction(bodyAction.release());
    }
    
    DelegateIfInControl( action, &BehaviorMakeNoise::MakeNoise );
    //DelegateIfInControl( bodyAction.get(), &BehaviorMakeNoise::MakeNoise );
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMakeNoise::BehaviorUpdate()
{
  if( !IsActivated() ) {
    return;
  }
  
  if( kFunction != (int)_dVars.function ) {
    _dVars.function = static_cast<Function>(kFunction);
    CancelDelegates(false);
    MakeNoise();
  }
  
  // todo: if one of the running actions has finished, restart it
}

}
}
