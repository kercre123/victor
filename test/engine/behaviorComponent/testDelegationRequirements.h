/**
 * File: testDelegationRequirements.h
 *
 * Author: ross
 * Created: 2018 September 19
 *
 * Description: Allows behaviors to perform actions prior to being force activated for test purposes
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Test_Engine_BehaviorComponent_TestDelegationRequirements_H__
#define __Test_Engine_BehaviorComponent_TestDelegationRequirements_H__

#include "clad/types/behaviorComponent/behaviorClasses.h"
#include "clad/types/behaviorComponent/behaviorIDs.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"

using namespace Anki;
using namespace Anki::Vector;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<BehaviorClass Class>
void ApplyTestDelegationRequirements( ICozmoBehavior* behavior, TestBehaviorFramework& tbf ) {}

template<BehaviorID ID>
void ApplyTestDelegationRequirements( ICozmoBehavior* behavior, TestBehaviorFramework& tbf ) {}

template<BehaviorID ID>
void RemoveTestDelegationRequirements( ICozmoBehavior* behavior,
                                       TestBehaviorFramework& tbf,
                                       const std::vector<bool>& cachedVals ) {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Put your TEST delegation requirements here. Feel free to make a separate file that is included by this one.
// You'll also have to add to the switch in testBehaviorFramework.h. If these requirements are needed for
// dev reasons beyond of unit tests, use devDelegationRequirements.h

template <>
void ApplyTestDelegationRequirements<BehaviorClass::AnimSequenceWithFace>( ICozmoBehavior* behavior,
                                                                           TestBehaviorFramework& tbf )
{
  tbf.AddFakeFirstFace();
}
 
template <>
void ApplyTestDelegationRequirements<BehaviorClass::ClearChargerArea>( ICozmoBehavior* behavior,
                                                                       TestBehaviorFramework& tbf )
{
  // does nothing if there's already a charger
  tbf.AddFakeFirstObject( ObjectType::Charger_Basic );
  // this won't do anything if a cube already exists, but we want to add one in front of the charger,
  // so this may still fail if a cube already exists but is somewhere else
  Anki::Pose3d inFrontOfChargerPose;
  inFrontOfChargerPose.SetParent( tbf._robot->GetWorldOrigin() );
  inFrontOfChargerPose.TranslateForward(20.0f);
  tbf.AddFakeFirstObject( ObjectType::Block_LIGHTCUBE1, &inFrontOfChargerPose );
}
  
template <>
void ApplyTestDelegationRequirements<BehaviorClass::GoHome>( ICozmoBehavior* behavior,
                                                             TestBehaviorFramework& tbf )
{
  tbf.AddFakeFirstObject( ObjectType::Charger_Basic );
}
  
template <>
void ApplyTestDelegationRequirements<BehaviorClass::VectorPlaysCubeSpinner>( ICozmoBehavior* behavior,
                                                                             TestBehaviorFramework& tbf )
{
  auto* blockWorld = tbf._robot->GetComponentPtr<BlockWorld>();
  blockWorld->AddConnectedActiveObject(0, "AA:AA:AA:AA:AA:AA", ObjectType::Block_LIGHTCUBE1);
}
  
template <>
void ApplyTestDelegationRequirements<BehaviorID::DriveOffChargerFace>( ICozmoBehavior* behavior,
                                                                       TestBehaviorFramework& tbf )
{
  const auto& idStr = BehaviorIDToString(behavior->GetID());
  DEV_ASSERT( tbf._cachedDelegationReqs.find(idStr) == tbf._cachedDelegationReqs.end(), "" );
  // robot must be on charger
  tbf._cachedDelegationReqs[idStr] = { tbf._robot->GetBatteryComponent().IsOnChargerContacts() };
  tbf._robot->GetBatteryComponent().SetOnChargeContacts(true);
  tbf._robot->GetBatteryComponent().UpdateOnChargerPlatform();
}


template <>
void ApplyTestDelegationRequirements<BehaviorID::Anonymous>( ICozmoBehavior* behavior,
                                                             TestBehaviorFramework& tbf )
{
  const std::string& debugLabel = behavior->GetDebugLabel();
  if( debugLabel.find("PRDemoObservingOnCharger") != std::string::npos ) {
    DEV_ASSERT( tbf._cachedDelegationReqs.find(debugLabel) == tbf._cachedDelegationReqs.end(), "" );
    // robot must be on charger
    tbf._cachedDelegationReqs[debugLabel] = { tbf._robot->GetBatteryComponent().IsOnChargerContacts()};
    tbf._robot->GetBatteryComponent().SetOnChargeContacts(true);
    tbf._robot->GetBatteryComponent().UpdateOnChargerPlatform();
  }
  DEV_ASSERT( tbf._cachedDelegationReqs.find("Anonymous") == tbf._cachedDelegationReqs.end(), "" );
}

template <>
void RemoveTestDelegationRequirements<BehaviorID::DriveOffChargerFace>( ICozmoBehavior* behavior,
                                                                        TestBehaviorFramework& tbf,
                                                                        const std::vector<bool>& cachedVals )
{
  tbf._robot->GetBatteryComponent().SetOnChargeContacts( cachedVals[0] );
  tbf._robot->GetBatteryComponent().UpdateOnChargerPlatform();
}

template <>
void RemoveTestDelegationRequirements<BehaviorID::Anonymous>( ICozmoBehavior* behavior,
                                                              TestBehaviorFramework& tbf,
                                                              const std::vector<bool>& cachedVals )
{
  const std::string& debugLabel = behavior->GetDebugLabel();
  if( debugLabel.find("PRDemoObservingOnCharger") != std::string::npos ) {
    tbf._robot->GetBatteryComponent().SetOnChargeContacts( cachedVals[0] );
    tbf._robot->GetBatteryComponent().UpdateOnChargerPlatform();
  }
}


#endif // __Test_Engine_BehaviorComponent_TestDelegationRequirements_H__
