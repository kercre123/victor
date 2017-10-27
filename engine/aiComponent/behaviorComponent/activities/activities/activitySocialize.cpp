/**
* File: ActivitySocialize.h
*
* Author: Kevin M. Karol
* Created: 04/27/17
*
* Description: Activity for cozmo to interact with the user's face
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorComponent/activities/activities/activitySocialize.h"

#include "anki/common/basestation/jsonTools.h"
#include "engine/ankiEventUtil.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/exploration/behaviorExploreLookAroundInPlace.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/robot.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PotentialObjectives::PotentialObjectives(const Json::Value& config)
{
  const std::string& objectiveStr = JsonTools::ParseString(config, "objective",
                                                           "FPSocialize.ObjectiveRequirement.InvalidConfig.NoObjective");
  objective = BehaviorObjectiveFromString(objectiveStr.c_str());
  
  std::string unlockStr;
  if( JsonTools::GetValueOptional(config, "ignoreIfLocked", unlockStr) ) {
    requiredUnlock = UnlockIdFromString(unlockStr);
  }

  probabilityToRequire = config.get("probabilityToRequireObjective", 1.0f).asFloat();
  randCompletionsMin = config.get("randomCompletionsNeededMin", 1).asUInt();
  randCompletionsMax = config.get("randomCompletionsNeededMax", 1).asUInt();
  behaviorID = ICozmoBehavior::ExtractBehaviorIDFromConfig(config);
  
  DEV_ASSERT(randCompletionsMax >= randCompletionsMin, "FPSocialize.ObjectiveRequirement.InvalidConfig.MaxLTMin");
  DEV_ASSERT(FLT_GE_ZERO( probabilityToRequire ), "FPSocialize.ObjectiveRequirement.InvalidConfig.NegativeProb");
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -`
ActivitySocialize::PotentialObjectivesList
          ActivitySocialize::ReadPotentialObjectives(const Json::Value& config)
{
  PotentialObjectivesList requirementList;
  
  const Json::Value& requirements = config["requiredObjectives"];
  if( !requirements.isNull() ) {
    for( const auto& objectiveConfig : requirements ) {
      requirementList.emplace_back( new PotentialObjectives(objectiveConfig) );
    }
  }
  
  return requirementList;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivitySocialize::ActivitySocialize(const Json::Value& config)
: IActivity(config)
, _potentialObjectives( ReadPotentialObjectives( config ) )
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySocialize::InitActivity(BehaviorExternalInterface& behaviorExternalInterface)
{
  // choosers and activities are created after the behaviors are added to the factory, so grab those now
  const BehaviorContainer& BC = behaviorExternalInterface.GetBehaviorContainer();
  ICozmoBehaviorPtr facesBehavior = BC.FindBehaviorByID(BehaviorID::FindFaces_socialize);
  assert(std::static_pointer_cast<BehaviorExploreLookAroundInPlace>(facesBehavior));
  _findFacesBehavior = std::static_pointer_cast<BehaviorExploreLookAroundInPlace>(facesBehavior);
  DEV_ASSERT(nullptr != _findFacesBehavior, "FPSocializeBehaviorChooser.MissingBehavior.FindFaces");
  
  _interactWithFacesBehavior = BC.FindBehaviorByID(BehaviorID::InteractWithFaces);
  DEV_ASSERT(nullptr != _interactWithFacesBehavior, "FPSocializeBehaviorChooser.MissingBehavior.InteractWithFaces");
  
  // defaults to 0 to mean allow infinite iterations
  _maxNumIterationsToAllowForSearch = _config.get("maxNumFindFacesSearchIterations", 0).asUInt();
  
  
  behaviorExternalInterface.GetStateChangeComponent().SubscribeToTags(this,
      {
        ExternalInterface::MessageEngineToGameTag::BehaviorObjectiveAchieved
      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySocialize::OnActivatedActivity(BehaviorExternalInterface& behaviorExternalInterface)
{
  // we always want to do the search first, if possible
  _state = State::Initial;
  PopulatePotentialObjectives(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ActivitySocialize::Update_Legacy(BehaviorExternalInterface& behaviorExternalInterface) {
  const auto& stateChangeComp = behaviorExternalInterface.GetStateChangeComponent();
  for(const auto& event: stateChangeComp.GetEngineToGameEvents()){
    if(event.GetData().GetTag() == ExternalInterface::MessageEngineToGameTag::BehaviorObjectiveAchieved){
      // transition out of the interacting state if needed
      auto& msg = event.GetData().Get_BehaviorObjectiveAchieved();
      if( _state == State::Interacting && msg.behaviorObjective == BehaviorObjective::InteractedWithFace ) {
        PRINT_CH_INFO("Behaviors", "SocializeBehaviorChooser.GotInteraction",
                      "Got interacted objective, advancing to next behavior");
        _state = State::FinishedInteraction;
        return Result::RESULT_OK;
      }
      
      // update objective counts needed
      int numObjectivesRemaining = Util::numeric_cast<int>(_objectivesLeft.size());
      auto objectiveIt = _objectivesLeft.find(msg.behaviorObjective);
      if( objectiveIt != _objectivesLeft.end() ) {
        DEV_ASSERT(objectiveIt->second > 0, "FPSocializeStrategy.HandleMessage.CorruptObjectiveData");
        
        objectiveIt->second--;
        numObjectivesRemaining = objectiveIt->second;
        if( objectiveIt->second == 0 ) {
          _objectivesLeft.erase( objectiveIt );
        }
      }
      
      PrintDebugObjectivesLeft("FPSocialize.HandleObjectiveAchieved.StillLeft");
      // _objectivesLeft might contain other objectives we decided not to do.
      if( numObjectivesRemaining == 0 && _state == State::Playing ) {
        PRINT_CH_INFO("Behaviors", "SocializeBehaviorChooser.FinishedPlaying",
                      "Got enough objectives to be done with pouncing, will transition out");
        if( _playingBehavior != nullptr && _playingBehavior->IsActivated() )
        {
          // tell the behavior to end nicely (when it's in control again)
          _playingBehavior->StopOnNextActionComplete();
        }
        
        _state = State::FinishedPlaying;
      }

    }
  }
  
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr ActivitySocialize::GetDesiredActiveBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr currentRunningBehavior)
{
  ICozmoBehaviorPtr bestBehavior;
  
  bestBehavior = IActivity::GetDesiredActiveBehaviorInternal(behaviorExternalInterface, currentRunningBehavior);
  
  if(bestBehavior != nullptr) {
    if( bestBehavior != currentRunningBehavior ) {
      PRINT_CH_INFO("Behaviors", "SocializeBehaviorChooser.ChooseNext.UseSimple",
                    "Simple behavior chooser chose behavior '%s', so use it",
                    BehaviorIDToString(bestBehavior->GetID()));
    }
    return bestBehavior;
  }

  // otherwise, check if it's time to change behaviors
  switch( _state ) {
    case State::Initial: {
      if( _interactWithFacesBehavior->WantsToBeActivated(behaviorExternalInterface) ) {
        // if we can just right to interact, do that
        bestBehavior = _interactWithFacesBehavior;
        _state = State::Interacting;
      }
      else if( _findFacesBehavior->WantsToBeActivated(behaviorExternalInterface) ) {
        // otherwise, search for a face
        bestBehavior = _findFacesBehavior;
        _state = State::FindingFaces;
        _lastNumSearchIterations = _findFacesBehavior->GetNumIterationsCompleted();
      }
      break;
    }
    
    case State::FindingFaces: {
      if( _interactWithFacesBehavior->WantsToBeActivated(behaviorExternalInterface) ) {
        bestBehavior = _interactWithFacesBehavior;
        _state = State::Interacting;
      }
      else if( _maxNumIterationsToAllowForSearch > 0 &&
              _findFacesBehavior->GetNumIterationsCompleted() - _lastNumSearchIterations >=
              _maxNumIterationsToAllowForSearch ) {
        
        // NOTE: this is different from setting "behavior_NumberOfScansBeforeStop" in the find faces behavior,
        // because this will only transition out of the FindingFaces state if we _actually_ complete the
        // scans, whereas if we just set behavior_NumberOfScansBeforeStop = 2, the behavior may end for any of
        // a number of reasons (e.g. interruption)
        
        // we ran out of time searching, give up on this activity
        PRINT_CH_INFO("Behaviors", "SocializeBehaviorChooser.CompletedSearchIterations",
                      "Finished %d search iterations, giving up on activity",
                      _findFacesBehavior->GetNumIterationsCompleted() - _lastNumSearchIterations);
        // TODO:(bn) ideally this wouldn't put socialize on cooldown, but that's hard to implement in the
        // current system
        bestBehavior = nullptr;
        _state = State::None;
      }
      else if( _findFacesBehavior->IsActivated() || _findFacesBehavior->WantsToBeActivated(behaviorExternalInterface) ) {
        bestBehavior = _findFacesBehavior;
      }
      break;
    }
    
    case State::Interacting: {
      // keep interacting until the behavior ends. If we can't interact (e.g. we lost the face) then go back
      // to searching
      if( _interactWithFacesBehavior->IsActivated() || _interactWithFacesBehavior->WantsToBeActivated(behaviorExternalInterface) ) {
        bestBehavior = _interactWithFacesBehavior;
      }
      else {
        // go back to find, but don't reset search count
        if( _findFacesBehavior->IsActivated() || _findFacesBehavior->WantsToBeActivated(behaviorExternalInterface) ) {
          bestBehavior = _findFacesBehavior;
        }        
        _state = State::FindingFaces;
      }
      break;
    }
    
    case State::FinishedInteraction: {
      bool needsToPlay = false;
      // Has objectives we might want to do
      if( !_objectivesLeft.empty() )
      {
        std::vector<ICozmoBehaviorPtr> wantsActivatedBehaviors;
        // fill in _playingBehavior with one of the valid objectives
        for( const auto& reqPtr : _potentialObjectives )
        {
          // Check if behavior objective found
          if( _objectivesLeft.find(reqPtr->objective) != _objectivesLeft.end() )
          {
            const BehaviorContainer& BC = behaviorExternalInterface.GetBehaviorContainer();
            ICozmoBehaviorPtr beh = BC.FindBehaviorByID(reqPtr->behaviorID);
            if( beh != nullptr )
            {
              // Check if activatable and valid.
              if( beh->WantsToBeActivated(behaviorExternalInterface) )
              {
                wantsActivatedBehaviors.push_back(beh);
                PRINT_CH_INFO("Behaviors", "SocializeBehaviorChooser.FinishedInteraction",
                              "%s is activatable", beh->GetIDStr().c_str());
              }
              else
              {
                PRINT_CH_INFO("Behaviors", "SocializeBehaviorChooser.FinishedInteraction",
                              "%s is NOT activatable", beh->GetIDStr().c_str());
              }
            }
          }
        }
        
        if(!wantsActivatedBehaviors.empty())
        {
          int index = behaviorExternalInterface.GetRNG().RandIntInRange( 0, Util::numeric_cast<int>(wantsActivatedBehaviors.size()) - 1);
          _playingBehavior = wantsActivatedBehaviors[index];
          needsToPlay = true;
        }
      }
      
      if( nullptr != _playingBehavior && needsToPlay ) {
        // should be activatable since it was just selected in the code above
        DEV_ASSERT( _playingBehavior->WantsToBeActivated(behaviorExternalInterface), "ActivitySocialize.PlayingBehavior.NotActivatable");
        bestBehavior = _playingBehavior; 
        _lastNumTimesPlayStarted = _playingBehavior->GetNumTimesBehaviorStarted();
        _state = State::Playing;
      }
      else {
        _state = State::None;
      }
      break;
    }
    
    case State::Playing: {
      
      if( currentRunningBehavior == nullptr ) {
        
        // current being null means the playing behavior may have stopped, or maybe a reactionary behavior
        // ran, so check how many times play started. If it has actually started since we entered this
        // state, the assume we are done playing once it stops
        
        const bool hasPlayBehaviorStarted =
        _playingBehavior->GetNumTimesBehaviorStarted() > _lastNumTimesPlayStarted;
        if( hasPlayBehaviorStarted ) {
          // play behavior stopped for some reason.. finished
          _state = State::None;
          break;
        }
      }
      
      if( _playingBehavior->IsActivated() || _playingBehavior->WantsToBeActivated(behaviorExternalInterface) ) {
        bestBehavior = _playingBehavior;
      }
      break;
    }
    
    case State::FinishedPlaying: {
      // At this point, we've told the pouncing behavior to stop after it's current action, so let it run
      // until that happens to avoid a harsh cut
      if( currentRunningBehavior == _playingBehavior && _playingBehavior->IsActivated() ) {
        // keep it going while it's running, let it stop itself
        bestBehavior = _playingBehavior;
      }
      break;
    }        
    
    case State::None:
    break;
  }

  return bestBehavior;      
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySocialize::PopulatePotentialObjectives(BehaviorExternalInterface& behaviorExternalInterface)
{
  _objectivesLeft.clear();
  
  for( const auto& reqPtr : _potentialObjectives ) {
    // first, check if the requirement is valid (based on unlock)
    if((reqPtr->requiredUnlock != UnlockId::Count) &&
       behaviorExternalInterface.HasProgressionUnlockComponent() &&
       !behaviorExternalInterface.GetProgressionUnlockComponent().IsUnlocked(
                                              reqPtr->requiredUnlock, true ) ) {
      
      PRINT_CH_INFO("Behaviors", "FPSocialize.Start.RequiredObjectiveLocked",
                    "objective %s requires %s, ignoring",
                    BehaviorObjectiveToString( reqPtr->objective ),
                    UnlockIdToString( reqPtr->requiredUnlock ));
      continue;
    }
    
    if( reqPtr->probabilityToRequire < 1.0f &&
       behaviorExternalInterface.GetRNG().RandDbl() >= reqPtr->probabilityToRequire ) {
      
      PRINT_CH_INFO("Behaviors", "FPSocialize.Start.CoinFlipFailed", "%s (p=%f)",
                    BehaviorObjectiveToString( reqPtr->objective ),
                    reqPtr->probabilityToRequire);
      continue;
    }
    
    int numRequired = behaviorExternalInterface.GetRNG().RandIntInRange( reqPtr->randCompletionsMin, reqPtr->randCompletionsMax );
    PRINT_CH_INFO("Behaviors", "FPSocialize.Start.RequiredObjective",
                  "must complete '%s' %d times ( range was [%d, %d] )",
                  BehaviorObjectiveToString( reqPtr->objective ),
                  numRequired,
                  reqPtr->randCompletionsMin,
                  reqPtr->randCompletionsMax);
    
    auto& objectiveRef = _objectivesLeft[ reqPtr->objective ];
    objectiveRef += numRequired;
  }
  
  PrintDebugObjectivesLeft("FPSocialize.Start.InitialObjectives");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySocialize::PrintDebugObjectivesLeft(const std::string& eventName) const
{
#if ANKI_DEV_CHEATS
  std::stringstream ss;
  
  ss << "{";
  for(const auto& objectivePair : _objectivesLeft) {
    if(objectivePair.second <= 0) {
      PRINT_NAMED_ERROR("FPSocialize.CorruptObjectiveData",
                        "Objective '%s' has count %u, should not be possible",
                        BehaviorObjectiveToString(objectivePair.first),
                        objectivePair.second);
    }
    
    ss << ' ' << BehaviorObjectiveToString(objectivePair.first) << ":" << objectivePair.second;
  }
  ss << " }";
  
  PRINT_CH_INFO("Behaviors", eventName.c_str(), "Objectives left: %s", ss.str().c_str());
#endif // ANKI_DEV_CHEATS
}
  
} // namespace Cozmo
} // namespace Anki
