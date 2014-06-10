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

#include "anki/cozmo/basestation/messages.h"
#include "messageHandler.h"
#include "pathDolerOuter.h"

namespace Anki {
namespace Cozmo {

PathDolerOuter::PathDolerOuter(IMessageHandler* msgHandler, RobotID_t robotID)
  : robotIdx_(0)
  , pathSizeOnBasestation_(0)
  , msgHandler_(msgHandler)
  , robotID_(robotID)
{
}

void PathDolerOuter::SetPath(const Planning::Path& path)
{
  path_ = path;
  robotIdx_ = 0;
  pathSizeOnBasestation_ = path.GetNumSegments();

  Dole(0, MAX_NUM_PATH_SEGMENTS_ROBOT);
}

void PathDolerOuter::Dole(size_t validRobotPathLength, size_t numToDole)
{
  assert(msgHandler_);

  size_t endIdx = robotIdx_ + numToDole;
  if(endIdx > pathSizeOnBasestation_)
    endIdx = pathSizeOnBasestation_;
  for(size_t i = robotIdx_ + validRobotPathLength; i < endIdx; ++i) {

    printf("PathDolerOuter: doling out basestation idx %zu, robotIdx_ = %zu\n", i, robotIdx_);

    switch(path_.GetSegmentConstRef(i).GetType()) {

    case Planning::PST_LINE:
    {
      MessageAppendPathSegmentLine m;
      const Planning::PathSegmentDef::s_line* l = &(path_.GetSegmentConstRef(i).GetDef().line);
      m.x_start_mm = l->startPt_x;
      m.y_start_mm = l->startPt_y;
      m.x_end_mm = l->endPt_x;
      m.y_end_mm = l->endPt_y;
      m.pathID = 0;
      m.segmentID = i - robotIdx_;
            
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
      m.pathID = 0;
      m.segmentID = i - robotIdx_;
            
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
      m.pathID = 0;
      m.segmentID = i - robotIdx_;
            
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
  }
}

void PathDolerOuter::Update(s8 indexOnRobotPath)
{
  // assumption: MAX_NUM_PATH_SEGMENTS_ROBOT is even
  if(indexOnRobotPath >= MAX_NUM_PATH_SEGMENTS_ROBOT/2) {
    assert(msgHandler_);

    MessageTrimPath msg;
    msg.numPopFrontSegments = indexOnRobotPath;
    msg.numPopBackSegments = 0;
    msgHandler_->SendMessage(robotID_, msg);

    robotIdx_ += indexOnRobotPath;
    Dole(MAX_NUM_PATH_SEGMENTS_ROBOT/2, MAX_NUM_PATH_SEGMENTS_ROBOT/2);
  }   
}


}
}
