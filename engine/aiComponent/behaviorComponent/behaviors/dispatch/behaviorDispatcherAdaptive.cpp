/**
 * File: BehaviorDispatcherAdaptive.cpp
 *
 * Author: Andrew Stout
 * Created: 2019-01-08
 *
 * Description:
 * 2019 R&D Sprint project: Adaptive Behavior Coordinator.
 * Chooses delegates according to a state-action-value function which is updated through very simple reinforcement
 * learning.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherAdaptive.h"

#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/smartFaceId.h"
#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorFindFaceAndThen.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/faceWorld.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/ILastRewardProvider.h"

#define LOG_CHANNEL "BehaviorDispatcherAdaptive"

namespace Anki {
namespace Vector {

namespace {
  const char* kActionSpaceKey = "actionSpace"; // [json config file name of the] array of the behaviors in the action space
  const char* kDefaultBehaviorKey = "defaultBehavior"; // default behavior must wantToBeActivated in all states, and must be interruptable

  const float kActionSelectionWeightEpsilon = 0.01f;
  std::mt19937 rng;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherAdaptive::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherAdaptive::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherAdaptive::BehaviorDispatcherAdaptive(const Json::Value& config)
 : IBehaviorDispatcher(config)
{
  LOG_WARNING("BehaviorDispatcherAdaptive.Constructor.Started", "");
  // read in the action space
  const Json::Value& actionNames = config[kActionSpaceKey];
  DEV_ASSERT_MSG(!actionNames.isNull(),
      "BehaviorDispatcherAdaptive.Constructor.ActionSpaceNotSpecified", "No %s found", kActionSpaceKey);
  if(!actionNames.isNull()) {
    for(const auto& behaviorIDStr: actionNames) { // not currently supporting cooldowns, etc. Could add.
      IBehaviorDispatcher::AddPossibleDispatch(behaviorIDStr.asString());
      // need to maintain the actionSpace separately
      // (default behavior will be last entry of _iConfig.behaviorStrs, but feels risky to count on that.)
      _iConfig.actionSpace.push_back(behaviorIDStr.asString());
    }
  }
  // read in the default behavior
  const Json::Value& defaultBehaviorName = config[kDefaultBehaviorKey];
  DEV_ASSERT_MSG(!defaultBehaviorName.isNull(),
                 "BehaviorDispatcherAdaptive.Constructor.DefaultBehaviorNotSpecified", "No %s found", kDefaultBehaviorKey);
  if(!defaultBehaviorName.isNull()) {
    IBehaviorDispatcher::AddPossibleDispatch(defaultBehaviorName.asString());
    _iConfig.defaultBehaviorName = defaultBehaviorName.asString(); // TODO: do I actually need this?
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherAdaptive::~BehaviorDispatcherAdaptive()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherAdaptive::InitDispatcher()
{
  // TODO: sanity check: check action lists here
  LOG_WARNING("BehaviorDispatcherAdaptive.InitDispatcher.SanityCheckBehaviorNames",
      "behaviorNames:\n");
  for(auto& b: GetAllPossibleDispatches()){
    LOG_WARNING("BehaviorDispatcherAdaptive.InitDispatcher.SanityCheckBehaviorIDs",
        "\t%s\n", BehaviorTypesWrapper::BehaviorIDToString( b->GetID() ) );
  }
  LOG_WARNING("BehaviorDispatcherAdaptive.InitDispatcher.SanityCheckDefaultBehaviorName",
      "_iConfig.defaultBehaviorName: %s", _iConfig.defaultBehaviorName.c_str());
  // get the default behavior (not just the name)
  _iConfig.defaultBehavior = FindBehavior(_iConfig.defaultBehaviorName);

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherAdaptive::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
      kActionSpaceKey,
      kDefaultBehaviorKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
  IBehaviorDispatcher::GetBehaviorJsonKeys(expectedKeys);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherAdaptive::BehaviorDispatcher_OnActivated()
{
  // reset _dVars? Nope, already done in parent class
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherAdaptive::BehaviorDispatcher_OnDeactivated()
{
  // TODO: any learning vars that need to be cleaned up?
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorDispatcherAdaptive::GetDesiredBehavior()
{

  // here's where we choose a new behavior
  // check state
  State state = EvaluateState();
  LOG_WARNING("BehaviorDispatcherAdaptive.GetDesiredBehavior.State",
      "State: %d", state);

  if( (_dVars.lastSelectedBehavior != nullptr) && // not first time running
      (_dVars.lastSelectedBehavior != _iConfig.defaultBehavior) ){  // if we just came from a non-default behavior)

    // do learning update here?

    // have to cast the ICozmoBehavior to a IRewardProvider. TODO: a safer way to do this would be nice
    std::shared_ptr<RewardProvidingBehavior> rewardProvider = std::dynamic_pointer_cast<RewardProvidingBehavior>(_dVars.lastSelectedBehavior);
    float lastReward = rewardProvider->GetLastReward();
    LOG_WARNING("BehaviorDispatcherAdaptive.GetDesiredBehavior.LastReward",
                "reward signal from last action: %f", lastReward);
  }

  ICozmoBehaviorPtr desiredBehavior;
  if (_dVars.lastSelectedBehavior == nullptr || // first time running
      (_dVars.lastSelectedBehavior != _iConfig.defaultBehavior) || // if we just came from a non-default behavior, force doing the default again (state is invalid)
      (state == 0) ) // no recognized face.
  {
    // do default action
    desiredBehavior = _iConfig.defaultBehavior;
  } else {
    // if delegates other than Default are available
    // for now, we assume actions are available if the state is non-zero. TODO: something smarter

    // evaluate state-action value for each available delegate
    // for now, call method to get state action value for each available action,
    // build the "row" of the table for this state on the fly.
    std::vector<float> weights(_iConfig.actionSpace.size());
    for(int i=0; i<_iConfig.actionSpace.size(); ++i) {
      weights[i] = GetStateActionValue(state, _iConfig.actionSpace[i]);
      // add an epsilon so we don't divide by zero, and so things with zero weight still have some chance of selection
      weights[i] += kActionSelectionWeightEpsilon;
    }
    std::discrete_distribution<> d(weights.begin(), weights.end());
    int idx = d(rng);

    // It would probably be more computationally efficient to do this differently in a real implementation.
    // probabilistically select

    desiredBehavior = FindBehavior(_iConfig.actionSpace[idx]);
  }

  LOG_WARNING("BehaviorDispatcherAdaptive.GetDesiredBehavior.Choice",
              "Choosing desired behavior: %s", BehaviorTypesWrapper::BehaviorIDToString(desiredBehavior->GetID()));
  _dVars.lastSelectedBehavior = desiredBehavior;
  if( !desiredBehavior->WantsToBeActivated() ){
    LOG_WARNING("BehaviorDispatcherAdaptive.GetDesiredBehavior.DoesNotWantToBeActivated",
        "desired behavior does not want to be activated. This might go poorly.");
  }
  return desiredBehavior;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherAdaptive::DispatcherUpdate()
{
  // if we've delegated to a behavior other than Default, let it run
  // otherwise...
  // if something other than Default just finished, do a learning update
  // consider checking state again for better identity?
  // how to get outcome of behavior: I think have a method on each behavior to get reward signal
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
State BehaviorDispatcherAdaptive::EvaluateState(){
  // TODO: problem: FFAT's FoundFace data doesn't get cleared until the next time it runs
  // call FFAT's GetFoundFace method
  std::shared_ptr<BehaviorFindFaceAndThen> ffat;
  const BehaviorContainer& container = GetBEI().GetBehaviorContainer();
  container.FindBehaviorByIDAndDowncast<BehaviorFindFaceAndThen>(
      BEHAVIOR_ID(AdaptiveFindFaceAndThen), // TODO: this name needs to be obtained dynamically, but that doesn't work
      BEHAVIOR_CLASS(FindFaceAndThen), ffat);
  SmartFaceID face = ffat->GetFoundFace();
  if ( !face.IsValid() ){
    LOG_WARNING("BehaviorDispatcherAdaptive.EvaluateState.Invalid",
        "face returned by GetFoundFace is no longer valid");
    return 0;
  }
  LOG_WARNING("BehaviorDispatcherAdaptive.EvaluateState.FaceDebugStr",
              "%s", face.GetDebugStr().c_str());
  // the only way I see to get the id from the SmartFaceID is through the debug string, which seems a little crazy
  std::string faceDebugStr = face.GetDebugStr();
  if (faceDebugStr == "<unknown>") {
    return 0;
  }
  int id = std::stoi(faceDebugStr);
  // negative IDs are pre-recognition IDs for tracking. We're going to map all of that to 0.
  if (id < 0) {
    return 0;
  }
  // positive IDs indicate that the face has been matched to a "named" or "session only" face.
  // distinguish between named (enrolled) faces and session only ones
  FaceWorld faceWorld = GetBEI().GetFaceWorld();
  const Vision::TrackedFace* trackedFace = faceWorld.GetFace(face);
  // for now use HasName/GetName. GetBestGuessName is an option if I want to relax assumptions
  if (!trackedFace->HasName()){
    // no name. Call it 0.
    return 0;
  }
  LOG_WARNING("BehaviorDispatcherAdaptive.EvaluateState.Name",
      "Face has name: %s", trackedFace->GetName().c_str());

  // for now, just return.
  return id;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SAValue BehaviorDispatcherAdaptive::GetStateActionValue(State s, Action a){
  // check whether there's an entry for s
  if (_iConfig.SAVTable.find(s) == _iConfig.SAVTable.end()) {
    // if not, initialize it
    _iConfig.SAVTable[s] = std::map< Action, SAValue >();
    // and issue a warning so we know it happened
    LOG_WARNING("BehaviorDispatcherAdaptive.GetStateActionValue.NewState",
        "adding new state %d in SAVTable", s);
  }
  auto& Vs = _iConfig.SAVTable[s];

  // check whether there's an entry in V(s,.) for a
  if (Vs.find(a) == Vs.end()) {
    // if not, initialize it to neutral reward
    Vs[a] = RewardProvidingBehavior::kRewardMid;
    // and issue a warning so we know it happened
    LOG_WARNING("BehaviorDispatcherAdaptive.GetStateActionValue.NewAction",
                "adding new action %s in SAVTable for state %d", a.c_str(), s);
  }
  LOG_WARNING("BehaviorDispatcherAdaptive.GetStateActionValue.Vsa",
      "Vsa(%d, %s): %f", s, a.c_str(), Vs[a]);
  const SAValue Vsa = Vs[a];

  return Vsa;
}


// callback (?)


}
}
