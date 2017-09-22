/**
 * File: ActivityStrategySimple
 *
 * Author: Raul
 * Created: 11/01/2016
 *
 * Description: Basic implementation of strategy for activities that rely on their behaviors to know when they want
 *              to start or finish, since duplicating the logic of when behaviors should run is too complex. An activity
 *              with this strategy will generally start when given a chance, and behaviors will drive when it ends.
 *              Note it still inherits the duration timers from the base class.
 *
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "engine/aiComponent/behaviorComponent/activities/activityStrategies/activityStrategySimple.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrategySimple::ActivityStrategySimple(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: IActivityStrategy(behaviorExternalInterface, config)
{
}


} // namespace
} // namespace
