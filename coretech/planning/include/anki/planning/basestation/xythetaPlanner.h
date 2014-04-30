/**
 * File: xythetaPlanner.h
 *
 * Author: Brad Neuman
 * Created: 2014-04-30
 *
 * Description: A* lattice planner
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKICORETECH_PLANNING_XYTHETA_PLANNER_H_
#define _ANKICORETECH_PLANNING_XYTHETA_PLANNER_H_

namespace Anki
{
namespace Planning
{

struct xythetaPlannerImpl;

class xythetaPlanner
{
public:

  xythetaPlanner();
  ~xythetaPlanner();

  void SayHello() const;

private:
  xythetaPlannerImpl* _impl;

};

}
}

#endif
