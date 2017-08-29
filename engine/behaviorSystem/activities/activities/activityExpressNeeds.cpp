/**
 * File: activityExpressNeeds.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-06-20
 *
 * Description: Activity to express severe needs states.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/behaviorSystem/activities/activities/activityExpressNeeds.h"

namespace Anki {
namespace Cozmo {

namespace {
}

ActivityExpressNeeds::ActivityExpressNeeds(Robot& robot, const Json::Value& config)
: IActivity(robot, config)
{
}


void ActivityExpressNeeds::OnSelectedInternal(Robot& robot)
{

}

void ActivityExpressNeeds::OnDeselectedInternal(Robot& robot)
{

}

}
}
