#include "anki/planning/basestation/xythetaPlanner.h"

#include <iostream>
#include "xythetaPlanner_internal.h"

namespace Anki {
namespace Planning {

xythetaPlanner::xythetaPlanner(const xythetaEnvironment& env)
{
  _impl = new xythetaPlannerImpl(env);
}

xythetaPlanner::~xythetaPlanner()
{
  delete _impl;
  _impl = NULL;
}

void xythetaPlanner::SetGoal(const State_c& goal)
{
  _impl->SetGoal(goal);
}

void xythetaPlanner::ComputePath()
{
  _impl->ComputePath();
}

const xythetaPlan& xythetaPlanner::GetPlan() const
{
  return _impl->plan_;
}

////////////////////////////////////////////////////////////////////////////////
// implementation functions
////////////////////////////////////////////////////////////////////////////////

xythetaPlannerImpl::xythetaPlannerImpl(const xythetaEnvironment& env)
  : env_(env),
    start_(0,0,0)
{
}

void xythetaPlannerImpl::SetGoal(const State_c& goal)
{
  goal_ = env_.State_c2State(goal);
}

void xythetaPlannerImpl::ComputePath()
{
  plan_.Clear();
}

}
}
