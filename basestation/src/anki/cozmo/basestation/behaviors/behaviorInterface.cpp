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

#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupHelpers.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

#include "util/math/numericCast.h"

namespace Anki {
namespace Cozmo {

  // Static initializations
  const char* IBehavior::kBaseDefaultName = "no_name";
  
  static const char* kNameKey              = "name";
  static const char* kEmotionScorersKey    = "emotionScorers";
  static const char* kRepetitionPenaltyKey = "repetitionPenalty";
  static const char* kBehaviorGroupsKey    = "behaviorGroups";
  
  
  IBehavior::IBehavior(Robot& robot, const Json::Value& config)
  : _moodScorer()
  , _robot(robot)
  , _startedRunningTime_s(0.0)
  , _lastRunTime_s(0.0)
  , _overrideScore(-1.0f)
  , _isRunning(false)
  , _isOwnedByFactory(false)
  , _isChoosable(true)
  , _enableRepetitionPenalty(true)
  {
    ReadFromJson(config);

    if(_robot.HasExternalInterface()) {
      // NOTE: this won't get sent down to derived classes (unless they also subscribe)
      _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(
                                EngineToGameTag::RobotCompletedAction,
                                [this](const EngineToGameEvent& event) {
                                  ASSERT_NAMED(event.GetData().GetTag() == EngineToGameTag::RobotCompletedAction,
                                               "Wrong event type from callback");
                                  HandleActionComplete(event.GetData().Get_RobotCompletedAction());
                                } ));
    }
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
  
  double IBehavior::GetRunningDuration(double currentTime_sec) const
  {
    if (_isRunning)
    {
      const double timeSinceStarted = currentTime_sec - _startedRunningTime_s;
      return timeSinceStarted;
    }
    return 0.0;
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

  // EvaluateScoreInternal is virtual and can optionally be overriden by subclasses
  float IBehavior::EvaluateRunningScoreInternal(const Robot& robot, double currentTime_sec) const
  {
    return EvaluateEmotionScore(robot.GetMoodManager());
  }
  
  float IBehavior::EvaluateRepetitionPenalty(double currentTime_sec) const
  {
    if (_lastRunTime_s > 0.0)
    {
      const float timeSinceRun = Util::numeric_cast<float>(currentTime_sec - _lastRunTime_s);
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
      const bool isRunning = IsRunning();
                
      float score = 0.0f;
      
      if (doOverrideScore)
      {
        score = _overrideScore;
      }
      else if (isRunning)
      {
        score = EvaluateRunningScoreInternal(robot, currentTime_sec);
      }
      else
      {
        score = EvaluateScoreInternal(robot, currentTime_sec);
      }
      
      // no repetition penalty when running
      if (_enableRepetitionPenalty && !isRunning)
      {
        const float repetitionPenalty = EvaluateRepetitionPenalty(currentTime_sec);
        score *= repetitionPenalty;
      }
      
      return score;
    }
    
    return 0.0f;
  }

  bool IBehavior::StartActing(IActionRunner* action, RobotCompletedActionCallback callback)
  {
    if( ! IsRunning() ) {
      PRINT_NAMED_WARNING("IBehavior.StartActing.Failure.NotRunning",
                          "Behavior '%s' can't start acting because it is not running",
                          GetName().c_str());
      return false;
    }

    if( IsActing() ) {
      PRINT_NAMED_WARNING("IBehavior.StartActing.Failure.AlreadyActing",
                          "Behavior '%s' can't start acting because it is already running an action",
                          GetName().c_str());
      return false;
    }

    _actingCallback = callback;
    _lastActionTag = action->GetTag();
    _robot.GetActionList().QueueActionNow(action);
    return true;
  }

  bool IBehavior::StartActing(IActionRunner* action, ActionResultCallback callback)
  {
    return StartActing(action,
                       [callback](const ExternalInterface::RobotCompletedAction& msg) {
                         callback(msg.result);
                       });
  }

  bool IBehavior::StartActing(IActionRunner* action, SimpleCallback callback)
  {
    return StartActing(action, [callback](ActionResult ret){ callback(); });
  }

  bool IBehavior::StartActing(IActionRunner* action, SimpleCallbackWithRobot callback)
  {
    return StartActing(action, [this, callback](ActionResult ret){ callback(_robot); });
  }

  void IBehavior::HandleActionComplete(const ExternalInterface::RobotCompletedAction& msg)
  {
    if( IsActing() && msg.idTag == _lastActionTag ) {
      _lastActionTag = ActionConstants::INVALID_TAG;

      if( IsRunning() && _actingCallback ) {
        _actingCallback(msg);
      }
    }
  }

  bool IBehavior::StopActing()
  {
    if( IsActing() ) {
      // TODO:(bn) should we check IsRunning here? Currently, no, because we want to support this happening
      // from Stop()
      bool ret = _robot.GetActionList().Cancel( _lastActionTag );
      // note that the callback, if there was one, should have already been called at this point, so it's safe
      // to clear the tag. Also, if the cancel itself failed, that is probably a bug, but somehow the action
      // is gone, so no sense keeping the tag around (and it clearly isn't running)
      _lastActionTag = ActionConstants::INVALID_TAG;
      return ret;
    }

    return false;
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
