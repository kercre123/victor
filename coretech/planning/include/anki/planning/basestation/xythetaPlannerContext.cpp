/**
 * File: xythetaPlannerContext.cpp
 *
 * Author: Brad Neuman
 * Created: 2015-09-23
 *
 * Description: The context is the interface between the xythetaPlanner and the rest of the world. All
 *              parameters and access flows through this context, which makes it easy to save and load the
 *              entire context for reproducibilty
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "json/json.h"
#include "util/jsonWriter/jsonWriter.h"
#include "util/logging/logging.h"
#include "xythetaPlannerContext.h"

namespace Anki {
namespace Planning {

xythetaPlannerContext::xythetaPlannerContext()
{
  Reset();
}

void xythetaPlannerContext::Reset()
{
  goal = State_c{ 0.0f, 0.0f, 0.0f };
  start = State_c{ 0.0f, 0.0f, 0.0f };
  allowFreeTurnInPlaceAtGoal = false;
  forceReplanFromScratch = false;
  env.ClearObstacles();
}

bool xythetaPlannerContext::Import(const Json::Value& config)
{
  try {
    if( ! env.Import( config["env"] ) ) {
      return false;
    }

    if( ! goal.Import( config["goal"] ) ) {
      return false;
    }

    if( ! start.Import( config["start"] ) ) {
      return false;
    }

    allowFreeTurnInPlaceAtGoal = config["free_turn_at_goal"].asBool();
    forceReplanFromScratch = config["force_replan"].asBool();
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("xythetaPlannerContext.Import.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }

  return true;
}

void xythetaPlannerContext::Dump(Util::JsonWriter& writer) const
{
  writer.StartGroup("env");
  env.Dump(writer);
  writer.EndGroup();

  writer.StartGroup("goal");
  goal.Dump(writer);
  writer.EndGroup();

  writer.StartGroup("start");
  start.Dump(writer);
  writer.EndGroup();

  writer.AddEntry("free_turn_at_goal", (int)allowFreeTurnInPlaceAtGoal);
  writer.AddEntry("force_replan", (int)forceReplanFromScratch);
}

}
}
