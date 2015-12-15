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
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

namespace Anki {
namespace Cozmo {

  // Static initializations
  const ActionList::SlotHandle IBehavior::sActionSlot = Robot::DriveAndManipulateSlot;
  Util::RandomGenerator IBehavior::sRNG;
  
  const char* IBehavior::kBaseDefaultName = "no_name";
  
  static const char* kNameKey           = "name";
  static const char* kEmotionScorersKey = "emotionScorers";
  
  
  IBehavior::IBehavior(Robot& robot, const Json::Value& config)
  : _robot(robot)
  , _isRunning(false)
  , _isOwnedByFactory(false)
  {
    if (config.isNull())
    {
      _name = kBaseDefaultName;
    }
    else
    {
      ReadFromJson(config);
    }
  }

  bool IBehavior::ReadFromJson(const Json::Value& config)
  {
    const Json::Value& nameJson = config[kNameKey];
    _name = nameJson.isString() ? nameJson.asCString() : kBaseDefaultName;

    _emotionScorers.clear();
    
    const Json::Value& emotionScorersJson = config[kEmotionScorersKey];
    if (!emotionScorersJson.isNull())
    {
      const uint32_t numEmotionScorers = emotionScorersJson.size();

      _emotionScorers.reserve(numEmotionScorers);
      
      const Json::Value kNullScorerValue;
      
      for (uint32_t i = 0; i < numEmotionScorers; ++i)
      {
        const Json::Value& emotionScorerJson = emotionScorersJson.get(i, kNullScorerValue);
        
        if (emotionScorerJson.isNull())
        {
          PRINT_NAMED_WARNING("IBehavior.BadEmotionScorer", "EmotionScorer %u failed to read", i);
        }
        else
        {
          _emotionScorers.emplace_back(emotionScorerJson);
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
    float totalScore = 0.0f;
    uint32_t numEmotionsScored = 0;
    const uint32_t kTickRangeForDelta = 60; // 2 would give any chances between the last 2 updates, but we might want to track more if it takes longer for a running behavior to interrupt and allow this to run
    
    for (const EmotionScorer& emotionScorer : _emotionScorers)
    {
      const EmotionType emotionType = emotionScorer.GetEmotionType();
      const float emotionValue = emotionScorer.TrackDeltaScore() ? moodManager.GetEmotionDeltaRecentTicks(emotionType, kTickRangeForDelta)
                                                                 : moodManager.GetEmotionValue(emotionType);
      const float score = emotionScorer.GetScoreGraph().EvaluateY(emotionValue);
      if (FLT_NEAR(score, 0.0f))
      {
        // any component scoring 0 forces an overal 0 score (e.g. prevent behavior ever being picked)
        return 0.0f;
      }
      else
      {
        totalScore += score;
        ++numEmotionsScored;
      }
    }
    
    const float averageScore = (numEmotionsScored > 0) ? (totalScore / float(numEmotionsScored)) : 0.0f;
    return averageScore;
  }
  
  float IBehavior::EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const
  {
    return EvaluateEmotionScore(robot.GetMoodManager());
  }


#pragma mark --- IReactionaryBehavior ----
  
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
