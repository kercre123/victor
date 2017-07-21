/**
 * File: ActivityStrategyFactory.cpp
 *
 * Author: Raul
 * Created: 08/11/2016
 *
 * Description: Factory to create ActivityStrategies from config.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategyFactory.h"

// Activity strategies
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategyFPPlayWithHumans.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategyNeedBasedCooldown.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategyNeeds.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategyPyramid.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategySevereNeedsTransition.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategySimple.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategySpark.h"

#include "anki/common/basestation/jsonTools.h"
#include "clad/types/behaviorSystem/activityTypes.h"
#include "json/json.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"

#include <string>

namespace Anki {
namespace Cozmo {

namespace{
static const char* kStrategyTypeConfigKey = "type";
}
  
namespace ActivityStrategyFactory {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActivityStrategy* CreateActivityStrategy(Robot& robot, const Json::Value& config)
{
  IActivityStrategy* newStrategy = nullptr;

  // extract type
  const std::string type = JsonTools::ParseString(config, kStrategyTypeConfigKey,
                                                  "IActivityStrategy.CreateActivity.MissingType");
  
  ActivityStrategy strategyType = ActivityStrategyFromString(type);

  switch(strategyType){
    case ActivityStrategy::PlayWithHumans:
    {
      newStrategy = new ActivityStrategyFPPlayWithHumans(robot, config);
      break;
    }
    case ActivityStrategy::Pyramid:
    {
      newStrategy = new ActivityStrategyPyramid(robot, config);
      break;
    }
    case ActivityStrategy::Simple:
    {
      newStrategy = new ActivityStrategySimple(robot, config);
      break;
    }
    case ActivityStrategy::Spark:
    {
      newStrategy = new ActivityStrategySpark(robot, config);
      break;
    }
    case ActivityStrategy::NeedBasedCooldown:
    {
      newStrategy = new ActivityStrategyNeedBasedCooldown(robot, config);
      break;
    }
    case ActivityStrategy::Needs:
    {
      newStrategy = new ActivityStrategyNeeds(robot, config);
      break;
    }
    case ActivityStrategy::SevereNeedTransition:
    {
      newStrategy = new ActivityStrategySevereNeedsTransition(robot, config);
      break;
    }     
    case ActivityStrategy::Count:
    {
      DEV_ASSERT(false,
                 "IActivityStrategy.CreateActivityStrategy.InvalidStrategyType");
    }
  }
  
  // if failed print information to debug
  const bool failed = (nullptr == newStrategy);
  if ( failed )
  {
    PRINT_NAMED_ERROR("ActivityStrategyFactory.CreateActivityStrategy.Fail",
      "Failed to create behavior chooser '%s'. Check log for config.", type.c_str() );
    JsonTools::PrintJsonError(config, "ActivityStrategyFactory.CreateActivityStrategy.Fail");
  }
  
  return newStrategy;
}


}; // namespace
}; // namespace
}; // namespace
