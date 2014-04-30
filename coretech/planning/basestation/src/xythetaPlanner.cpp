#include "anki/planning/basestation/xythetaPlanner.h"

#include <iostream>
#include "xythetaPlanner_internal.h"

namespace Anki {
namespace Planning {

xythetaPlanner::xythetaPlanner()
{
  _impl = new xythetaPlannerImpl;
}

xythetaPlanner::~xythetaPlanner()
{
  delete _impl;
  _impl = NULL;
}

void xythetaPlanner::SayHello() const
{
  std::cout<<"Hello form inside the planner!\n";
}

}
}
