/**
 * File: iAIGoalStrategy
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Interface for goal strategies (when goals become active/inactive)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_iAIGoalStrategy_H__
#define __Cozmo_Basestation_BehaviorSystem_iAIGoalStrategy_H__

#include "util/signals/simpleSignal_fwd.h"

#include "json/json-forwards.h"

#include <memory>
#include <vector>

namespace Anki {
namespace Cozmo {

template<typename TYPE> class AnkiEvent;
class MoodScorer;
class Robot;

class IAIGoalStrategy
{
public:

  // construction/destruction
  IAIGoalStrategy(const Json::Value& config);
  virtual ~IAIGoalStrategy();

  // true when this goal would be happy to start, false if it doens't want to be fired now
  bool WantsToStart(const Robot& robot, float lastTimeGoalRanSec) const;

  // true when this goal wants to finish, false if it would rather continue
  bool WantsToEnd(const Robot& robot, float lastTimeGoalStartedSec) const;
  
protected:

  // true when this goal would be happy to start, false if it doens't want to be fired now
  virtual bool WantsToStartInternal(const Robot& robot, float lastTimeGoalRanSec) const = 0;

  // true when this goal wants to finish, false if it would rather continue
  virtual bool WantsToEndInternal(const Robot& robot, float lastTimeGoalStartedSec) const = 0;
  
  // allow access to GoalShouldEnd in children to tell when the goal's about to be killed
  void SetGoalShouldEndSecs(float timeout_s){ _goalShouldEndSecs = timeout_s;}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // signal handles for events strategies register to
  std::vector<Signal::SmartHandle> _eventHandles;

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // goal runs for at least this much
  float _goalCanEndSecs;
  
  // goal runs for at most this much
  float _goalShouldEndSecs;

  // after finishing, the goal will not run again for this long
  float _cooldownSecs;
  
  // mood scoring to start the strategy. Only initialized if _requiredMinStartMoodScore is specified in json
  std::unique_ptr<MoodScorer> _startMoodScorer;
  
  // if _requiredMinStartMoodScore is set in json, the _startMoodScorer's score has to be greater than this value, for
  // the strategy to consider starting (other requirements may fail, so start is not guaranteed)
  float _requiredMinStartMoodScore;

};
  
} // namespace
} // namespace

#endif // endif
