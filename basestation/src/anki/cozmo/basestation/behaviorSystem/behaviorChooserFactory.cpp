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
#include "behaviorChooserFactory.h"

// todo organize the choosers in a subfolder
#include "anki/cozmo/basestation/behaviorChooser.h"


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
  IBehaviorChooser* newChooserBase = nullptr;

  // extract type
  const Json::Value& type = config["type"];
  std::string typeStr = type.isNull() ? "(type_not_found)" : type.asString();
  std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::tolower);

  // should this be more sophisticated than string compare?
  if ( typeStr == "simple" )
  {
    // create behavior
    SimpleBehaviorChooser* newChooser = new SimpleBehaviorChooser(robot, config);
    
    // todo rsam: I would like to validate configs, but currenly chooser load in the constructor and never fail
//    // try to initialize, and if it fails destroy the instance
//    const bool initOk = newChooser->ReadFromJson(robot, config);
//    if ( !initOk ) {
//      DestroyBehaviorChooser(newChooser);
//    } else {
      newChooserBase = newChooser;
//    }
  }
  else
  {
    JsonTools::PrintJsonError(config, "BehaviorChooserFactory.CreateBehaviorChooser.InvalidType");
    ASSERT_NAMED(false, "BehaviorChooserFactory.CreateBehaviorChooser.InvalidType");
  }
  
  // if failed print information to debug
  const bool failed = (nullptr == newChooserBase);
  if ( failed )
  {
    PRINT_NAMED_ERROR("BehaviorChooserFactory.CreateBehaviorChooser.Fail",
      "Failed to create behavior chooser '%s'. Check log for config.", typeStr.c_str() );
    JsonTools::PrintJsonError(config, "BehaviorChooserFactory.CreateBehaviorChooser.Fail");
  }
  
  return newChooserBase;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DestroyBehaviorChooser(IBehaviorChooser* &chooserPtr)
{
  delete chooserPtr;
  chooserPtr = nullptr;
}

}; // namespace BehaviorChooserFactory
} // namespace Cozmo
} // namespace Anki
