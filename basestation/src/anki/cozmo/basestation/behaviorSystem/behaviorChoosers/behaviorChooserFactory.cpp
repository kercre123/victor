/**
 * File: behaviorChooserFactory
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: Exactly what it sounds like, a factory for behavior choosers.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/behaviorChooserFactory.h"

// behavior choosers
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityFreeplay.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/selectionBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/scoringBehaviorChooser.h"

#include "anki/common/basestation/jsonTools.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "json/json.h"

#include <algorithm>

namespace Anki {
namespace Cozmo {
namespace BehaviorChooserFactory {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorChooser* CreateBehaviorChooser(Robot& robot, const Json::Value& config)
{
  IBehaviorChooser* newChooser = nullptr;

  // extract type
  const Json::Value& type = config["type"];
  std::string typeStr = type.isNull() ? "(type_not_found)" : type.asString();
  std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::tolower);
  
  // should this be more sophisticated than string compare?
  if ( typeStr == "scoring" )
  {
    newChooser = new ScoringBehaviorChooser(robot, config);
  }
  else if ( typeStr == "selection" ) {
    newChooser = new SelectionBehaviorChooser(robot, config);
  }
  else
  {
    JsonTools::PrintJsonError(config, "BehaviorChooserFactory.CreateBehaviorChooser.InvalidType");
    DEV_ASSERT(false, "BehaviorChooserFactory.CreateBehaviorChooser.InvalidType");
  }
  
  // if failed print information to debug
  const bool failed = (nullptr == newChooser);
  if ( failed )
  {
    PRINT_NAMED_ERROR("BehaviorChooserFactory.CreateBehaviorChooser.Fail",
      "Failed to create behavior chooser '%s'. Check log for config.", typeStr.c_str() );
    JsonTools::PrintJsonError(config, "BehaviorChooserFactory.CreateBehaviorChooser.Fail");
  }
  
  return newChooser;
}

}; // namespace BehaviorChooserFactory
} // namespace Cozmo
} // namespace Anki
