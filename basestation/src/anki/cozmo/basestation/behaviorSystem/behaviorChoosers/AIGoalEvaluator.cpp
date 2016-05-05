/**
 * File: AIGoalEvaluator
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: Evaluator/Selector of high level goals/objectives for the robot
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "AIGoalEvaluator.h"

#include "AIGoal.h"

#include "anki/common/basestation/jsonTools.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

const char* AIGoalEvaluator::kSelfConfigKey = "evaluator";
const char* AIGoalEvaluator::kGoalsConfigKey = "goals";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoalEvaluator::AIGoalEvaluator(Robot& robot, const Json::Value& config)
{
  CreateFromConfig(robot, config);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoalEvaluator::~AIGoalEvaluator()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoalEvaluator::CreateFromConfig(Robot& robot, const Json::Value& config)
{
  _goals.clear();

  // read every goal and create them with their own config
  const Json::Value& goalsConfig = config[kGoalsConfigKey];
  if ( !goalsConfig.isNull() )
  {
    // reserve room for goals
    _goals.reserve( goalsConfig.size() );
  
    // iterate all objects in the goals adding them to our container
    Json::Value::const_iterator it = goalsConfig.begin();
    const Json::Value::const_iterator end = goalsConfig.end();
    for(; it != end; ++it )
    {
      const Json::Value& goalConfig = *it;
    
      AIGoal* goal = new AIGoal();
      bool goalInitOk = goal->Init( robot, goalConfig );
      if ( goalInitOk )
      {
        // add the goal to our container
        _goals.emplace_back( goal );
      }
      else
      {
        // flag as failed and log
        PRINT_NAMED_ERROR("AIGoalEvaluator.Init", "Failed to initialize goal from config. Look for 'AIGoalEvaluator.Init.BadGoalConfig' event for information");
        JsonTools::PrintJsonError(goalConfig, "AIGoalEvaluator.Init.BadGoalConfig");
        
        // delete this goal, it did not initialize properly
        Anki::Util::SafeDelete( goal );
      }
      
    }
  }
}

} // namespace Cozmo
} // namespace Anki
