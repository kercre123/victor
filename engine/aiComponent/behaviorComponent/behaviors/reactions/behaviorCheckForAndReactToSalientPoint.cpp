/**
 * File: checkForAndReactToSalientPoint.cpp
 *
 * Author:  Andrew Stein
 * Created: 2018-11-14
 *
 * Description: Run a single frame of a VisionMode specified in Json and react if one of the Json-configured
 *              SalientPointTypes is seen. Does nothing otherwise. The reaction can either be an animation
 *              or another behavior.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorCheckForAndReactToSalientPoint.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/salientPointsComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/utils/cozmoFeatureGate.h"

#include "clad/types/animationTrigger.h"
#include "clad/types/featureGateTypes.h"

#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {
  
namespace {
  // When there is a non-zero cooldown defined in iConfig, override with this if set to a value > 0
  // Note that this will affect *all* instantiations of of this behavior class with non-zero cooldown!
  CONSOLE_VAR(float, kCFARTSP_CooldownOverride_sec, "Behaviors.CheckForAndReactToSalientPoint", 0.f);
}

namespace JsonKeys {
  static const char* AnimTriggerKey          = "animTrigger";
  static const char* BehaviorKey             = "behaviorID";
  static const char* CooldownKey             = "cooldown_sec";
  static const char* HeadAngleKey            = "headAngle_deg";
  static const char* NumFramesKey            = "numFrames";
  static const char* ReactionsKey            = "reactions";
  static const char* SalientPointTypeKey     = "salientPointType";
  static const char* VisionModeKey           = "visionMode";
  static const char* WaitAnimTriggerKey      = "waitAnimTrigger";
}

struct BehaviorCheckForAndReactToSalientPoint::InstanceConfig
{
  VisionMode visionMode = VisionMode::Count;
  int numFrames = 1;
  
  struct ReactionEntry
  {
    std::string       behaviorString;
    ICozmoBehaviorPtr behavior;       // NOTE: Should have EITHER a behavior or an AnimTrigger
    AnimationTrigger  animTrigger;    //       defined as the reaction; not BOTH!
    float             lastReacted_sec;
  };
  std::map<Vision::SalientPointType, ReactionEntry> reactions;
  AnimationTrigger waitAnimTrigger = AnimationTrigger::Count;
  
  bool  useSpecificHeadAngle = false;
  float headAngle_rad = 0.f;
  float cooldown_sec = 30.f;
  
  inline float GetCooldown() const
  {
    // If this instance has a non-zero cooldown, use the override if set
    if(Util::IsFltGTZero(cooldown_sec) && Util::IsFltGTZero(kCFARTSP_CooldownOverride_sec))
    {
      return kCFARTSP_CooldownOverride_sec;
    }
    return cooldown_sec;
  }
  
  inline bool HasCooldownPassed(const float currentTime_sec, const ReactionEntry& reaction) const
  {
    const float timeSinceSeen_sec = currentTime_sec - reaction.lastReacted_sec;
    return (timeSinceSeen_sec > GetCooldown());
  }
};

struct BehaviorCheckForAndReactToSalientPoint::DynamicVariables
{
  RobotTimeStamp_t _lastImageTime_ms = 0;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCheckForAndReactToSalientPoint::BehaviorCheckForAndReactToSalientPoint(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig(new InstanceConfig)
, _dVars(new DynamicVariables)
{
  // load anim triggers / behaviors for each salient point type
  if(ANKI_VERIFY(config.isMember(JsonKeys::ReactionsKey),
                 "BehaviorCheckForAndReactToSalientPoint.Constructor.MissingAnimTriggers", ""))
  {
    const Json::Value& animTriggers = config[JsonKeys::ReactionsKey];
    if(ANKI_VERIFY(animTriggers.isArray(),
                   "BehaviorCheckForAndReactToSalientPoint.Constructor.AnimTriggersNotArray", ""))
    {
      for(const auto& triggerDefinition : animTriggers)
      {
        const bool hasAnimTrigger = triggerDefinition.isMember(JsonKeys::AnimTriggerKey);
        const bool hasBehaviorID  = triggerDefinition.isMember(JsonKeys::BehaviorKey);
        if(ANKI_VERIFY(triggerDefinition.isMember(JsonKeys::SalientPointTypeKey) &&
                       (hasAnimTrigger ^ hasBehaviorID),
                       "BehaviorCheckForAndReactToSalientPoint.Constructor.BadTriggerDefinition", ""))
        {
          const std::string& salientPointTypeString = triggerDefinition[JsonKeys::SalientPointTypeKey].asString();
          Vision::SalientPointType salientPointType = Vision::SalientPointType::Unknown;
          if(ANKI_VERIFY(Vision::SalientPointTypeFromString(salientPointTypeString, salientPointType),
                         "BehaviorCheckForAndReactToSalientPoint.Constructor.BadSalientPointType",
                         "%s", salientPointTypeString.c_str()))
          {
            InstanceConfig::ReactionEntry newEntry{"", nullptr, AnimationTrigger::Count, 0.f};
            
            if(hasAnimTrigger)
            {
              DEV_ASSERT(!hasBehaviorID,
                         "BehaviorCheckForAndReactToSalientPoint.Constructor.DefinesAnimTriggerAndBehavior");
              const std::string& animString = triggerDefinition[JsonKeys::AnimTriggerKey].asString();
              ANKI_VERIFY(AnimationTriggerFromString(animString, newEntry.animTrigger),
                          "BehaviorCheckForAndReactToSalientPoint.Constructor.BadAnimTrigger",
                          "%s", animString.c_str());
            }
            else if(hasBehaviorID)
            {
              // NOTE: we store the string here and defer finding the actual behavior to InitBehavior
              //       because we cannot safely use the BEI or FindBehabvior from within the constructor
              //       since the factory is still constructing other behaviors.
              newEntry.behaviorString = triggerDefinition[JsonKeys::BehaviorKey].asString();
            }
            else
            {
              // Should not get here by virtue of earlier checks
              DEV_ASSERT(false, "BehaviorCheckForAndReactToSalientPoint.Constructor.HasNeitherBehaviorNorAnimDefined");
            }
            
            _iConfig->reactions.emplace(salientPointType, std::move(newEntry));
          }
        }
      }
    }
  }
  
  if(ANKI_VERIFY(config.isMember(JsonKeys::VisionModeKey),
                 "BehaviorCheckForAndReactToSalientPoint.Constructor.MissingVisionMode", ""))
  {
    const std::string& visionModeString = config[JsonKeys::VisionModeKey].asString();
    ANKI_VERIFY(VisionModeFromString(visionModeString, _iConfig->visionMode),
                "BehaviorCheckForAndReactToSalientPoint.Constructor.BadVisionMode",
                "%s", visionModeString.c_str());
  }
  
  JsonTools::GetValueOptional(config, JsonKeys::NumFramesKey, _iConfig->numFrames);
  
  if(config.isMember(JsonKeys::WaitAnimTriggerKey))
  {
    const std::string& animString = config[JsonKeys::WaitAnimTriggerKey].asString();
    AnimationTrigger animTrigger = AnimationTrigger::Count;
    if(ANKI_VERIFY(AnimationTriggerFromString(animString, animTrigger),
                   "BehaviorCheckForAndReactToSalientPoint.Constructor.BadWaitAnimTrigger",
                   "%s", animString.c_str()))
    {
      _iConfig->waitAnimTrigger = AnimationTriggerFromString(animString);
    }
  }
  
  JsonTools::GetValueOptional(config, JsonKeys::CooldownKey, _iConfig->cooldown_sec);
  
  float headAngle_deg = 0.f;
  if(JsonTools::GetValueOptional(config, JsonKeys::HeadAngleKey, headAngle_deg))
  {
    _iConfig->headAngle_rad = DEG_TO_RAD(headAngle_deg);
    _iConfig->useSpecificHeadAngle = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCheckForAndReactToSalientPoint::~BehaviorCheckForAndReactToSalientPoint()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForAndReactToSalientPoint::InitBehavior()
{
  // Set up the behaviors for any reactions that have behavior strings defined from config
  // Can't use BEI/FindBehavior in the constructor, so have to do it here :-(
  for(auto & entryPair : _iConfig->reactions)
  {
    InstanceConfig::ReactionEntry& entry = entryPair.second;
    if(!entry.behaviorString.empty())
    {
      DEV_ASSERT(entry.animTrigger == AnimationTrigger::Count,
                 "BehaviorCheckForAndReactToSalientPoint.InitBehavior.BehaviorStringAndAnimTriggerSet");
      
      entry.behavior = FindBehavior(entry.behaviorString);
      ANKI_VERIFY(entry.behavior != nullptr,
                  "BehaviorCheckForAndReactToSalientPoint.InitBehavior.BadBehaviorString",
                  "%s", entry.behaviorString.c_str());
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForAndReactToSalientPoint::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  for(const auto& entryPair : _iConfig->reactions)
  {
    const InstanceConfig::ReactionEntry& entry = entryPair.second;
    if(entry.behavior != nullptr)
    {
      DEV_ASSERT(entry.animTrigger == AnimationTrigger::Count,
                 "BehaviorCheckForAndReactToSalientPoint.GetAllDelegates.BehaviorAndAnimTriggerSet");
      
      delegates.insert( entry.behavior.get() );
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForAndReactToSalientPoint::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    JsonKeys::AnimTriggerKey,
    JsonKeys::BehaviorKey,
    JsonKeys::CooldownKey,
    JsonKeys::HeadAngleKey,
    JsonKeys::NumFramesKey,
    JsonKeys::ReactionsKey,
    JsonKeys::SalientPointTypeKey,
    JsonKeys::VisionModeKey,
    JsonKeys::WaitAnimTriggerKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCheckForAndReactToSalientPoint::WantsToBeActivatedBehavior() const
{
  // Only want to be activated if we haven't reacted to at least one of the salient point types too recently
  const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  for(const auto& reaction : _iConfig->reactions)
  {
    if(_iConfig->HasCooldownPassed(currentTime_sec, reaction.second))
    {
      return true;
    }
  }
  
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForAndReactToSalientPoint::OnBehaviorActivated()
{
  _dVars->_lastImageTime_ms = GetBEI().GetVisionComponent().GetLastProcessedImageTimeStamp();
  
  WaitForImagesAction* waitAction = new WaitForImagesAction(_iConfig->numFrames, _iConfig->visionMode);
  
  CompoundActionSequential* action = new CompoundActionSequential();
  
  if(_iConfig->useSpecificHeadAngle)
  {
    // First move the head to the right angle, if one was specified
    action->AddAction( new MoveHeadToAngleAction(_iConfig->headAngle_rad) );
  }
  
  if(_iConfig->waitAnimTrigger == AnimationTrigger::Count)
  {
    // No animation to play while waiting
    action->AddAction( waitAction );
  }
  else
  {
    // Loop a waiting animation while waiting
    CompoundActionParallel* compoundAction = new CompoundActionParallel({
      waitAction,
      new ReselectingLoopAnimationAction{_iConfig->waitAnimTrigger},
    });
    compoundAction->SetShouldEndWhenFirstActionCompletes(true);
    action->AddAction( compoundAction );
  }
  
  DelegateIfInControl(action, &BehaviorCheckForAndReactToSalientPoint::CheckForStimulus);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForAndReactToSalientPoint::CheckForStimulus()
{
  const SalientPointsComponent& salientPointsComp = GetAIComp<SalientPointsComponent>();
  
  std::vector<std::pair<Vision::SalientPointType, InstanceConfig::ReactionEntry*>> possibleReactions;
  const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  for(auto& reaction : _iConfig->reactions)
  {
    if(_iConfig->HasCooldownPassed(currentTime_sec, reaction.second))
    {
      std::list<Vision::SalientPoint> salientPoints;
      salientPointsComp.GetSalientPointSinceTime(salientPoints, reaction.first, _dVars->_lastImageTime_ms);
      if(!salientPoints.empty())
      {
        possibleReactions.emplace_back(reaction.first, &reaction.second);
      }
    }
  }
  
  LOG_DEBUG("BehaviorCheckForAndReactToSalientPoint.CheckForStimulus.NumPossibleReactions",
            "%zu", possibleReactions.size());
  
  if(!possibleReactions.empty())
  {
    int chosenIndex = 0;
    if(possibleReactions.size() > 1)
    {
      chosenIndex = GetRNG().RandInt((int)possibleReactions.size());
    }
    const auto& chosenReactionPair = possibleReactions[chosenIndex];
    InstanceConfig::ReactionEntry* chosenReaction = chosenReactionPair.second;
    
    if(chosenReaction->animTrigger != AnimationTrigger::Count)
    {
      DEV_ASSERT(chosenReaction->behavior == nullptr,
                 "BehaviorCheckForAndReactToSalientPoint.CheckForStimulus.HasAnimAndBehavior");
      const AnimationTrigger animTrigger = chosenReaction->animTrigger;
      TriggerAnimationAction* action = new TriggerAnimationAction(animTrigger);
      DelegateIfInControl(action, [chosenReaction](ActionResult result)
                          {
                            // Reset the cooldown timer for the thing we reacted to, iff the animation succeeds
                            if(ActionResult::SUCCESS == result)
                            {
                              const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
                              chosenReaction->lastReacted_sec = currentTime_sec;
                            }
                          });
    }
    else if(chosenReaction->behavior != nullptr)
    {
      if(!chosenReaction->behavior->IsActivated() &&
         chosenReaction->behavior->WantsToBeActivated())
      {
        DelegateIfInControl(chosenReaction->behavior.get(), [chosenReaction]()
                            {
                              const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
                              chosenReaction->lastReacted_sec = currentTime_sec;
                            });
      }
    }
    else
    {
      // Should not get here by virtue of earlier checks
      DEV_ASSERT(false, "BehaviorCheckForAndReactToSalientPoint.CheckForStimulus.HasNeitherAnimNorBehaviorDefined");
    }
    
    DASMSG(behavior_check_for_and_react_to_did_react, "behavior.check_for_and_react_to.did_react",
           "Found the required stimulus after cooldown and reacted to it");
    DASMSG_SET(s1, EnumToString(chosenReactionPair.first),
               "SalientPointType we are reacting to, e.g. Hand, Cat, Dog...");
    DASMSG_SET(i1, (int)possibleReactions.size(),
               "Number of possible reactions found");
    DASMSG_SEND();
  }
}

} // namespace Vector
} // namespace Anki
