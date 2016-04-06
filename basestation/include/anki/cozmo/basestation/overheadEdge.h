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
#include "anki/common/basestation/math/quad_impl.h"

namespace Anki {
namespace Cozmo {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Point in an edge
  struct OverheadEdgePoint {
    Point2f      position;
    Vec3f        gradient;
  };

  // container of points
  using OverheadEdgePointVector = std::vector<OverheadEdgePoint>;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Chain of points that define a segment
  struct OverheadEdgePointChain {
    OverheadEdgePointChain() : isBorder(true) { }
    OverheadEdgePointVector points;
    bool isBorder; // isBorder: true = detected border; false = reached end of ground plane without detecting a border
  };

  // container of chains
  using OverheadEdgeChainVector = std::vector<OverheadEdgePointChain>;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // information processed for a frame at the given timestamp
  struct OverheadEdgeFrame {
  OverheadEdgeFrame() : timestamp(0), groundPlaneValid(false) {}
    TimeStamp_t timestamp;
    bool groundPlaneValid;
    Quad2f groundplane;
    OverheadEdgeChainVector chains;
  };
    
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_OverheadEdge_H__
