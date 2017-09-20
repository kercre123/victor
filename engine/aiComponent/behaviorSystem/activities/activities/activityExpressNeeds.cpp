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

#include "engine/aiComponent/behaviorSystem/activities/activities/activityExpressNeeds.h"

namespace Anki {
namespace Cozmo {

namespace {
}

ActivityExpressNeeds::ActivityExpressNeeds(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: IActivity(behaviorExternalInterface, config)
{
}


void ActivityExpressNeeds::OnSelectedInternal(BehaviorExternalInterface& behaviorExternalInterface)
{

}

void ActivityExpressNeeds::OnDeselectedInternal(BehaviorExternalInterface& behaviorExternalInterface)
{

}

}
}
