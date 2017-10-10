/**
 * File: ActivityBehaviorsOnly.h
 *
 * Author: Kevin M. Karol
 * Created: 04/27/17
 *
 * Description: Activity when choosing the next behavior is all that's desired
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/activities/activities/activityBehaviorsOnly.h"

namespace Anki {
namespace Cozmo {

ActivityBehaviorsOnly::ActivityBehaviorsOnly(const Json::Value& config)
: IActivity(config)
{
  
}

} // namespace Cozmo
} // namespace Anki
