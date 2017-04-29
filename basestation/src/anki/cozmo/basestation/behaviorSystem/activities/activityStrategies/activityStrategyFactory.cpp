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
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategyObjectTapInteraction.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategyPyramid.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategySpark.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategySimple.h"

#include "anki/common/basestation/jsonTools.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "json/json.h"

#include <string>

namespace Anki {
namespace Cozmo {
namespace ActivityStrategyFactory {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActivityStrategy* CreateActivityStrategy(Robot& robot, const Json::Value& config)
{
  IActivityStrategy* newStrategy = nullptr;

  // extract type
  const Json::Value& type = config["type"];
  std::string typeStr = type.isNull() ? "" : type.asString();
  std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::tolower);

  // should this be more sophisticated than string compare?
  if ( typeStr == "simple" )
  {
    newStrategy = new ActivityStrategySimple(robot, config);
  }
  else if ( typeStr == "fp_playwithhumans" ) {
    newStrategy = new ActivityStrategyFPPlayWithHumans(robot, config);
  }
  else if ( typeStr == "spark" ) {
    newStrategy = new ActivityStrategySpark(robot, config);
  }
  else if ( typeStr == "object_tap_interaction" ) {
    newStrategy = new ActivityStrategyObjectTapInteraction(robot, config);
  }
  else if ( typeStr == "pyramid"){
    newStrategy = new ActivityStrategyPyramid(robot, config);
  }
  else
  {
    JsonTools::PrintJsonError(config, "ActivityStrategyFactory.CreateActivityStrategy.InvalidType");
    DEV_ASSERT(false, "ActivityStrategyFactory.CreateActivityStrategy.InvalidType");
  }
  
  // if failed print information to debug
  const bool failed = (nullptr == newStrategy);
  if ( failed )
  {
    PRINT_NAMED_ERROR("ActivityStrategyFactory.CreateActivityStrategy.Fail",
      "Failed to create behavior chooser '%s'. Check log for config.", typeStr.c_str() );
    JsonTools::PrintJsonError(config, "ActivityStrategyFactory.CreateActivityStrategy.Fail");
  }
  
  return newStrategy;
}


}; // namespace
}; // namespace
}; // namespace
