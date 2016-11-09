/**
 * File: AIGoalStrategySimple
 *
 * Author: Raul
 * Created: 11/01/2016
 *
 * Description: Basic implementation of strategy for goals that rely on their behaviors to know when they want
 *              to start or finish, since duplicating the logic of when behaviors should run is too complex. A goal
 *              with this strategy will generally start when given a chance, and behaviors will drive when it ends.
 *              Note it still inherits the duration timers from the base class.
 *
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "AIGoalStrategySimple.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoalStrategySimple::AIGoalStrategySimple(Robot& robot, const Json::Value& config)
  : Anki::Cozmo::IAIGoalStrategy(config)
{
}


} // namespace
} // namespace
