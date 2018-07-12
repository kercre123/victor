/**
 * File: BehaviorScoringWrapper.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-31
 *
 * Description: Simple helper to track cooldowns
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_BehaviorScoringWrapper_H__
#define __Engine_AiComponent_BehaviorComponent_BehaviorScoringWrapper_H__

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "engine/moodSystem/moodScorer.h"
#include "json/json.h"
#include "util/logging/logging.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {
  
enum class BehaviorObjective : int32_t;
  
namespace ExternalInterface {
struct BehaviorObjectiveAchieved;
}

class BehaviorScoringWrapper
{
  
public:
  BehaviorScoringWrapper(const Json::Value& config);
  virtual ~BehaviorScoringWrapper();
  
  void Init(BehaviorExternalInterface& bei);
   
  // Notify the wrapper of the behaviors activation/deactivation state
  void BehaviorWillBeActivated();
  void BehaviorDeactivated();

  // Allow us to load scored JSON in seperately from the rest of parameters
  bool ReadFromJson(const Json::Value& config);
  
  float EvaluateScore(BehaviorExternalInterface& bei) const;
  const MoodScorer& GetMoodScorer() const { return _moodScorer; }
  size_t GetEmotionScorerCount() const { return _moodScorer.GetEmotionScorerCount(); }
  const EmotionScorer& GetEmotionScorer(size_t index) const { return _moodScorer.GetEmotionScorer(index); }
  
  float EvaluateRepetitionPenalty() const;
  const Util::GraphEvaluator2d& GetRepetitionPenalty() const { return _repetitionPenalty; }
  
  float EvaluateActivatedPenalty() const;
  const Util::GraphEvaluator2d& GetActivatedPenalty() const { return _activatedPenalty; }

private:
  void HandleBehaviorObjective(const ExternalInterface::BehaviorObjectiveAchieved& msg);
  
  // ==================== Member Vars ====================
  std::vector<Signal::SmartHandle> _eventHandlers;
  bool _isActivated = false;
  
  MoodScorer              _moodScorer;
  Util::GraphEvaluator2d  _repetitionPenalty;
  Util::GraphEvaluator2d  _activatedPenalty;
  float                   _flatScore = 0.f;
  float                   _lastTimeDeactivated = 0.f;
  float                   _timeActivated   = 0.f;  
  
  // if this behavior objective gets sent (by any behavior), then consider this behavior to have been activated
  // (for purposes of repetition penalty, aka cooldown)
  BehaviorObjective _cooldownOnObjective;
  
  
  bool _enableRepetitionPenalty = true;
  bool _enableActivatedPenalty = true;
};

} // namespace Cozmo
} // namespace Anki


#endif
