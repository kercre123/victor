/**
 * File: AIGoalStrategyFactory
 *
 * Author: Raul
 * Created: 08/11/2016
 *
 * Description: Factory to create AIGoalStrategies from config.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "AIGoalStrategyFactory.h"

// AI Goal strategies
#include "AIGoalStrategies/AIGoalStrategyFPHiking.h"
#include "AIGoalStrategies/AIGoalStrategyFPPlayAlone.h"
#include "AIGoalStrategies/AIGoalStrategyFPPlayWithHumans.h"
#include "AIGoalStrategies/AIGoalStrategyFPSocialize.h"
#include "AIGoalStrategies/AIGoalStrategySpark.h"

#include "anki/common/basestation/jsonTools.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "json/json.h"

#include <string>

namespace Anki {
namespace Cozmo {
namespace AIGoalStrategyFactory {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IAIGoalStrategy* CreateAIGoalStrategy(Robot& robot, const Json::Value& config)
{
  IAIGoalStrategy* newStrategy = nullptr;

  // extract type
  const Json::Value& type = config["type"];
  std::string typeStr = type.isNull() ? "" : type.asString();
  std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::tolower);

  // should this be more sophisticated than string compare?
  if ( typeStr == "fp_hiking" )
  {
    newStrategy = new AIGoalStrategyFPHiking(robot, config);
  }
  else if ( typeStr == "fp_playalone" ) {
    newStrategy = new AIGoalStrategyFPPlayAlone(robot, config);
  }
  else if ( typeStr == "fp_playwithhumans" ) {
    newStrategy = new AIGoalStrategyFPPlayWithHumans(robot, config);
  }
  else if ( typeStr == "fp_socialize" ) {
    newStrategy = new AIGoalStrategyFPSocialize(robot, config);
  }
  else if ( typeStr == "spark" ) {
    newStrategy = new AIGoalStrategySpark(robot, config);
  }
  else
  {
    JsonTools::PrintJsonError(config, "AIGoalStrategyFactory.CreateAIGoalStrategy.InvalidType");
    ASSERT_NAMED(false, "AIGoalStrategyFactory.CreateAIGoalStrategy.InvalidType");
  }
  
  // if failed print information to debug
  const bool failed = (nullptr == newStrategy);
  if ( failed )
  {
    PRINT_NAMED_ERROR("AIGoalStrategyFactory.CreateAIGoalStrategy.Fail",
      "Failed to create behavior chooser '%s'. Check log for config.", typeStr.c_str() );
    JsonTools::PrintJsonError(config, "AIGoalStrategyFactory.CreateAIGoalStrategy.Fail");
  }
  
  return newStrategy;
}


}; // namespace
}; // namespace
}; // namespace
