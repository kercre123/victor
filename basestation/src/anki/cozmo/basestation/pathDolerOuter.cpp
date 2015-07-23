/**
 * File: pathDolerOuter.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-06-09
 *
 * Description: This object keeps track of the full-length path on the
 * basestation, and send it out bit by bit to the robot in chunks that
 * it can handle
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include <assert.h>

#include "util/logging/logging.h"
#include "anki/cozmo/basestation/comms/robot/robotMessages.h"

#include "robotMessageHandler.h"
#include "pathDolerOuter.h"

namespace Anki {
namespace Cozmo {

PathDolerOuter::PathDolerOuter(IRobotMessageHandler* msgHandler, RobotID_t robotID)
  : pathSizeOnBasestation_(0)
  , lastDoledSegmentIdx_(-1)
  , msgHandler_(msgHandler)
  , robotID_(robotID)
{
}

void PathDolerOuter::SetPath(const Planning::Path& path)
{
  path_ = path;
  lastDoledSegmentIdx_ = -1;
  pathSizeOnBasestation_ = path.GetNumSegments();

  Dole(MAX_NUM_PATH_SEGMENTS_ROBOT);
}

void PathDolerOuter::ClearPath()
{
  path_.Clear();
  pathSizeOnBasestation_ = 0;
  lastDoledSegmentIdx_ = -1;
}

void PathDolerOuter::Dole(size_t numToDole)
{
  assert(msgHandler_);

  size_t endIdx = lastDoledSegmentIdx_ + numToDole;
  if(endIdx >= pathSizeOnBasestation_)
    endIdx = pathSizeOnBasestation_ - 1;

  printf("PathDolerOuter: should dole from %d to %lu (totalSegments = %lu)\n",
         lastDoledSegmentIdx_ + 1,
         endIdx,
         pathSizeOnBasestation_);

  for(size_t i = lastDoledSegmentIdx_ + 1; i <= endIdx; ++i) {

    printf("PathDolerOuter: doling out basestation idx %zu :  ", i);
    path_.GetSegmentConstRef(i).Print();

    switch(path_.GetSegmentConstRef(i).GetType()) {

    case Planning::PST_LINE:
    {
      MessageAppendPathSegmentLine m;
      const Planning::PathSegmentDef::s_line* l = &(path_.GetSegmentConstRef(i).GetDef().line);
      m.x_start_mm = l->startPt_x;
      m.y_start_mm = l->startPt_y;
      m.x_end_mm = l->endPt_x;
      m.y_end_mm = l->endPt_y;
            
      m.targetSpeed = path_.GetSegmentConstRef(i).GetTargetSpeed();
      m.accel = path_.GetSegmentConstRef(i).GetAccel();
      m.decel = path_.GetSegmentConstRef(i).GetDecel();
            
      if (msgHandler_->SendMessage(robotID_, m) == RESULT_FAIL) {
        printf("ERROR: failed to send message!");
        return;
      }
      break;
    }
    case Planning::PST_ARC:
    {
      MessageAppendPathSegmentArc m;
      const Planning::PathSegmentDef::s_arc* a = &(path_.GetSegmentConstRef(i).GetDef().arc);
      m.x_center_mm = a->centerPt_x;
      m.y_center_mm = a->centerPt_y;
      m.radius_mm = a->radius;
      m.startRad = a->startRad;
      m.sweepRad = a->sweepRad;
            
      m.targetSpeed = path_.GetSegmentConstRef(i).GetTargetSpeed();
      m.accel = path_.GetSegmentConstRef(i).GetAccel();
      m.decel = path_.GetSegmentConstRef(i).GetDecel();
            
      if (msgHandler_->SendMessage(robotID_, m) == RESULT_FAIL) {
        printf("ERROR: failed to send message!");
        return;
      }
      break;
    }
    case Planning::PST_POINT_TURN:
    {
      MessageAppendPathSegmentPointTurn m;
      const Planning::PathSegmentDef::s_turn* t = &(path_.GetSegmentConstRef(i).GetDef().turn);
      m.x_center_mm = t->x;
      m.y_center_mm = t->y;
      m.targetRad = t->targetAngle;
            
      m.targetSpeed = path_.GetSegmentConstRef(i).GetTargetSpeed();
      m.accel = path_.GetSegmentConstRef(i).GetAccel();
      m.decel = path_.GetSegmentConstRef(i).GetDecel();

      if (msgHandler_->SendMessage(robotID_, m) == RESULT_FAIL) {
        printf("ERROR: failed to send message!");
        return;
      }
      break;
    }
    default:
      PRINT_NAMED_ERROR("Invalid path segment", "Can't send path segment of unknown type");
      return;
            
    }
    
    lastDoledSegmentIdx_ = i;
  }

}

void PathDolerOuter::Update(const s8 currPathIdx, const u8 numFreeSlots)
{
  // assumption: MAX_NUM_PATH_SEGMENTS_ROBOT is even
  const u8 numSegmentsToDole = MAX_NUM_PATH_SEGMENTS_ROBOT/2;
  if(numFreeSlots >= numSegmentsToDole                     // Are there enough free slots?
     && (lastDoledSegmentIdx_ < pathSizeOnBasestation_-1)  // Have we already doled out?
     && ((lastDoledSegmentIdx_ - currPathIdx) <= numSegmentsToDole)) {  // Are there any segments left to dole?
    printf("PDO::Update(%i) re-doling upto %d path segments\n", currPathIdx, numFreeSlots);

    Dole(numSegmentsToDole);
  }   
}


}
}
