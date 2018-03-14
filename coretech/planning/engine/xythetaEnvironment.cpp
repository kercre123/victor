/**
 * File: xythetaEnvironment.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-01-10
 *
 * Description: Environment for lattice planning in x,y,theta space
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/rotatedRect.h"
#include "coretech/common/shared/radians.h"
#include "coretech/common/shared/utilities_shared.h"
#include "coretech/planning/engine/xythetaEnvironment.h"
#include "json/json.h"
#include "util/console/consoleInterface.h"
#include "util/jsonWriter/jsonWriter.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdio.h>

using namespace std;

namespace Anki {
namespace Planning {

CONSOLE_VAR(f32, kXYTPlanner_PointTurnTolOverride_deg, "Planner", 2.0f);

#define LATTICE_PLANNER_ACCEL 200
#define LATTICE_PLANNER_DECEL 200

#define LATTICE_PLANNER_ROT_ACCEL 10
#define LATTICE_PLANNER_ROT_DECEL 10
  
#define LATTICE_PLANNER_POINT_TURN_TOL DEG_TO_RAD(2)

#define REPLAN_PENALTY_BUFFER 0.5

// #define HACK_USE_FIXED_SPEED 60.0

State::State(StateID sid)
  :
  x(xythetaEnvironment::GetXFromStateID(sid)),
  y(xythetaEnvironment::GetYFromStateID(sid)),
  theta(xythetaEnvironment::GetThetaFromStateID(sid))
{
}

bool State::Import(const Json::Value& config)
{
  try {
    if( config.isNull() ) {
      PRINT_NAMED_ERROR("State.Import.Null", "config value is null");
      return false;
    }

    if(!JsonTools::GetValueOptional(config, "x", x) ||
       !JsonTools::GetValueOptional(config, "y", y) ||
       !JsonTools::GetValueOptional(config, "theta", theta)) {
      PRINT_NAMED_ERROR("State.Import.Invalid", "could not parse state, dump follows");
      JsonTools::PrintJsonCout(config, 1);
      return false;
    }
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("State.Import.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }

  return true;
}

void State::Dump(Util::JsonWriter& writer) const
{
  writer.AddEntry("x", x);
  writer.AddEntry("y", y);
  writer.AddEntry("theta", theta);
}

StateID State::GetStateID() const
{
  //return x | y<<MAX_XY_BITS | theta<<(2*MAX_XY_BITS);
  StateID sid;
  sid.s.x = x;
  sid.s.y = y;
  sid.s.theta = theta;
  return sid;
}

bool State::operator==(const State& other) const
{
  // TODO:(bn) use union?
  return x==other.x && y==other.y && theta==other.theta;
}

bool State::operator!=(const State& other) const
{
  // TODO:(bn) use union?
  return x!=other.x || y!=other.y || theta!=other.theta;
}


std::ostream& operator<<(std::ostream& out, const State& state)
{
  return out<<'['<<(int)state.x<<','<<(int)state.y<<','<<(int)state.theta<<']';
}

std::ostream& operator<<(std::ostream& out, const State_c& state)
{
  return out<<'('<<state.x_mm<<','<<state.y_mm<<','<<state.theta<<')';
}

bool State_c::Import(const Json::Value& config)
{
  try {
    if( config.isNull() ) {
      PRINT_NAMED_ERROR("State_c.Import.Null", "config value is null");
      return false;
    }
  
    if(!JsonTools::GetValueOptional(config, "x_mm", x_mm) ||
       !JsonTools::GetValueOptional(config, "y_mm", y_mm) ||
       !JsonTools::GetValueOptional(config, "theta_rads", theta)) {
      PRINT_NAMED_ERROR("State_c.Import.Invalid", "could not parse State_c, dump follows");
      JsonTools::PrintJsonCout(config, 1);
      return false;
    }
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("State_c.Import.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }

  return true;
}

void State_c::Dump(Util::JsonWriter& writer) const
{
  writer.AddEntry("x_mm", x_mm);
  writer.AddEntry("y_mm", y_mm);
  writer.AddEntry("theta_rads", theta);
}

bool IntermediatePosition::Import(const Json::Value& config)
{
  try {
    if( config.isNull() ) {
      PRINT_NAMED_ERROR("IntermediatePosition.Import.Null", "config value is null");
      return false;
    }

    if( ! position.Import(config["position"]) ) {
      return false;
    }
  
    nearestTheta = config["theta"].asInt();
    oneOverDistanceFromLastPosition = config["inverseDist"].asFloat();
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("IntermediatePosition.Import.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }


  return true;
}

void IntermediatePosition::Dump(Util::JsonWriter& writer) const
{
  writer.StartGroup("position");
  position.Dump(writer);
  writer.EndGroup();

  writer.AddEntry("theta", nearestTheta);
  writer.AddEntry("inverseDist", oneOverDistanceFromLastPosition);
}


SuccessorIterator::SuccessorIterator(const xythetaEnvironment* env, StateID startID, Cost startG, bool reverse)
  : start_c_(env->StateID2State_c(startID))
  , start_(startID)
  , startG_(startG)
  , nextAction_(0)
  , reverse_(reverse)
{
  assert(start_.theta == xythetaEnvironment::GetThetaFromStateID(startID));
}

// TODO:(bn) inline?
bool SuccessorIterator::Done(const xythetaEnvironment& env) const
{
  if( ! reverse_ ) {
    return nextAction_ > env.allMotionPrimitives_[start_.theta].size();
  }
  else {
    return nextAction_ > env.reverseMotionPrimitives_[start_.theta].size();
  }
}

Cost xythetaEnvironment::ApplyAction(const ActionID& action, StateID& stateID, bool checkCollisions) const
{
  State curr(stateID);
  float start_x = GetX_mm(curr.x);
  float start_y = GetY_mm(curr.y);

  assert(curr.theta >= 0);
  assert(curr.theta < allMotionPrimitives_.size());

  if(action >= allMotionPrimitives_[curr.theta].size())
    return FATAL_OBSTACLE_COST;

  const MotionPrimitive* prim = & allMotionPrimitives_[curr.theta][action];

  Cost penalty = 0.0;

  if(checkCollisions) {
    // size_t endObs = obstacles_.size();
    // size_t endPoints = prim->intermediatePositions.size();
    // for(size_t point=0; point < endPoints; ++point) {
    //   for(size_t obs=0; obs < endObs; ++obs) {
    //     float x = start_x + prim->intermediatePositions[point].position.x_mm;
    //     float y = start_y + prim->intermediatePositions[point].position.y_mm;
    //     if(obstacles_[obs].first.Contains(x, y)) {
    //       penalty += obstacles_[obs].second;
    //       if( penalty >= MAX_OBSTACLE_COST) {
    //         return penalty;
    //       }
    //     }
    //   }
    // }

    size_t endPoints = prim->intermediatePositions.size();

    for(size_t point=0; point < endPoints; ++point) {
      StateTheta angle = prim->intermediatePositions[point].nearestTheta;
      size_t endObs = obstaclesPerAngle_[angle].size();

      for(size_t obs = 0; obs < endObs; ++obs) {

        if(obstaclesPerAngle_[angle][obs].first.Contains(
             start_x + prim->intermediatePositions[point].position.x_mm,
             start_y + prim->intermediatePositions[point].position.y_mm ) ) {

          penalty += obstaclesPerAngle_[angle][obs].second *
            prim->intermediatePositions[point].oneOverDistanceFromLastPosition;
          if( penalty >= MAX_OBSTACLE_COST) {
            return penalty;
          }
        }
      }
    }
  }

  State result(curr);
  result.x += prim->endStateOffset.x;
  result.y += prim->endStateOffset.y;
  result.theta = prim->endStateOffset.theta;

  stateID = result.GetStateID();
  return penalty;
}

Cost xythetaEnvironment::ApplyPathSegment(const PathSegment& pathSegment,
                                          State_c& state_c,
                                          bool checkCollisions,
                                          float maxPenalty) const
{
  Cost penalty = 0.0f;
  
  float endX, endY, endAngle;
  pathSegment.GetEndPose(endX, endY, endAngle);
  State_c endState_c = State_c(endX, endY, endAngle);
  
  if( !checkCollisions ) {
    state_c = endState_c;
    return penalty;
  }
  
  const PathSegmentDef& segmentDef = pathSegment.GetDef();
  const PathSegmentType& segmentType = pathSegment.GetType();
  
  float start_x = state_c.x_mm;
  float start_y = state_c.y_mm;
  float start_theta = state_c.theta;
  
  // intermediate state to move along path
  State_c intermediate_c = state_c;

  // figure out increments to path
  
  // since mprim.json uses an intermediate step size of 0.25 in positon, and PI/32 in orientation,
  // find a step size that matches the smaller of those
  const float kPrimPositionStepSizeRecip = 4.0f;
  const float kPrimAngleStepSizeRecip = 32/M_PI_F;
  const size_t numPointsPosition = std::ceil(1 + pathSegment.GetLength() * kPrimPositionStepSizeRecip);
  const size_t numPointsAngle
    = std::ceil(Anki::Util::Abs(Radians(segmentDef.turn.targetAngle - start_theta).ToFloat()) * kPrimAngleStepSizeRecip);
  // number of points, including start and end
  const size_t numIntermediatePoints = Anki::Util::Max((size_t)2, Anki::Util::Max(numPointsAngle, numPointsPosition));
  DEV_ASSERT(numIntermediatePoints>1,"Number of intermediate points for ApplyPathSegments must be >= 2");
  
  float dx = 0.0f;
  float dy = 0.0f;
  float dtheta = 0.0f;
  const float step = 1.0f/(numIntermediatePoints-1);
  const float oneOverDistFromLastPosition = (PST_POINT_TURN == segmentType)
                                            ? (numIntermediatePoints-1) / segmentDef.arc.sweepRad
                                            : (numIntermediatePoints-1) / pathSegment.GetLength();
  
  switch( segmentType ) {
    case PST_POINT_TURN:
    {
      dtheta = step*Radians(segmentDef.turn.targetAngle - start_theta).ToFloat();
      break;
    }
    case PST_LINE:
    {
      dx = step*(segmentDef.line.endPt_x - segmentDef.line.startPt_x);
      dy = step*(segmentDef.line.endPt_y - segmentDef.line.startPt_y);
      break;
    }
    case PST_ARC:
    {
      // dont use dx and dy here, instead compute x and y in the loop below
      dtheta = step*segmentDef.arc.sweepRad;
      break;
    }
    default:
      PRINT_NAMED_ERROR("xythetaEnvironment.ApplyPathSegment", "Undefined segment %d", segmentType);
      assert(false);
      return MAX_OBSTACLE_COST;
  }

  StateTheta intermediateTheta = GetTheta(intermediate_c.theta);
  for(size_t i=1; i < numIntermediatePoints; ++i) {
    if( segmentType == PST_POINT_TURN ) {
      intermediate_c.theta = start_theta + dtheta*i;
      intermediateTheta = GetTheta(intermediate_c.theta);
    } else if( segmentType == PST_LINE ) {
      intermediate_c.x_mm = start_x + dx*i;
      intermediate_c.y_mm = start_y + dy*i;
    } else if( segmentType == PST_ARC ) {
      intermediate_c.x_mm = segmentDef.arc.centerPt_x + segmentDef.arc.radius * cosf(segmentDef.arc.startRad + dtheta*i);
      intermediate_c.y_mm = segmentDef.arc.centerPt_y + segmentDef.arc.radius * sinf(segmentDef.arc.startRad + dtheta*i);
      intermediate_c.theta = start_theta + dtheta*i;
      intermediateTheta = GetTheta(intermediate_c.theta);
    }
    
    size_t endObs = obstaclesPerAngle_[intermediateTheta].size();
    for(size_t obs=0; obs<endObs; ++obs) {
      if( obstaclesPerAngle_[intermediateTheta][obs].first.Contains(intermediate_c.x_mm, intermediate_c.y_mm) ) {
        penalty += obstaclesPerAngle_[intermediateTheta][obs].second * oneOverDistFromLastPosition;
        if( penalty >= maxPenalty) {
          return penalty;
        }
      }
    }
  }
  
  // no fatal cost, so update state
  state_c = endState_c;
  return penalty;
}


bool xythetaEnvironment::PlanIsSafe(const xythetaPlan& plan,
                                    const float maxDistancetoFollowOldPlan_mm,
                                    int currentPathIndex) const
{
  State_c waste1;
  xythetaPlan waste2;
  return PlanIsSafe(plan, maxDistancetoFollowOldPlan_mm, currentPathIndex, waste1, waste2);
}

bool xythetaEnvironment::PlanIsSafe(const xythetaPlan& plan,
                                    const float maxDistancetoFollowOldPlan_mm,
                                    int currentPathIndex,
                                    State_c& lastSafeState,
                                    xythetaPlan& validPlan) const
{
  if(plan.Empty())
    return false;

  validPlan.Clear();

  size_t numActions = plan.Size();

  StateID curr(plan.start_.GetStateID());
  validPlan.start_ = curr;

  bool useOldPlan = true;

  // first go through all the actions that we are "skipping"
  for(size_t i=0; i<currentPathIndex; ++i) {
    // advance without checking collisions
    ApplyAction(plan.GetAction(i), curr, false);
  }

  State currentRobotState(curr);
  lastSafeState = State2State_c(curr);
  validPlan.start_ = curr;

  float totalPenalty = 0.0;

  // now go through the rest of the plan, checking for collisions at each step
  for(size_t i = currentPathIndex; i < numActions; ++i) {

    // check for collisions and possibly update curr
    Cost actionPenalty = ApplyAction(plan.GetAction(i), curr, true);
    assert(i < plan.Size());

    if(actionPenalty > plan.GetPenalty(i) + REPLAN_PENALTY_BUFFER) {
      printf("Collision along plan action %lu (starting from %d) Penalty increased from %f to %f\n",
             (unsigned long)i,
             currentPathIndex,
             plan.GetPenalty(i),
             actionPenalty);
      // there was a collision trying to follow action i, so we are done
      return false;
    }

    totalPenalty += actionPenalty;

    // no collision. If we are still within
    // maxDistancetoFollowOldPlan_mm, update the valid old plan
    if(useOldPlan) {

      validPlan.Push(plan.GetAction(i), plan.GetPenalty(i));
      lastSafeState = State2State_c(State(curr));

      // TODO:(bn) this is kind of wrong. It's using euclidean
      // distance, instead we should add up the lengths of each
      // action. That will be faster and better
      if(GetDistanceBetween(lastSafeState, currentRobotState) > maxDistancetoFollowOldPlan_mm) {
        useOldPlan = false;
      }
    }
  }

  // if we get here, we successfully applied every action in the plan
  return true;
}

bool xythetaEnvironment::PathIsSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const
{
  size_t numSegments = path.GetNumSegments();
  // choose the penalty for now so that ANY obstacle invalidates the path
  const float maxPenalty = Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE;
  
  if(numSegments == 0) {
    return false;
  }

  validPath.Clear();
  float totalPenalty = 0.0f;
  
  float startX, startY;
  path.GetSegmentConstRef(0).GetStartPoint(startX, startY);
  
  State_c currState_c(startX, startY, startAngle);
  
  for(size_t i = 0; i < numSegments; ++i) {
    const PathSegment& currSegment = path.GetSegmentConstRef(i);
    
    // check for collisions and possibly update curr
    Cost penalty = ApplyPathSegment(currSegment, currState_c, true, maxPenalty);
    totalPenalty += penalty;
    
    if(totalPenalty > maxPenalty) {
      PRINT_NAMED_INFO("xythetaEnvironment.PathIsSafe",
                       "Collision along path segment %lu with penalty %f, total penalty %f",
                       (unsigned long) i,
                       penalty,
                       totalPenalty);
      // there was a collision trying to follow action i, so we are done
      return false;
    }
    
    validPath.AppendSegment(currSegment);
  }

  // if we get here, we successfully applied every action in the path
  return true;
}

void xythetaEnvironment::PrepareForPlanning()
{
  int numObstacles = 0;

  for(size_t angle = 0; angle < numAngles_; ++angle) {
    for( auto& obstaclePair : obstaclesPerAngle_[angle] ) {
      if( angle == 0 ) {
        numObstacles++;
      }
      obstaclePair.first.SortEdgeVectors();
    }
  }

  // fill in the obstacle bounds, default values are set
  obstacleBounds_.resize(numObstacles);
  
  for(size_t angle = 0; angle < numAngles_; ++angle) {
    for( int obsIdx = 0; obsIdx < obstaclesPerAngle_[angle].size(); ++obsIdx ) {
      if( obstaclesPerAngle_[angle][obsIdx].first.GetMinX() < obstacleBounds_[obsIdx].minX ) {
        obstacleBounds_[obsIdx].minX = obstaclesPerAngle_[angle][obsIdx].first.GetMinX();
      }
      if( obstaclesPerAngle_[angle][obsIdx].first.GetMaxX() > obstacleBounds_[obsIdx].maxX ) {
        obstacleBounds_[obsIdx].maxX = obstaclesPerAngle_[angle][obsIdx].first.GetMaxX();
      }

      if( obstaclesPerAngle_[angle][obsIdx].first.GetMinY() < obstacleBounds_[obsIdx].minY ) {
        obstacleBounds_[obsIdx].minY = obstaclesPerAngle_[angle][obsIdx].first.GetMinY();
      }
      if( obstaclesPerAngle_[angle][obsIdx].first.GetMaxY() > obstacleBounds_[obsIdx].maxY ) {
        obstacleBounds_[obsIdx].maxY = obstaclesPerAngle_[angle][obsIdx].first.GetMaxY();
      }
    }
  }
}

const MotionPrimitive& xythetaEnvironment::GetRawMotionPrimitive(StateTheta theta, ActionID action) const
{
  return allMotionPrimitives_[theta][action];
}

void SuccessorIterator::Next(const xythetaEnvironment& env)
{
  size_t numActions = 0;

  if( ! reverse_ ) {
    numActions = env.allMotionPrimitives_[start_.theta].size();
  }
  else {
    numActions = env.reverseMotionPrimitives_[start_.theta].size();
  }

  while(nextAction_ < numActions) {
    const MotionPrimitive* prim = nullptr; 
    if( ! reverse_ ) {
      prim = &env.allMotionPrimitives_[start_.theta][nextAction_];
    }
    else { 
      prim = &env.reverseMotionPrimitives_[start_.theta][nextAction_];
    }

    // collision checking
    long endPoints = prim->intermediatePositions.size();
    bool collision = false; // fatal collision
    bool reverseMotion = env.GetActionType(prim->id).IsReverseAction();

    nextSucc_.g = 0;

    Cost penalty = 0.0f;

    // first, check if we are well-clear of everything, and can skip this check
    bool possibleObstacle = false;

    State_c primtiveOffset = start_c_;

    State result(start_);
    result.x += prim->endStateOffset.x;
    result.y += prim->endStateOffset.y;
    result.theta = prim->endStateOffset.theta;


    if( reverse_ ) {
      primtiveOffset = env.State2State_c(result);
    }

    float minPrimX = prim->minX + primtiveOffset.x_mm;
    float maxPrimX = prim->maxX + primtiveOffset.x_mm;
    float minPrimY = prim->minY + primtiveOffset.y_mm;
    float maxPrimY = prim->maxY + primtiveOffset.y_mm;

    if( env.obstacleBounds_.empty() && ! env.obstaclesPerAngle_[0].empty() ) {
      // unit tests might do this
      PRINT_NAMED_WARNING("xythetaEnvironment.Successor.NoBounds",
                          "missing obstacle bounding boxes! Did you call env.PrepareForPlanning()???");
      possibleObstacle = true;      
    }
    else {
      for( const auto& bound : env.obstacleBounds_ ) {
        if( maxPrimX < bound.minX ||
            minPrimX > bound.maxX ||
            maxPrimY < bound.minY ||
            minPrimY > bound.maxY ) {
          // can't possibly be a collision
          continue;
        }
        // otherwise, we need to do a full check
        possibleObstacle = true;
        break;
      }
    }

    
    if( possibleObstacle ) {

      // two collision check cases. If the angle is changing, then we'll need to potentially switch which
      // obstacle angle we check while checking, so that is the more complciated case

      // First, handle the simpler case, for straight lines. In this case, we can do a quick bounding box check first

      if( prim->endStateOffset.theta == prim->startTheta ) {
        for( const auto& obs : env.obstaclesPerAngle_[prim->startTheta] ) {

          if( maxPrimX < obs.first.GetMinX() ||
              minPrimX > obs.first.GetMaxX() ||
              maxPrimY < obs.first.GetMinY() ||
              minPrimY > obs.first.GetMaxY() ) {
            // can't possibly be a collision, rule out this whole obstacle
            continue;
          }

          for( const auto& pt : prim->intermediatePositions ) {
            if( obs.first.Contains(primtiveOffset.x_mm + pt.position.x_mm,
                                   primtiveOffset.y_mm + pt.position.y_mm ) ) {

              if(obs.second >= MAX_OBSTACLE_COST) {
                collision = true;
                break;
              }
              else {
                // apply soft penalty, but allow the action
                penalty += obs.second * pt.oneOverDistanceFromLastPosition 
                        +  (reverseMotion ? REVERSE_OVER_OBSTACLE_COST : 0);

                assert(!isinf(penalty));
                assert(!isnan(penalty));
              }
            }
          }
        }
      }

      else {
        // handle the more complex case

        for(long pointIdx = endPoints-1; pointIdx >= 0; --pointIdx) {

          StateTheta angle = prim->intermediatePositions[pointIdx].nearestTheta;
          for( const auto& obs : env.obstaclesPerAngle_[angle] ) {
            if( obs.first.Contains(
                  primtiveOffset.x_mm + prim->intermediatePositions[pointIdx].position.x_mm,
                  primtiveOffset.y_mm + prim->intermediatePositions[pointIdx].position.y_mm ) ) {

              if(obs.second >= MAX_OBSTACLE_COST) {
                collision = true;
                break;
              }
              else {
                // apply soft penalty, but allow the action
                penalty += obs.second  * prim->intermediatePositions[pointIdx].oneOverDistanceFromLastPosition
                        +  (reverseMotion ? REVERSE_OVER_OBSTACLE_COST : 0);

                assert(!isinf(penalty));
                assert(!isnan(penalty));
              }
            }
          }
        }
      }
    }

    assert(!isinf(penalty));
    assert(!isnan(penalty));

    nextSucc_.g += penalty;

    if(!collision) {
      nextSucc_.stateID = result.GetStateID();
      nextSucc_.g += startG_ + prim->cost;

      assert(!isinf(nextSucc_.g));
      assert(!isnan(nextSucc_.g));

      nextSucc_.penalty = penalty;
      nextSucc_.actionID = prim->id;
      assert( reverse_ || nextAction_ == prim->id);
      break;
    }

    nextAction_++;
  }

  nextAction_++;
}

bool xythetaEnvironment::IsInCollision(State s) const
{
  return IsInCollision(State2State_c(s));
}

bool xythetaEnvironment::IsInCollision(State_c c) const
{  
  StateTheta angle = GetTheta(c.theta);
  size_t endObs = obstaclesPerAngle_[angle].size();

  for(size_t obsIdx=0; obsIdx<endObs; ++obsIdx) {
    if(obstaclesPerAngle_[angle][obsIdx].first.Contains( c.x_mm, c.y_mm ) &&
       obstaclesPerAngle_[angle][obsIdx].second >= MAX_OBSTACLE_COST)
      return true;
  }
  return false;
}

bool xythetaEnvironment::IsInSoftCollision(State s) const
{
  StateTheta angle = s.theta;
  size_t endObs = obstaclesPerAngle_[angle].size();

  for(size_t obsIdx=0; obsIdx<endObs; ++obsIdx) {
    if(obstaclesPerAngle_[angle][obsIdx].first.Contains( GetX_mm(s.x), GetY_mm(s.y) ))
      return true;
  }
  return false;
}

Cost xythetaEnvironment::GetCollisionPenalty(State s) const
{
  StateTheta angle = s.theta;
  size_t endObs = obstaclesPerAngle_[angle].size();

  for(size_t obsIdx=0; obsIdx<endObs; ++obsIdx) {
    if(obstaclesPerAngle_[angle][obsIdx].first.Contains( GetX_mm(s.x), GetY_mm(s.y) ))
      return obstaclesPerAngle_[angle][obsIdx].second;
  }
  return 0.0;
}

void xythetaPlan::Append(const xythetaPlan& other)
{
  if(_actionCostPairs.empty()) {
    start_ = other.start_;
  }
  _actionCostPairs.insert(_actionCostPairs.end(), other._actionCostPairs.begin(), other._actionCostPairs.end() );
}

ActionType::ActionType()
  : _extraCostFactor(0.0)
  , _id(-1)
  , _name("<invalid>")
  , _reverse(false)
{
}

bool ActionType::Import(const Json::Value& config)
{
  try {
    if( config.isNull() ) {
      PRINT_NAMED_ERROR("ActionType.Import.Null", "config value is null");
      return false;
    }

    if(!JsonTools::GetValueOptional(config, "extra_cost_factor", _extraCostFactor) ||
       !JsonTools::GetValueOptional(config, "index", _id) ||
       !JsonTools::GetValueOptional(config, "name", _name)) {
      printf("error: could not parse ActionType\n");
      JsonTools::PrintJsonCout(config, 1);
      return false;
    }
    JsonTools::GetValueOptional(config, "reverse_action", _reverse);
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("ActionType.Import.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }

  return true;
}

void ActionType::Dump(Util::JsonWriter& writer) const
{
  writer.AddEntry("extra_cost_factor", _extraCostFactor);
  writer.AddEntry("index", _id);
  writer.AddEntry("name", _name);
  writer.AddEntry("reverse_action", _reverse);
}

xythetaEnvironment::~xythetaEnvironment()
{
}

xythetaEnvironment::xythetaEnvironment()
{
  numAngles_ = 1<<THETA_BITS;
  obstaclesPerAngle_.resize(numAngles_);

  radiansPerAngle_ = 2*M_PI/numAngles_;
  oneOverRadiansPerAngle_ = (float)(1.0 / ((double)radiansPerAngle_));
}

bool xythetaEnvironment::Init(const Json::Value& mprimJson)
{
  ClearObstacles();
  return ParseMotionPrims(mprimJson);
}

size_t xythetaEnvironment::GetNumObstacles() const
{
  return obstaclesPerAngle_[0].size();
}

bool xythetaEnvironment::Init(const char* mprimFilename)
{
  if(ReadMotionPrimitives(mprimFilename)) {
    numAngles_ = 1<<THETA_BITS;
    obstaclesPerAngle_.resize(numAngles_);

    radiansPerAngle_ = 2*M_PI/numAngles_;
    oneOverRadiansPerAngle_ = (float)(1.0 / ((double)radiansPerAngle_));
  }
  else {
    PRINT_NAMED_ERROR("xythetaEnvironemnt.Init.Fail", "could not parse motion primitives");
    return false;
  }

  return true;
}

void xythetaEnvironment::Dump(Util::JsonWriter& writer) const
{
  // writer.StartGroup("mprim");
  // DumpMotionPrims(writer);
  // writer.EndGroup();

  writer.StartGroup("obstacles");
  DumpObstacles(writer);
  writer.EndGroup();

  writer.StartGroup("robotParams");
  _robotParams.Dump(writer);
  writer.EndGroup();  
}

bool xythetaEnvironment::Import(const Json::Value& config)
{
  try {
    if( config.isNull() ) {
      PRINT_NAMED_ERROR("xythetaEnvironment.Import.Null", "config value is null");
      return false;
    }

    // if( ! config["mprim"].isNull() ) {
    //   if( ! ParseMotionPrims(config["mprim"], true) ) {
    //     return false;
    //   }
    // }

    if( ! config["obstacles"].isNull() ) {
      if( ! ParseObstacles(config["obstacles"]) ) {
        return false;
      }
    }

    // params are currently fixed, so ignore this
    // if( ! config["robotParams"].isNull() ) {
    //   if( ! _robotParams.Import(config["robotParams"]) ) {
    //     return false;
    //   }
    // }
    // else {
    //   _robotParams.Reset();
    // }
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("xythetaEnvironment.Import.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }

  return true;
}

bool xythetaEnvironment::ReadMotionPrimitives(const char* mprimFilename)
{
  Json::Reader reader;
  Json::Value mprimTree;

  ifstream mprimStream(mprimFilename);

  if(!reader.parse(mprimStream, mprimTree)) {
    cout<<"error: could not parse json form file '"<<mprimFilename
        <<"'\n" << reader.getFormattedErrorMessages()<<endl;
  }
  
  return ParseMotionPrims(mprimTree, false);
}

bool xythetaEnvironment::ParseMotionPrims(const Json::Value& config, bool useDumpFormat)
{
  try {
    if(!JsonTools::GetValueOptional(config, "resolution_mm", resolution_mm_) ||
       !JsonTools::GetValueOptional(config, "num_angles", numAngles_)) {
      printf("error: could not find key 'resolution_mm' or 'num_angles' in motion primitives\n");
      JsonTools::PrintJsonCout(config, 1);
      return false;
    }

    oneOverResolution_ = (float)(1.0 / ((double)resolution_mm_));

    // parse through the action types
    if(config["actions"].size() == 0) {
      printf("empty or non-existant actions section! (old format, perhaps?)\n");
      return false;
    }

    for(const auto & actionConfig : config["actions"]) {
      ActionType at;
      at.Import(actionConfig);
      actionTypes_.push_back(at);
    }

    try {
      for(const auto & angle : config["angle_definitions"]) {
        angles_.push_back(angle.asFloat());
      }
    }
    catch( const std::exception&  e ) {
      PRINT_NAMED_ERROR("ParseMotionPrims.angle_definitions.Exception",
                        "json exception: %s",
                        e.what());
      return false;
    }

    if(angles_.size() != numAngles_) {
      printf("ERROR: numAngles is %u, but we read %lu angle definitions\n",
             numAngles_,
             (unsigned long)angles_.size());
      return false;
    }

    // parse through each starting angle
    if(config["angles"].size() != numAngles_) {
      printf("error: could not find key 'angles' in motion primitives\n");
      JsonTools::PrintJsonCout(config, 1);
      return false;
    }

    allMotionPrimitives_.resize(numAngles_);

    unsigned int numPrims = 0;

    try {
      for(unsigned int angle = 0; angle < numAngles_; ++angle) {
        Json::Value prims = config["angles"][angle]["prims"];
        for(unsigned int i = 0; i < prims.size(); ++i) {
          MotionPrimitive p;

          if( useDumpFormat ) {
            if( ! p.Import(prims[i]) ) {
              return false;
            }
          }
          else {
            if(!p.Create(prims[i], angle, *this)) {
              PRINT_NAMED_ERROR("ParseMotionPrims.CreateFormat.Mprim", "Failed to import motion primitive");
              return false;
            }
          }

          allMotionPrimitives_[angle].push_back(p);
          numPrims++;
        }
      }
    }
    catch( const std::exception&  e ) {
      PRINT_NAMED_ERROR("ParseMotionPrims.anglesPrims.Exception",
                        "json exception: %s",
                        e.what());
      return false;
    }

    PRINT_NAMED_INFO("ParseMotionPrims.Added", "Added %d motion primitives", numPrims);
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("ParseMotionPrims.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }

  PopulateReverseMotionPrims();

  return true;
}

void xythetaEnvironment::PopulateReverseMotionPrims()
{
  reverseMotionPrimitives_.clear();
  reverseMotionPrimitives_.resize(numAngles_);

  // go through each motion primitive, and populate the corresponding reverse primitive
  for(int startAngle = 0; startAngle < numAngles_; ++startAngle) {
    for(int actionID = 0; actionID < allMotionPrimitives_[ startAngle ].size(); ++actionID) {
      const MotionPrimitive& prim( allMotionPrimitives_[ startAngle ][ actionID ] );
      int endAngle = prim.endStateOffset.theta;

      MotionPrimitive reversePrim(prim);
      reversePrim.endStateOffset.theta = startAngle;
      reversePrim.endStateOffset.x = -reversePrim.endStateOffset.x;
      reversePrim.endStateOffset.y = -reversePrim.endStateOffset.y;

      reverseMotionPrimitives_[endAngle].push_back( reversePrim );
    }
  }
}

void xythetaEnvironment::DumpMotionPrims(Util::JsonWriter& writer) const
{
  writer.AddEntry("resolution_mm", resolution_mm_);
  writer.AddEntry("num_angles", (int)numAngles_);

  writer.StartList("actions");
  for(const auto& action : actionTypes_) {
    writer.NextListItem();
    action.Dump(writer);
  }
  writer.EndList();

  writer.StartList("angle_definitions");
  for(const auto& angleDef : angles_) {
    writer.AddRawListEntry(angleDef);
  }
  writer.EndList();

  writer.StartList("angles");
  for(unsigned int angle = 0; angle < numAngles_; ++angle) {
    writer.NextListItem();
    writer.StartList("prims");
    for(const auto& prim : allMotionPrimitives_[angle]) {
      writer.NextListItem();
      prim.Dump(writer);
    }
    writer.EndList();
  }
  writer.EndList();  
}

bool xythetaEnvironment::ParseObstacles(const Json::Value& config)
{
  try {
    if( numAngles_ == 0 || config["angles"].isNull() ) {
      PRINT_NAMED_ERROR("xythetaEnvironment.ParseObstacles.InvalidObjectAngles",
                        "numAngles_ = %d",
                        numAngles_);
      return false;
    }

    if( numAngles_ != config["angles"].size() ) {
      PRINT_NAMED_ERROR("xythetaEnvironment.ParseObstacles.AnglesMismatch",
                        "this has %d angles, but json has %d",
                        numAngles_,
                        config.size());
      return false;
    }

    obstaclesPerAngle_.clear();
    obstaclesPerAngle_.resize(numAngles_);

    for( int theta = 0; theta < numAngles_; ++theta ) {

      if( config["angles"][theta]["obstacles"].isNull() ) {
        PRINT_NAMED_ERROR("xythetaEnvironment.ParseObstacles.badConfig",
                          "theta %d contains invalid obstacle (dump follows)",
                          theta);
        JsonTools::PrintJsonCout( config["angles"][theta], 3 );
        return false;
      }

      for( const auto& obsConfig : config["angles"][theta]["obstacles"] ) {
        if( obsConfig["cost"].isNull() ) {
          PRINT_NAMED_ERROR("xythetaEnvironment.ParseObstacles.badConfig",
                            "theta %d contains invalid obstacle (dump follows)",
                            theta);
          JsonTools::PrintJsonCout( obsConfig["cost"], 3 );
          return false;
        }

        Cost c = Util::numeric_cast<Cost>(obsConfig["cost"].asFloat());
        Poly2f p;

        for( const auto& ptConfig : obsConfig["poly"] ) {
          p.push_back( Point2f{ ptConfig["x"].asFloat(), ptConfig["y"].asFloat() } );
        }

        obstaclesPerAngle_[theta].push_back( std::make_pair( FastPolygon{ p }, c ) );
      }
    }
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("ParseObstacles.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }

  return true;
}

void xythetaEnvironment::DumpObstacles(Util::JsonWriter& writer) const
{
  writer.StartList("angles");
  for(const auto& angle : obstaclesPerAngle_) {
    writer.NextListItem();
    writer.StartList("obstacles");
    for(const auto& obs : angle) {
      writer.NextListItem();
      writer.AddEntry("cost", obs.second);

      writer.StartList("poly");      
      for(const auto pt : obs.first) {
        writer.NextListItem();
        writer.AddEntry("x", pt.x());
        writer.AddEntry("y", pt.y());
      }
      writer.EndList();
    }
    writer.EndList();
  }
  writer.EndList();
}

void xythetaEnvironment::AddObstacleAllThetas(const Quad2f& quad, Cost cost)
{
  // for legacy purposes, add this to every single angle!
  Poly2f poly;
  poly.ImportQuad2d(quad);
  FastPolygon fastPoly(poly);

  for(size_t i=0; i<numAngles_; ++i) {
    obstaclesPerAngle_[i].push_back( std::make_pair( fastPoly, cost ) );
  }
}
  
void xythetaEnvironment::AddObstacleAllThetas(const RotatedRectangle& rect, Cost cost)
{
  Poly2f poly(rect);
  FastPolygon fastPoly(poly);

  for(size_t i=0; i<numAngles_; ++i) {
    obstaclesPerAngle_[i].push_back( std::make_pair( fastPoly, cost ) );
  }
}

void xythetaEnvironment::ClearObstacles()
{
  for(size_t i=0; i<numAngles_; ++i) {
    obstaclesPerAngle_[i].clear();
  }
}

FastPolygon xythetaEnvironment::ExpandCSpace(const ConvexPolygon& obstacle,
                                             const ConvexPolygon& robot)
{
  // TODO:(bn) don't copy polygon into vector

  assert(obstacle.size() > 0);
  assert(robot.size() > 0);
  assert(obstacle.GetClockDirection() == ConvexPolygon::CW);
  assert(robot.GetClockDirection() == ConvexPolygon::CW);

  // CoreTechPrint("obstacle: ");
  // obstacle.Print();

  // CoreTechPrint("robot: ");
  // robot.Print();

  // This algorithm is a Minkowski difference. It requires both
  // obstacle and robot to be convex complete polygons in clockwise
  // order. You can also see a python implementation with plotting and
  // such in coretech/planning/matlab/minkowski.py


  Radians startingAngle = obstacle.GetEdgeAngle(0);

  // compute the starting point for each polygon
  //size_t obstacleStart = 0;
  size_t robotStart = 0;

  size_t robotSize = robot.size();
  
  float minDist = FLT_MAX;
  for(size_t i = 0; i < robotSize; ++i) {
    // note that we are "adding" the "negative" robot, so we will
    // consider robot angles to be +pi (opposite)
    Radians angle(robot.GetEdgeAngle(i) + M_PI);
    float dist = angle.angularDistance(startingAngle, false);
    if(dist < minDist) {
      minDist = dist;
      robotStart = i;
    }
  }

  // CoreTechPrint("Robot start %d\n", robotStart);

  // now find the starting point where we will anchor the
  // expansion. Remember the robot is centered at (0,0)
  Point2f curr = obstacle[0] - robot[robotStart];

  // now merge based on angle
  std::vector<Point2f> expansion;

  expansion.push_back(curr);

  size_t robotIdx = robotStart;
  size_t robotNum = 0; // number of points added from the robot
  size_t obstacleIdx = 0;
  size_t obstacleSize = obstacle.size();


  while( obstacleIdx < obstacleSize && robotNum < robotSize ) {
    Radians robotAngle(robot.GetEdgeAngle(robotIdx) + M_PI);
    float robotDiff = robotAngle.angularDistance(startingAngle, false);

    Radians obstacleAngle(obstacle.GetEdgeAngle(obstacleIdx));
    float obstacleDiff = obstacleAngle.angularDistance(startingAngle, false);

    if( obstacleDiff < robotDiff ) {
      Point2f edge = obstacle.GetEdgeVector(obstacleIdx);
      curr += edge;
      expansion.push_back( curr );
      obstacleIdx++;
    }
    else {
      Point2f edge = robot.GetEdgeVector(robotIdx);
      curr -= edge;
      expansion.push_back( curr );
      robotNum++;
      robotIdx = (robotIdx + 1) % robotSize;
    }
  }

  while( obstacleIdx < obstacleSize ) {
    Point2f edge = obstacle.GetEdgeVector(obstacleIdx);
    curr += edge;
    expansion.push_back( curr );
    obstacleIdx++;
  }

  while( robotNum < robotSize ) {
    Point2f edge = robot.GetEdgeVector(robotIdx);
    curr -= edge;
    expansion.push_back( curr );
    robotNum++;
    robotIdx = (robotIdx + 1) % robotSize;
  }

  // the last entry is redundant, so remove it
  // TODO:(bn) don't add it?
  expansion.pop_back();

  // CoreTechPrint("c-space obstacle: ");
  // expansion.Print();

  // VIC-1702: Minkowski sum implementation is subject to floating point errors, so take the convex hull for now
  return FastPolygon{ ConvexPolygon::ConvexHull(std::move(expansion)) };
}


const FastPolygon& xythetaEnvironment::AddObstacleWithExpansion(const ConvexPolygon& obstacle,
                                                                const ConvexPolygon& robot,
                                                                StateTheta theta,
                                                                Cost cost)
{
  if(theta >= obstaclesPerAngle_.size()) {
    CoreTechPrint("ERROR: theta = %d, but only have %zu obstacle angles and %u angles total\n",
                  theta,
                  obstaclesPerAngle_.size(),
                  numAngles_);
    
    theta = 0;
  }

  obstaclesPerAngle_[theta].emplace_back(std::make_pair(ExpandCSpace(obstacle, robot), cost));

  return obstaclesPerAngle_[theta].back().first;
}


bool MotionPrimitive::Create(const Json::Value& config, StateTheta startingAngle, const xythetaEnvironment& env)
{
  startTheta = startingAngle;

  if(!JsonTools::GetValueOptional(config, "action_index", id)) {
    printf("error: missing key 'action_index'\n");
    JsonTools::PrintJsonCout(config, 1);
    return false;
  }
  assert(id >= 0);

  if(!endStateOffset.Import(config["end_pose"])) {
    printf("error: could not read 'end_pose'\n");
    return false;
  }

  unsigned int numIntermediatePoses = config["intermediate_poses"].size();
  for(unsigned int i=0; i<numIntermediatePoses; ++i) {
    State_c s;
    if(!s.Import(config["intermediate_poses"][i])) {
      printf("error: could not read 'intermediate_poses'[%d]\n", i);
        return false;
    }

    State_c old(0, 0, 0);
    float penalty = 0.0;

    if(!intermediatePositions.empty()) {
      old = intermediatePositions.back().position;

      float dist = env.GetDistanceBetween(old, s);

      // TODO:(bn) use actual time / cost computation!
      float cost = dist;

      Radians deltaTheta = Radians(s.theta) - Radians(old.theta);
      cost += std::abs(deltaTheta.ToFloat()) *
        env._robotParams.halfWheelBase_mm *
        env._robotParams.oneOverMaxVelocity;

      penalty = 1.0 / cost;
    }

    intermediatePositions.emplace_back(s, env.GetTheta(s.theta),  penalty);
  }

  if(config.isMember("extra_cost_factor")) {
    printf("ERROR: individual primitives shouldn't have cost factors. Old file format?\n");
    return false;
  }

  double linearSpeed = env._robotParams.maxVelocity_mmps;
  double oneOverLinearSpeed = env._robotParams.oneOverMaxVelocity;
  bool isReverse = env.GetActionType(id).IsReverseAction();
  if(isReverse) {
    linearSpeed = env.GetMaxReverseVelocity_mmps();
    oneOverLinearSpeed = 1.0 / env._robotParams.maxReverseVelocity_mmps;
  }

  // Compute cost based on the action. Cost is time in seconds
  cost = 0.0;

#ifdef HACK_USE_FIXED_SPEED
  linearSpeed = HACK_USE_FIXED_SPEED;
  oneOverLinearSpeed = 1.0 / linearSpeed;
#endif

  double length = std::abs(config["straight_length_mm"].asDouble());
  if(length > 0.0) {
    cost += length * oneOverLinearSpeed;

    float signedLength = config["straight_length_mm"].asFloat();
    
    if(std::abs(signedLength) > 0.001) {
      pathSegments_.AppendLine(0.0,
                               0.0,
                               signedLength * cos(env.GetTheta_c(startingAngle)),
                               signedLength * sin(env.GetTheta_c(startingAngle)),
                               isReverse ? -linearSpeed : linearSpeed,
                               LATTICE_PLANNER_ACCEL,
                               LATTICE_PLANNER_DECEL);
    }
  }

  if(config.isMember("arc")) {
    // the section of the angle we will sweep through
    double deltaTheta = std::abs(config["arc"]["sweepRad"].asDouble());

    // the radius of the circle that the outer wheel will follow
    double turningRadius = std::abs(config["arc"]["radius_mm"].asDouble());
    double radius_mm = turningRadius + env._robotParams.halfWheelBase_mm;

    // the total time is the arclength of the outer wheel arc divided by the max outer wheel speed
    Cost arcTime = deltaTheta * radius_mm * oneOverLinearSpeed;
    cost += arcTime;

    Cost arcSpeed = deltaTheta * turningRadius / arcTime;

    // TODO:(bn) these don't work properly backwards at the moment

#ifdef HACK_USE_FIXED_SPEED
    arcSpeed = HACK_USE_FIXED_SPEED;
#endif

    pathSegments_.AppendArc(config["arc"]["centerPt_x_mm"].asFloat(),
                            config["arc"]["centerPt_y_mm"].asFloat(),
                            config["arc"]["radius_mm"].asFloat(),
                            config["arc"]["startRad"].asFloat(),
                            config["arc"]["sweepRad"].asFloat(),
                            isReverse ? -arcSpeed : arcSpeed,
                            LATTICE_PLANNER_ACCEL,
                            LATTICE_PLANNER_DECEL);
  }
  else if(config.isMember("turn_in_place_direction")) {
    double direction = config["turn_in_place_direction"].asDouble();

    // turn in place is just like an arc with radius 0
    Radians startRads(env.GetTheta_c(startTheta));
    double deltaTheta = startRads.angularDistance(env.GetTheta_c(endStateOffset.theta), direction < 0);

    Cost turnTime = std::abs(deltaTheta) * env._robotParams.halfWheelBase_mm * oneOverLinearSpeed;
    cost += turnTime;

    float rotSpeed = deltaTheta / turnTime;

    pathSegments_.AppendPointTurn(0.0,
                                  0.0,
                                  startRads.ToFloat(),
                                  env.GetTheta_c(endStateOffset.theta),
                                  rotSpeed,
                                  LATTICE_PLANNER_ROT_ACCEL,
                                  LATTICE_PLANNER_ROT_DECEL,
                                  LATTICE_PLANNER_POINT_TURN_TOL,
                                  true);
  }

  assert(env.GetNumActions() > id);

  if(cost < 1e-6) {
    printf("ERROR: base action cost is %f for action %d '%s'\n", cost, id, env.GetActionType(id).GetName().c_str());
    return false;
  }

  // printf("from angle %2d %20s costs %f * %f = %f\n",
  //            startTheta,
  //            env.GetActionType(id).GetName().c_str(),
  //            cost,
  //            env.GetActionType(id).GetExtraCostFactor(),
  //            cost * env.GetActionType(id).GetExtraCostFactor());

  cost *= env.GetActionType(id).GetExtraCostFactor();

  if(cost < 1e-6) {
    printf("ERROR: final action cost is %f (%f x) for action %d '%s\n",
           cost,
           env.GetActionType(id).GetExtraCostFactor(),
           id, env.GetActionType(id).GetName().c_str());
    return false;
  }

  CacheBoundingBox();

  return true;
}

bool MotionPrimitive::Import(const Json::Value& config)
{
  try {
    if( config.isNull() ) {
      return false;
    }

    if( config["action_index"].isNull() ) {
      PRINT_NAMED_ERROR("MotionPrimitive.Import.InvalidConfig",
                        "no action_index in config. dump follows");
      JsonTools::PrintJsonCout(config, 3);
      return false;
    }

    if( config["cost"].isNull() ) {
      PRINT_NAMED_ERROR("MotionPrimitive.Import.InvalidConfig2",
                        "missing 'cost' key. Did you mean to call Create() instead of Import()?");
      JsonTools::PrintJsonCout(config, 3);
      return false;
    }

    id = config["action_index"].asInt();
    startTheta = config["start_theta"].asInt();
    cost = config["cost"].asFloat();
    if( ! endStateOffset.Import(config["end_state_offset"]) ) {
      return false;
    }
  
    if(config["intermediate_poses"].size() <= 1 ) {
      PRINT_NAMED_ERROR("MotionPrimitive.Import.InvalidConfig3",
                        "'intermediate_poses' size %d too small (or not a list). Dump follows",
                        config["intermediate_poses"].size());
      JsonTools::PrintJsonCout(config["intermediate_poses"], 3);
      return false;
    }

    intermediatePositions.clear();

    for(const auto& poseConfig : config["intermediate_poses"]) {
      IntermediatePosition p;
      if( ! p.Import( poseConfig ) ) {
        return false;
      }
      intermediatePositions.push_back(p);
    }
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("MotionPrimitive.Import.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }

  CacheBoundingBox();

  return true;
}

void MotionPrimitive::CacheBoundingBox()
{
  minX = numeric_limits<decltype(minX)>::max();
  maxX = numeric_limits<decltype(maxX)>::min();
  minY = numeric_limits<decltype(minY)>::max();
  maxY = numeric_limits<decltype(maxY)>::min();

  for( const auto& pt : intermediatePositions ) {
    if( pt.position.x_mm < minX ) {
      minX = pt.position.x_mm;
    }
    if( pt.position.x_mm > maxX ) {
      maxX = pt.position.x_mm;
    }

    if( pt.position.y_mm < minY ) {
      minY = pt.position.y_mm;
    }
    if( pt.position.y_mm > maxY ) {
      maxY = pt.position.y_mm;
    }
  }
}

void MotionPrimitive::Dump(Util::JsonWriter& writer) const
{
  writer.AddEntry("action_index", id);
  writer.AddEntry("start_theta", startTheta);
  writer.AddEntry("cost", cost);

  writer.StartGroup("end_state_offset");
  endStateOffset.Dump(writer);
  writer.EndGroup();

  writer.StartList("intermediate_poses");
  for(const auto& pose : intermediatePositions) {
    writer.NextListItem();
    pose.Dump(writer);
  }
  writer.EndList();
}


u8 MotionPrimitive::AddSegmentsToPath(State_c start, Path& path) const
{
  State_c curr(start);

  bool added = false;
  u8 firstSegment = path.GetNumSegments();

  for(u8 pathIdx = 0; pathIdx < pathSegments_.GetNumSegments(); ++pathIdx) {
    const PathSegment& seg(pathSegments_.GetSegmentConstRef(pathIdx));
    PathSegment segment(seg);
    segment.OffsetStart(curr.x_mm, curr.y_mm);

#if REMOTE_CONSOLE_ENABLED
    if( segment.GetType() == PST_POINT_TURN) {
      segment.GetDef().turn.angleTolerance = DEG_TO_RAD(kXYTPlanner_PointTurnTolOverride_deg);
    }
#endif


    float xx, yy, aa;
    segment.GetStartPoint(xx,yy);
    // printf("start: (%f, %f)\n", xx, yy);

    // if this segment can be combined with the previous one, do
    // that. otherwise, append a new segment.
    bool shouldAdd = true;
    if(path.GetNumSegments() > 0 && path[path.GetNumSegments()-1].GetType() == segment.GetType()) {
      size_t endIdx = path.GetNumSegments()-1;

      switch(segment.GetType()) {

      case PST_LINE:
      {
        bool oldSign = path[path.GetNumSegments()-1].GetTargetSpeed() > 0;
        bool newSign = segment.GetTargetSpeed() > 0;
        if(oldSign == newSign) {
          path[endIdx].GetDef().line.endPt_x = segment.GetDef().line.endPt_x;
          path[endIdx].GetDef().line.endPt_y = segment.GetDef().line.endPt_y;
          shouldAdd = false;
        }
        break;
      }

      case PST_ARC:
        // TODO:(bn) had to disable this because it was combing arcs
        // that the robot was going to split up. This doesn't happen
        // in the lattice planner anyway (its always line, arc for
        // each turn action)

        // if(FLT_NEAR(path[endIdx].GetDef().arc.centerPt_x, segment.GetDef().arc.centerPt_x) &&
        //        FLT_NEAR(path[endIdx].GetDef().arc.centerPt_y, segment.GetDef().arc.centerPt_y) &&
        //        FLT_NEAR(path[endIdx].GetDef().arc.radius, segment.GetDef().arc.radius)) {

        //   path[endIdx].GetDef().arc.sweepRad += segment.GetDef().arc.sweepRad;
        //   shouldAdd = false;
        // }
        break;

      case PST_POINT_TURN:
        // only combine point turns if they are the same and the new
        // target angle is less that 180 degrees away from the current
        // angle
        if(FLT_NEAR(path[endIdx].GetDef().turn.x, segment.GetDef().turn.x) &&
           FLT_NEAR(path[endIdx].GetDef().turn.y, segment.GetDef().turn.y) &&
           FLT_NEAR(path[endIdx].GetTargetSpeed(), segment.GetTargetSpeed())) {
          path[endIdx].GetDef().turn.targetAngle = segment.GetDef().turn.targetAngle;
          shouldAdd = false;
        }
        break;

      default:
        printf("ERROR (AddSegmentsToPath): Undefined segment %d\n", segment.GetType());
        assert(false);
      }
    }

    if(shouldAdd) {
      path.AppendSegment(segment);
      added = true;
    }

    segment.GetEndPose(xx,yy,aa);
    // printf("end: (%f, %f)\n", xx, yy);
  }

  if(!added && firstSegment > 0)
    firstSegment--;

  return firstSegment;
}


bool xythetaEnvironment::RoundSafe(const State_c& c, State& rounded) const
{
  float bestDist2 = 999999.9;

  // TODO:(bn) smarter rounding for theta?
  rounded.theta = GetTheta(c.theta);

  StateXY startX = (StateXY) floor(c.x_mm * oneOverResolution_);
  StateXY endX   = (StateXY)  ceil(c.x_mm * oneOverResolution_);
  StateXY startY = (StateXY) floor(c.y_mm * oneOverResolution_);
  StateXY endY   = (StateXY)  ceil(c.y_mm * oneOverResolution_);

  for(StateXY x = startX; x <= endX; ++x) {
    for(StateXY y = startY; y <= endY; ++y) {
      State candidate(x, y, rounded.theta);
      if(!IsInCollision(candidate)) {
        float dist2 = pow(GetX_mm(x) - c.x_mm, 2) + pow(GetY_mm(y) - c.y_mm, 2);
        if(dist2 < bestDist2) {
          bestDist2 = dist2;
          rounded.x = x;
          rounded.y = y;
        }
      }
    }
  }

  return bestDist2 < 999999.8;
}

float xythetaEnvironment::GetDistanceBetween(const State_c& start, const State& end) const
{
  float distSq = 
    pow(GetX_mm(end.x) - start.x_mm, 2)
    + pow(GetY_mm(end.y) - start.y_mm, 2);

  return sqrtf(distSq);
}

float xythetaEnvironment::GetDistanceBetween(const State_c& start, const State_c& end)
{
  float distSq = 
    pow(end.x_mm - start.x_mm, 2)
    + pow(end.y_mm - start.y_mm, 2);

  return sqrtf(distSq);
}

float xythetaEnvironment::GetMinAngleBetween(const State_c& start, const State& end) const
{
  const float diff1 = std::abs(GetTheta_c(end.theta) - start.theta);
  const float diff2 = std::abs( std::abs(GetTheta_c(end.theta) - start.theta) - M_PI );
  return std::min( diff1, diff2 );
}

float xythetaEnvironment::GetMinAngleBetween(const State_c& start, const State_c& end)
{
  const float diff1 = std::abs(end.theta - start.theta);
  const float diff2 = std::abs( std::abs(end.theta - start.theta) - M_PI );
  return std::min( diff1, diff2 );
}

SuccessorIterator xythetaEnvironment::GetSuccessors(StateID startID, Cost currG, bool reverse) const
{
  return SuccessorIterator(this, startID, currG, reverse);
}

void xythetaEnvironment::AppendToPath(xythetaPlan& plan, Path& path, int numActionsToSkip) const
{
  State curr = plan.start_;

  int actionsLeftToSkip = numActionsToSkip;

  for (size_t i = 0; i < plan.Size(); ++i) {
    const ActionID& actionID = plan.GetAction(i);
    
    if(curr.theta >= allMotionPrimitives_.size() || actionID >= allMotionPrimitives_[curr.theta].size()) {
      printf("ERROR: can't look up prim for angle %d and action id %d\n", curr.theta, actionID);
      break;
    }

    // printf("(%d) %s\n", curr.theta, actionTypes_[actionID].GetName().c_str());

    const MotionPrimitive* prim = &allMotionPrimitives_[curr.theta][actionID];

    if( actionsLeftToSkip == 0 ) {
      prim->AddSegmentsToPath(State2State_c(curr), path);
    }
    else {
      actionsLeftToSkip--;
    }

    curr.x += prim->endStateOffset.x;
    curr.y += prim->endStateOffset.y;
    curr.theta = prim->endStateOffset.theta;
  }
}

// void xythetaEnvironment::SetRobotActionParams(double halfWheelBase_mm,
//                                               double maxVelocity_mmps,
//                                               double maxReverseVelocity_mmps)
// {
//   _robotParams.halfWheelBase_mm = halfWheelBase_mm;
//   _robotParams.maxVelocity_mmps = maxVelocity_mmps;
//   _robotParams.maxReverseVelocity_mmps = maxReverseVelocity_mmps;
//   _robotParams.oneOverMaxVelocity = 1.0 / _robotParams.maxVelocity_mmps; 
// }

void xythetaEnvironment::PrintPlan(const xythetaPlan& plan) const
{
  State_c curr_c = State2State_c(plan.start_);
  StateID currID = plan.start_.GetStateID();

  PRINT_STREAM_DEBUG("xythetaEnvironment.PrintPlan", "plan start: " << plan.start_);

  for(size_t i=0; i<plan.Size(); ++i) {
    PRINT_NAMED_DEBUG("xythetaEnvironment.PrintPlan", "%2lu: (%f, %f, %f [%d]) --> %s (penalty = %f)",
           (unsigned long)i,
           curr_c.x_mm, curr_c.y_mm, curr_c.theta, currID.s.theta, 
           actionTypes_[plan.GetAction(i)].GetName().c_str(),
           plan.GetPenalty(i));
    ApplyAction(plan.GetAction(i), currID, false);
    curr_c = State2State_c(State(currID));
  }
}


State xythetaEnvironment::GetPlanFinalState(const xythetaPlan& plan) const
{
  StateID currID = plan.start_.GetStateID();

  for(size_t i = 0; i < plan.Size(); i++) {
    const ActionID& action = plan.GetAction(i);
    ApplyAction(action, currID, false);
  }

  return State(currID);
}

size_t xythetaEnvironment::FindClosestPlanSegmentToPose(const xythetaPlan& plan,
                                                        const State_c& state,
                                                        float& distanceToPlan,
                                                        bool debug) const
{

  // Ideally, we have a position which is within the discretization error of the planner. If so, use the
  // lowest index that fits within the error. We use the lowest to avoid chopping off point-turn actions.

  // Otherwise, that means we are further away, so just find the closest (x,y) point, ignoring theta since we
  // will need to drive onto the path anyway.

  using namespace std;
  if(debug) {
    cout<<"Searching for position near "<<state<<" along plan of length "<<plan.Size()<<endl;
  }

  float closestXYDist = std::numeric_limits<float>::max();
  size_t closestPointIdx = 0;

  const State discreteInputState = State_c2State(state);

  // First search *just* the action starting points. If there is an exact match here, that's the winner.
  // Also, keep track of closest distances in case there isn't an exact match
  State currPlanState = plan.start_;
  size_t planSize = plan.Size();
  for(size_t planIdx = 0; planIdx < planSize; ++planIdx) {
    if(debug) {
      cout<<planIdx<<": "<<"initial " << currPlanState;
    }

    if( currPlanState == discreteInputState ) {
      if( debug ) {
        cout << " exact: " << discreteInputState << endl;
      }
      
      distanceToPlan = 0.0f;
      return planIdx;
    }

    // TODO:(bn) no sqrt!
    const float xError = state.x_mm - GetX_mm(currPlanState.x);
    const float yError = state.y_mm - GetY_mm(currPlanState.y);
    const float initialXYDist = sqrt(pow(xError, 2) + pow(yError, 2));

    if( initialXYDist < closestXYDist ) {
      closestXYDist = initialXYDist;
      closestPointIdx = planIdx;

      if(debug) {
        cout<<"  closest\n";
      }
    }
    else if( debug ) {
      cout<<endl;
    }

    StateID currID(currPlanState);
    ApplyAction(plan.GetAction(planIdx), currID, false);
    currPlanState = State(currID);
  }
  
  // if we get here, it means we didn't find a starting point which was an exact match. Check to see if any
  // intermediate points are exact matches. If so, return the starting point before it, otherwise just return
  // the closest xy distance point
  currPlanState = plan.start_;
  for(size_t planIdx = 0; planIdx < planSize; ++planIdx) {

    // the intermediate position (x,y) coordinates are all centered at 0. Instead of shifting them over each
    // time, we just shift the state we are searching for (fewer ops). (theta doesn't matter here)
    const State_c offsetToPlanState(state.x_mm - GetX_mm(currPlanState.x),
                                    state.y_mm - GetY_mm(currPlanState.y),
                                    state.theta);
    
    const MotionPrimitive& prim(GetRawMotionPrimitive(currPlanState.theta, plan.GetAction(planIdx)));

    // skip the last intermediate position, since it overlaps with the next start
    size_t intermediatePositionsSize = prim.intermediatePositions.size();
    for( size_t intermediateIdx = 0; intermediateIdx + 1 < intermediatePositionsSize; ++intermediateIdx ) {
      const auto& intermediate = prim.intermediatePositions[ intermediateIdx ];

      if(debug) {
        cout<<planIdx<<": "<<"position "<<intermediate.position<<" --> "<<offsetToPlanState;
      }


      // check if this intermediate state is an exact match
      State intermediateStateRounded = State_c2State( intermediate.position );
      if( intermediateStateRounded == discreteInputState ) {
        if(debug) {
          cout << " exact: " << discreteInputState << endl;
        }

        distanceToPlan = 0.0f;
        return planIdx;
      }
      
      const float xyDist = GetDistanceBetween(offsetToPlanState, intermediate.position);

      if( xyDist < closestXYDist ) {
        closestXYDist = xyDist;
        closestPointIdx = planIdx;
        if(debug) {
          cout<<"  closest\n";
        }
      }
      else if( debug ) {
        cout<<endl;
      }

    }

    StateID currID(currPlanState);
    ApplyAction(plan.GetAction(planIdx), currID, false);
    currPlanState = State(currID);
  }

  distanceToPlan = closestXYDist;
  return closestPointIdx;
}

void xythetaEnvironment::ConvertToXYPlan(const xythetaPlan& plan, std::vector<State_c>& continuousPlan) const
{
  continuousPlan.clear();

  State_c curr_c = State2State_c(plan.start_);
  StateTheta currTheta = plan.start_.theta;
  // TODO:(bn) replace theta with radians? maybe just cast it here

  for(size_t i=0; i<plan.Size(); ++i) {
    printf("curr = (%f, %f, %f [%d]) : %s\n", curr_c.x_mm, curr_c.y_mm, curr_c.theta, currTheta, 
               actionTypes_[plan.GetAction(i)].GetName().c_str());

    if(currTheta >= allMotionPrimitives_.size() || plan.GetAction(i) >= allMotionPrimitives_[currTheta].size()) {
      printf("ERROR: can't look up prim for angle %d and action id %d\n", currTheta, plan.GetAction(i));
      break;
    }
    else {

      const MotionPrimitive* prim = &allMotionPrimitives_[currTheta][plan.GetAction(i)];
      for(size_t j=0; j<prim->intermediatePositions.size(); ++j) {
        float x = curr_c.x_mm + prim->intermediatePositions[j].position.x_mm;
        float y = curr_c.y_mm + prim->intermediatePositions[j].position.y_mm;
        float theta = prim->intermediatePositions[j].position.theta;

        // printf("  (%+5f, %+5f, %+5f) -> (%+5f, %+5f, %+5f)\n",
        //            prim->intermediatePositions[j].x_mm,
        //            prim->intermediatePositions[j].y_mm,
        //            prim->intermediatePositions[j].theta,
        //            x,
        //            y,
        //            theta);

        continuousPlan.push_back(State_c(x, y, theta));
      }

      if(!continuousPlan.empty())
        curr_c = continuousPlan.back();
      else
        printf("ERROR: no intermediate positiong?!\n");
      currTheta = prim->endStateOffset.theta;
    }
  }
}

}
}
