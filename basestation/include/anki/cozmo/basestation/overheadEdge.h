/**
 * File: overheadEdge.h
 *
 * Author: Andrew Stein
 * Date:   3/1/2016
 *
 * Description: Defines a container for holding edge information found in 
 *              Cozmo's overhead ground-plane image.
 *
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Anki_Cozmo_Basestation_OverheadEdge_H__
#define __Anki_Cozmo_Basestation_OverheadEdge_H__

#include "anki/common/types.h"
#include "anki/common/basestation/math/point.h"

namespace Anki {
namespace Cozmo {

  // Point in an edge
  struct OverheadEdgePoint {
    Point<2,s32> position;
    Vec3f        gradient;
  };

  // container of points
  using OverheadEdgePointVector = std::vector<OverheadEdgePoint>;
  
  // Chain of points in what seems to be the same edge
  struct OverheadEdgePointChain {
    OverheadEdgePointChain() : timestamp(0){}
    OverheadEdgePointVector points;
    TimeStamp_t timestamp;
  };
  
  // Container of chains of points
  using OverheadEdgeVector = std::vector<OverheadEdgePointChain>;
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_OverheadEdge_H__
