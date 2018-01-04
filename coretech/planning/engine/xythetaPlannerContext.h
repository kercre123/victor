/**
 * File: xythetaPlannerContext.h
 *
 * Author: Brad Neuman
 * Created: 2015-09-14
 *
 * Description: The context is the interface between the xythetaPlanner and the rest of the world. All
 *              parameters and access flows through this context, which makes it easy to save and load the
 *              entire context for reproducibilty
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef _ANKICORETECH_PLANNING_XYTHETA_PLANNER_CONTEXT_H_
#define _ANKICORETECH_PLANNING_XYTHETA_PLANNER_CONTEXT_H_

#include "coretech/planning/engine/xythetaEnvironment.h"
#include "coretech/planning/shared/goalDefs.h"
#include "json/json-forwards.h"
#include "util/helpers/noncopyable.h"
#include <vector>
#include <utility>

namespace Anki {

namespace Util {
class JsonWriter;
}

namespace Planning {

// Copying the environment would be expensive, and there's no reason we should need to
struct xythetaPlannerContext : private Util::noncopyable
{
  xythetaPlannerContext();

  void Reset();

  // returns true if successful.
  bool Import(const Json::Value& config);
  void Dump(Util::JsonWriter& writer) const;

  xythetaEnvironment env;

  // Goals in meters and radians. Elements remain sorted, but might not have consecutive GoalIDs
  std::vector<std::pair<GoalID, State_c>> goals_c;

  // Set start in meters and radians
  State_c start;

  // if true, turning in place at the goal has zero cost
  bool allowFreeTurnInPlaceAtGoal;

  // If true, then the next time we plan, we should do it from scratch instead of allowing replanning
  bool forceReplanFromScratch;
};

}
}


#endif
