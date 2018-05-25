/**
 * File: behaviorConfirmObject.cpp
 *
 * Author: ross
 * Created: 2018-04-18
 *
 * Description: Runs only if an object position is known for a given object type(s), drives to where
 *              it should be, and ends when it (or any other object of the given type[s]) is seen. If it
 *              is not seen and there are multiple objects of the given type(s) in blockworld, it will
 *              sequentially try them all starting with the closest
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorConfirmObject.h"
#include "coretech/vision/engine/observableObject.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectKnown.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  

namespace {
  const char* kObjectTypesKey = "objectTypes";
  const char* kMinAgeForActivationKey = "minAgeForActivation_s";
  const char* kMaxAttemptsKey = "maxNumAttempts";
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorConfirmObject::InstanceConfig::InstanceConfig()
{
  maxAttempts = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorConfirmObject::DynamicVariables::DynamicVariables()
{
  shouldActivate = false;
  shouldCancel = false;
  numAttempts = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorConfirmObject::BehaviorConfirmObject(const Json::Value& config)
 : ICozmoBehavior(config)
{
  _iConfig.maxAttempts = config.get( kMaxAttemptsKey, -1 ).asInt();
  DEV_ASSERT( _iConfig.maxAttempts == -1 || _iConfig.maxAttempts > 0, "Invalid param" );
  
  // create the 3 conditions (or just two if min age isnt specified)
  Json::Value conditionConfig = IBEICondition::GenerateBaseConditionConfig( BEIConditionType::ObjectKnown );
  conditionConfig[kObjectTypesKey] = config[kObjectTypesKey];
  _iConfig.objectKnownCondition.reset( new ConditionObjectKnown(conditionConfig) );
  _iConfig.objectKnownCondition->SetOwnerDebugLabel( GetDebugLabel() );
  
  if( config[kMinAgeForActivationKey].isDouble() ) {
    const float minAge_ms = config.get( kMinAgeForActivationKey, 0.0f ).asFloat();
    
    conditionConfig["maxAge_ms"] = minAge_ms;
    _iConfig.objectKnownRecentlyCondition.reset( new ConditionObjectKnown(conditionConfig) );
    _iConfig.objectKnownRecentlyCondition->SetOwnerDebugLabel( GetDebugLabel() );
  }
  
  // NOTE: Some specifics of what happens if you provide maxAge_ms:
  // If you provide multiple object types:
  //   If types A and B are known, and type A hasnt been seen recently, but type B has, within
  //   maxAge_ms, this behavior won't activate
  // Similarly, if you provide one object type, but there are multiple objects of that type:
  //   If objects A and B are known, and object A hasnt been seen recently, but object B has, within
  //   maxAge_ms, this behavior won't activate
  conditionConfig["maxAge_ms"] = 0;
  _iConfig.objectJustSeen.reset( new ConditionObjectKnown(conditionConfig) );
  _iConfig.objectJustSeen->SetOwnerDebugLabel( GetDebugLabel() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorConfirmObject::~BehaviorConfirmObject()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConfirmObject::InitBehavior()
{
  _iConfig.objectKnownCondition->Init( GetBEI() );
  _iConfig.objectJustSeen->Init( GetBEI() );
  if( _iConfig.objectKnownRecentlyCondition != nullptr ) {
    _iConfig.objectKnownCondition->Init( GetBEI() );
  }
}
  
void BehaviorConfirmObject::OnBehaviorEnteredActivatableScope()
{
  _iConfig.objectKnownCondition->SetActive( GetBEI(), true );
  _iConfig.objectJustSeen->SetActive( GetBEI(), true );
  if( _iConfig.objectKnownRecentlyCondition != nullptr ) {
    _iConfig.objectKnownCondition->SetActive( GetBEI(), true );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConfirmObject::OnBehaviorLeftActivatableScope()
{
  _iConfig.objectKnownCondition->SetActive( GetBEI(), false );
  _iConfig.objectJustSeen->SetActive( GetBEI(), false );
  if( _iConfig.objectKnownRecentlyCondition != nullptr ) {
    _iConfig.objectKnownCondition->SetActive( GetBEI(), false );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorConfirmObject::WantsToBeActivatedBehavior() const
{
  return _dVars.shouldActivate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConfirmObject::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConfirmObject::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kObjectTypesKey,
    kMinAgeForActivationKey,
    kMaxAttemptsKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConfirmObject::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  // get the objects that triggered it
  std::vector<const ObservableObject*> knownObjects = _iConfig.objectKnownCondition->GetObjects( GetBEI() );
  if( _iConfig.objectKnownRecentlyCondition != nullptr ) {
    std::vector<const ObservableObject*> recentObjects = _iConfig.objectKnownRecentlyCondition->GetObjects( GetBEI() );
    // remove all of recentObjects from knownObjects
    for( const auto& recentObject : recentObjects ) {
      for( ssize_t i=knownObjects.size()-1; i>=0; --i ) {
        if( knownObjects[i] == recentObject ) {
          std::swap( knownObjects[i], knownObjects.back() );
          knownObjects.pop_back();
        }
      }
    }
  }
  
  if( knownObjects.empty() ) {
    // (can happen in tests)
    PRINT_NAMED_WARNING( "BehaviorConfirmObject.OnBehaviorActivated.Empty",
                         "The behavior activated but has no objects" );
    _dVars.shouldCancel = true;
    return;
  }
  
  // since DriveToObjectAction only takes one object, find the closest one, but keep all the rest
  // in the same vector, sorted, with the closest at the end
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  _dVars.sortedObjects = knownObjects;
  std::sort( _dVars.sortedObjects.begin(), _dVars.sortedObjects.end(), [&robotPose](const auto* a, const auto* b) {
    const auto& poseA = a->GetPose();
    const auto& poseB = b->GetPose();
    const float distA = ComputeDistanceBetween( poseA, robotPose );
    const float distB = ComputeDistanceBetween( poseB, robotPose );
    return distA > distB; // descending order
  });
  
  DriveToClosestKnownObject();
}
  
void BehaviorConfirmObject::DriveToClosestKnownObject()
{
  ++_iConfig.maxAttempts;
  // closest object
  ObjectID closestObject = _dVars.sortedObjects.back()->GetID();
  
  // docking is what's used for cubes and charger, so we'll use it for all object types
  auto* driveToAction = new DriveToObjectAction( closestObject, PreActionPose::ActionType::DOCKING );
  DelegateIfInControl( driveToAction, [this](ActionResult result ) {
    // if we're here and the behavior wasnt already canceled, it means we either
    // arrived at the destination and didnt see an object, or driving to the pose
    // failed. in either case, pop that object from the list and try the next one
    _dVars.sortedObjects.pop_back();
    const bool exceededAttempts = _dVars.numAttempts >= _iConfig.maxAttempts;
    if( exceededAttempts || _dVars.sortedObjects.empty() ) {
      // todo: a good spot for a config'd animation
      CancelSelf();
    } else {
      // todo: a good spot for a config'd animation
      DriveToClosestKnownObject();
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConfirmObject::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    
    const bool known = _iConfig.objectKnownCondition->AreConditionsMet( GetBEI() );
    bool seenRecently = false;
    if( _iConfig.objectKnownRecentlyCondition != nullptr ) {
      seenRecently = _iConfig.objectKnownRecentlyCondition->AreConditionsMet( GetBEI() );
    }
    const bool oldEnoughKnownObjects = known && !seenRecently;
    const bool seeingNow = _iConfig.objectJustSeen->AreConditionsMet( GetBEI() );
    _dVars.shouldActivate = oldEnoughKnownObjects && !seeingNow;
    
  } else {
    
    if( _dVars.shouldCancel ) {
      CancelSelf();
      return;
    }
    
    const bool seeingNow = _iConfig.objectJustSeen->AreConditionsMet( GetBEI() );
    if( seeingNow ) {
      CancelSelf();
      // todo: a good spot for a config'd animation
      return;
    }
  }
  
}

}
}

