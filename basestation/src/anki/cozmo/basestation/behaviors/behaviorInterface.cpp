/**
 * File: behaviorInterface.cpp
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Defines interface for a Cozmo "Behavior".
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupHelpers.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

#include "util/math/numericCast.h"

namespace Anki {
namespace Cozmo {

  // Static initializations
  const ActionList::SlotHandle IBehavior::sActionSlot = Robot::DriveAndManipulateSlot;
  Util::RandomGenerator IBehavior::sRNG;
  
  const char* IBehavior::kBaseDefaultName = "no_name";
  
  static const char* kNameKey              = "name";
  static const char* kEmotionScorersKey    = "emotionScorers";
  static const char* kRepetitionPenaltyKey = "repetitionPenalty";
  static const char* kBehaviorGroupsKey    = "behaviorGroups";
  
  
  IBehavior::IBehavior(Robot& robot, const Json::Value& config)
  : _moodScorer()
  , _robot(robot)
  , _lastRunTime(0.0)
  , _overrideScore(-1.0f)
  , _isRunning(false)
  , _isOwnedByFactory(false)
  , _enableRepetitionPenalty(true)
  {
    ReadFromJson(config);
  }

  bool IBehavior::ReadFromJson(const Json::Value& config)
  {
    const Json::Value& nameJson = config[kNameKey];
    _name = nameJson.isString() ? nameJson.asCString() : kBaseDefaultName;

    _moodScorer.ClearEmotionScorers();
    
    const Json::Value& emotionScorersJson = config[kEmotionScorersKey];
    if (!emotionScorersJson.isNull())
    {
      _moodScorer.ReadFromJson(emotionScorersJson);
    }
    
    _repetitionPenalty.Clear();
    
    const Json::Value& repetitionPenaltyJson = config[kRepetitionPenaltyKey];
    if (!repetitionPenaltyJson.isNull())
    {
      if (!_repetitionPenalty.ReadFromJson(repetitionPenaltyJson))
      {
        PRINT_NAMED_WARNING("IBehavior.BadRepetitionPenalty", "Behavior '%s': %s failed to read", _name.c_str(), kRepetitionPenaltyKey);
      }
    }
    
    // Ensure there is a valid graph
    if (_repetitionPenalty.GetNumNodes() == 0)
    {
      _repetitionPenalty.AddNode(0.0f, 1.0f); // no penalty for any value
    }
    
    // Behavior Groups
    
    ClearBehaviorGroups();
    
    const Json::Value& behaviorGroupsJson = config[kBehaviorGroupsKey];
    if (behaviorGroupsJson.isArray())
    {
      const uint32_t numBehaviorGroups = behaviorGroupsJson.size();
      
      const Json::Value kNullValue;
      
      for (uint32_t i = 0; i < numBehaviorGroups; ++i)
      {
        const Json::Value& behaviorGroupJson = behaviorGroupsJson.get(i, kNullValue);
        
        const char* behaviorGroupString = behaviorGroupJson.isString() ? behaviorGroupJson.asCString() : "";
        const BehaviorGroup behaviorGroup = BehaviorGroupFromString(behaviorGroupString);
        
        if (behaviorGroup != BehaviorGroup::Count)
        {
          SetBehaviorGroup(behaviorGroup, true);
        }
        else
        {
          PRINT_NAMED_WARNING("IBehavior.BadBehaviorGroup", "Failed to read group %u '%s'", i, behaviorGroupString);
        }
      }
    }
    
    return true;
  }

  IBehavior::~IBehavior()
  {
    ASSERT_NAMED(!IsOwnedByFactory(), "Behavior must be removed from factory before destroying it!");
  }
  
  void IBehavior::SubscribeToTags(std::set<GameToEngineTag> &&tags)
  {
    if(_robot.HasExternalInterface()) {
      auto handlerCallback = [this](const GameToEngineEvent& event) {
        this->HandleEvent(event);
      };
      
      for(auto tag : tags) {
        _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(tag, handlerCallback));
      }
    }
  }
  
  void IBehavior::SubscribeToTags(std::set<EngineToGameTag> &&tags)
  {
    if(_robot.HasExternalInterface()) {
      auto handlerCallback = [this](const EngineToGameEvent& event) {
        this->HandleEvent(event);
      };
      
      for(auto tag : tags) {
        _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(tag, handlerCallback));
      }
    }
  }
  
  float IBehavior::EvaluateEmotionScore(const MoodManager& moodManager) const
  {
    return _moodScorer.EvaluateEmotionScore(moodManager);
  }
  
  // EvaluateScoreInternal is virtual and can optionally be overriden by subclasses
  float IBehavior::EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const
  {
    return EvaluateEmotionScore(robot.GetMoodManager());
  }

  float IBehavior::EvaluateRepetitionPenalty(double currentTime_sec) const
  {
    if (_lastRunTime > 0.0)
    {
      const float timeSinceRun = Util::numeric_cast<float>(currentTime_sec - _lastRunTime);
      const float repetitionPenalty = _repetitionPenalty.EvaluateY(timeSinceRun);
      return repetitionPenalty;
    }
    
    return 1.0f;
  }
  
  float IBehavior::EvaluateScore(const Robot& robot, double currentTime_sec) const
  {
    if (IsRunnable(robot, currentTime_sec))
    {
      const bool doOverrideScore = (_overrideScore >= 0.0f);
      float score = doOverrideScore ? _overrideScore : EvaluateScoreInternal(robot, currentTime_sec);
      
      if (_enableRepetitionPenalty)
      {
        const float repetitionPenalty = EvaluateRepetitionPenalty(currentTime_sec);
        score *= repetitionPenalty;
      }
      
      return score;
    }
    
    return 0.0f;
  }

#pragma mark --- IReactionaryBehavior ----
  
  IReactionaryBehavior::IReactionaryBehavior(Robot& robot, const Json::Value& config)
    : IBehavior(robot, config)
  {
    SetBehaviorGroup(BehaviorGroup::Reactionary, true);
  }
  
  void IReactionaryBehavior::SubscribeToTriggerTags(std::set<EngineToGameTag>&& tags)
  {
    _engineToGameTags.insert(tags.begin(), tags.end());
    SubscribeToTags(std::move(tags));
  }
  
  void IReactionaryBehavior::SubscribeToTriggerTags(std::set<GameToEngineTag>&& tags)
  {
    _gameToEngineTags.insert(tags.begin(), tags.end());
    SubscribeToTags(std::move(tags));
  }
  
  
} // namespace Cozmo
} // namespace Anki
