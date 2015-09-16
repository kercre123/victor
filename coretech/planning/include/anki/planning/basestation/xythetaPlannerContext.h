/**
 * File: xythetaPlannerContext.h
 *
 * Author: Brad Neuman
 * Created: 2015-09-14
 *
 * Description: The context is the interface between the xythetaPlanner and the rest of the world. All
 * parameters and access flows through this context, which makes it easy to save and load the entire context
 * for reproducibilty
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef _ANKICORETECH_PLANNING_XYTHETA_PLANNER_CONTEXT_H_
#define _ANKICORETECH_PLANNING_XYTHETA_PLANNER_CONTEXT_H_

#include "anki/planning/basestation/xythetaEnvironment.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Planning {

// Copying the environment would be expensive, and there's no reason we should need to
struct xythetaPlannerContext : private Util::noncopyable
{
  xythetaPlannerContext()
    {
      Reset();
    }

  void Reset() {
    goal = State_c{ 0.0f, 0.0f, 0.0f };
    start = State_c{ 0.0f, 0.0f, 0.0f };
    allowFreeTurnInPlaceAtGoal = false;
    forceReplanFromScratch = false;
    env.ClearObstacles();
  }

  xythetaEnvironment env;

  // Set start and goal in meters and radians
  State_c goal;
  State_c start;

  // if true, turning in place at the goal has zero cost
  bool allowFreeTurnInPlaceAtGoal;

  // If true, then the next time we plan, we should do it from scratch instead of allowing replanning
  bool forceReplanFromScratch;
};

}
}


#endif
