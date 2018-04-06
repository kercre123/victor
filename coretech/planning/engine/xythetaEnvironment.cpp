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


#define REPLAN_PENALTY_BUFFER 0.5

// #define HACK_USE_FIXED_SPEED 60.0

SuccessorIterator::SuccessorIterator(const xythetaEnvironment* env, StateID startID, Cost startG, bool reverse)
  : start_c_(env->State2State_c(startID))
  , start_(startID)
  , startG_(startG)
  , nextAction_(0)
  , reverse_(reverse)
{
  // verify unpacking worked
  assert(start_.theta == startID.s.theta);
}

// TODO:(bn) inline?
bool SuccessorIterator::Done(const xythetaEnvironment& env) const
{
  return nextAction_ > env.allActions_.GetNumActions();
}

void SuccessorIterator::Next(const xythetaEnvironment& env)
{
  size_t numActions = env.allActions_.GetNumActions();

  while(nextAction_ < numActions) {
    const MotionPrimitive* prim = (reverse_) ? &env.allActions_.GetReverseMotion((size_t)start_.theta, nextAction_)
                                             : &env.allActions_.GetForwardMotion((size_t)start_.theta, nextAction_);


    // collision checking
    long endPoints = prim->intermediatePositions.size();
    bool collision = false; // fatal collision
    bool reverseMotion = env.GetActionSpace().GetActionType(prim->id).IsReverseAction();


    nextSucc_.g = 0;

    Cost penalty = 0.0f;

    // first, check if we are well-clear of everything, and can skip this check
    bool possibleObstacle = false;

    State_c primtiveOffset = start_c_;

    GraphState result(start_);
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

          GraphTheta angle = prim->intermediatePositions[pointIdx].nearestTheta;
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

Cost xythetaEnvironment::CheckAndApplyAction(const ActionID& action, StateID& stateID) const
{
  GraphState curr(stateID);
  Point2f start = curr.GetPointXY_mm();

  assert(curr.theta >= 0);
  assert(curr.theta < GraphState::numAngles_);

  if(action >= allActions_.GetNumActions())
    return FATAL_OBSTACLE_COST;

  const MotionPrimitive* prim = & allActions_.GetForwardMotion(curr.theta, action);

  Cost penalty = 0.0;


  size_t endPoints = prim->intermediatePositions.size();

  for(size_t point=0; point < endPoints; ++point) {
    GraphTheta angle = prim->intermediatePositions[point].nearestTheta;
    size_t endObs = obstaclesPerAngle_[angle].size();

    for(size_t obs = 0; obs < endObs; ++obs) {
      const Point2f intermediatePoint = prim->intermediatePositions[point].position.GetPointXY_mm();
      if( obstaclesPerAngle_[angle][obs].first.Contains(start + intermediatePoint) ) {

        penalty += obstaclesPerAngle_[angle][obs].second *
          prim->intermediatePositions[point].oneOverDistanceFromLastPosition;
        if( penalty >= MAX_OBSTACLE_COST) {
          return penalty;
        }
      }
    }
  }

  GraphState result(curr);
  result.x += prim->endStateOffset.x;
  result.y += prim->endStateOffset.y;
  result.theta = prim->endStateOffset.theta;

  stateID = result.GetStateID();
  return penalty;
}

Cost xythetaEnvironment::CheckAndApplyPathSegment(const PathSegment& pathSegment,
                                          State_c& state_c,
                                          float maxPenalty) const
{
  Cost penalty = 0.0f;
  
  float endX, endY, endAngle;
  pathSegment.GetEndPose(endX, endY, endAngle);
  State_c endState_c = State_c(endX, endY, endAngle);
  
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

  GraphTheta intermediateTheta = intermediate_c.GetGraphTheta();
  for(size_t i=1; i < numIntermediatePoints; ++i) {
    if( segmentType == PST_POINT_TURN ) {
      intermediate_c.theta = start_theta + dtheta*i;
      intermediateTheta = intermediate_c.GetGraphTheta();
    } else if( segmentType == PST_LINE ) {
      intermediate_c.x_mm = start_x + dx*i;
      intermediate_c.y_mm = start_y + dy*i;
    } else if( segmentType == PST_ARC ) {
      intermediate_c.x_mm = segmentDef.arc.centerPt_x + segmentDef.arc.radius * cosf(segmentDef.arc.startRad + dtheta*i);
      intermediate_c.y_mm = segmentDef.arc.centerPt_y + segmentDef.arc.radius * sinf(segmentDef.arc.startRad + dtheta*i);
      intermediate_c.theta = start_theta + dtheta*i;
      intermediateTheta = intermediate_c.GetGraphTheta();
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
    allActions_.ApplyAction(plan.GetAction(i), curr);
  }

  GraphState currentRobotState(curr);
  lastSafeState = State2State_c(curr);
  validPlan.start_ = curr;

  float totalPenalty = 0.0;

  // now go through the rest of the plan, checking for collisions at each step
  for(size_t i = currentPathIndex; i < numActions; ++i) {

    // check for collisions and possibly update curr
    Cost actionPenalty = CheckAndApplyAction(plan.GetAction(i), curr);
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
      lastSafeState = State2State_c(GraphState(curr));

      // TODO:(bn) this is kind of wrong. It's using euclidean
      // distance, instead we should add up the lengths of each
      // action. That will be faster and better
      if(State_c::GetDistanceBetween(lastSafeState, currentRobotState) > maxDistancetoFollowOldPlan_mm) {
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
    Cost penalty = CheckAndApplyPathSegment(currSegment, currState_c, maxPenalty);
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

  for(size_t angle = 0; angle < GraphState::numAngles_; ++angle) {
    for( auto& obstaclePair : obstaclesPerAngle_[angle] ) {
      if( angle == 0 ) {
        numObstacles++;
      }
      obstaclePair.first.SortEdgeVectors();
    }
  }

  // fill in the obstacle bounds, default values are set
  obstacleBounds_.resize(numObstacles);
  
  for(size_t angle = 0; angle < GraphState::numAngles_; ++angle) {
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

bool xythetaEnvironment::IsInCollision(GraphState s) const
{
  return IsInCollision(State2State_c(s));
}

bool xythetaEnvironment::IsInCollision(State_c c) const
{  
  GraphTheta angle = c.GetGraphTheta();
  size_t endObs = obstaclesPerAngle_[angle].size();

  for(size_t obsIdx=0; obsIdx<endObs; ++obsIdx) {
    if(obstaclesPerAngle_[angle][obsIdx].first.Contains( c.x_mm, c.y_mm ) &&
       obstaclesPerAngle_[angle][obsIdx].second >= MAX_OBSTACLE_COST)
      return true;
  }
  return false;
}

bool xythetaEnvironment::IsInSoftCollision(GraphState s) const
{
  GraphTheta angle = s.theta;
  size_t endObs = obstaclesPerAngle_[angle].size();

  for(size_t obsIdx=0; obsIdx<endObs; ++obsIdx) {
    if(obstaclesPerAngle_[angle][obsIdx].first.Contains( s.GetPointXY_mm() ))
      return true;
  }
  return false;
}

Cost xythetaEnvironment::GetCollisionPenalty(GraphState s) const
{
  GraphTheta angle = s.theta;
  size_t endObs = obstaclesPerAngle_[angle].size();

  for(size_t obsIdx=0; obsIdx<endObs; ++obsIdx) {
    if(obstaclesPerAngle_[angle][obsIdx].first.Contains( s.GetPointXY_mm() ))
      return obstaclesPerAngle_[angle][obsIdx].second;
  }
  return 0.0;
}

xythetaEnvironment::~xythetaEnvironment()
{
}

xythetaEnvironment::xythetaEnvironment()
{
  obstaclesPerAngle_.resize(GraphState::numAngles_);
}

bool xythetaEnvironment::Init(const Json::Value& mprimJson)
{
  ClearObstacles();
  return allActions_.ParseMotionPrims(mprimJson);
}

size_t xythetaEnvironment::GetNumObstacles() const
{
  return obstaclesPerAngle_[0].size();
}

bool xythetaEnvironment::Init(const char* mprimFilename)
{
  if(ReadMotionPrimitives(mprimFilename)) {
    obstaclesPerAngle_.resize(GraphState::numAngles_);
  } else {
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

  return allActions_.ParseMotionPrims(mprimTree, false);
}

bool xythetaEnvironment::ParseObstacles(const Json::Value& config)
{
  try {
    if( GraphState::numAngles_ == 0 || config["angles"].isNull() ) {
      PRINT_NAMED_ERROR("xythetaEnvironment.ParseObstacles.InvalidObjectAngles",
                        "numAngles_ = %d",
                        GraphState::numAngles_);
      return false;
    }

    if( GraphState::numAngles_ != config["angles"].size() ) {
      PRINT_NAMED_ERROR("xythetaEnvironment.ParseObstacles.AnglesMismatch",
                        "this has %d angles, but json has %d",
                        GraphState::numAngles_,
                        config.size());
      return false;
    }

    for (auto obstacles : obstaclesPerAngle_) {
      obstacles.clear();
    }

    for( int theta = 0; theta < GraphState::numAngles_; ++theta ) {

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

  for(size_t i=0; i<GraphState::numAngles_; ++i) {
    obstaclesPerAngle_[i].push_back( std::make_pair( fastPoly, cost ) );
  }
}
  
void xythetaEnvironment::AddObstacleAllThetas(const RotatedRectangle& rect, Cost cost)
{
  Poly2f poly(rect);
  FastPolygon fastPoly(poly);

  for(size_t i=0; i<GraphState::numAngles_; ++i) {
    obstaclesPerAngle_[i].push_back( std::make_pair( fastPoly, cost ) );
  }
}

void xythetaEnvironment::ClearObstacles()
{
  for(size_t i=0; i<GraphState::numAngles_; ++i) {
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
                                                                GraphTheta theta,
                                                                Cost cost)
{
  if(theta >= obstaclesPerAngle_.size()) {
    CoreTechPrint("ERROR: theta = %d, but only have %zu obstacle angles and %u angles total\n",
                  theta,
                  obstaclesPerAngle_.size(),
                  GraphState::numAngles_);
    
    theta = 0;
  }

  obstaclesPerAngle_[theta].emplace_back(std::make_pair(ExpandCSpace(obstacle, robot), cost));

  return obstaclesPerAngle_[theta].back().first;
}

bool xythetaEnvironment::RoundSafe(const State_c& c, GraphState& rounded) const
{
  float bestDist2 = 999999.9;

  // TODO:(bn) smarter rounding for theta?
  rounded.theta = c.GetGraphTheta();

  GraphXY startX = (GraphXY) floor(c.x_mm * GraphState::oneOverResolution_);
  GraphXY endX   = (GraphXY)  ceil(c.x_mm * GraphState::oneOverResolution_);
  GraphXY startY = (GraphXY) floor(c.y_mm * GraphState::oneOverResolution_);
  GraphXY endY   = (GraphXY)  ceil(c.y_mm * GraphState::oneOverResolution_);

  for(GraphXY x = startX; x <= endX; ++x) {
    for(GraphXY y = startY; y <= endY; ++y) {
      GraphState candidate(x, y, rounded.theta);
      if(!IsInCollision(candidate)) {
        float dist2 = (candidate.GetPointXY_mm() - c.GetPointXY_mm()).LengthSq();
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

SuccessorIterator xythetaEnvironment::GetSuccessors(StateID startID, Cost currG, bool reverse) const
{
  return SuccessorIterator(this, startID, currG, reverse);
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

  const GraphState discreteInputState(state);

  // First search *just* the action starting points. If there is an exact match here, that's the winner.
  // Also, keep track of closest distances in case there isn't an exact match
  GraphState currPlanState = plan.start_;
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
    const float initialXYDist = (state.GetPointXY_mm() - currPlanState.GetPointXY_mm()).Length();

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
    allActions_.ApplyAction(plan.GetAction(planIdx), currID);
    currPlanState = GraphState(currID);
  }
  
  // if we get here, it means we didn't find a starting point which was an exact match. Check to see if any
  // intermediate points are exact matches. If so, return the starting point before it, otherwise just return
  // the closest xy distance point
  currPlanState = plan.start_;
  for(size_t planIdx = 0; planIdx < planSize; ++planIdx) {

    // the intermediate position (x,y) coordinates are all centered at 0. Instead of shifting them over each
    // time, we just shift the state we are searching for (fewer ops). (theta doesn't matter here)
    const Point2f offset = state.GetPointXY_mm() - currPlanState.GetPointXY_mm();
    const State_c offsetToPlanState(offset.x(), offset.y(), state.theta);
    
    const MotionPrimitive& prim = allActions_.GetForwardMotion(currPlanState.theta, plan.GetAction(planIdx));

    // skip the last intermediate position, since it overlaps with the next start
    size_t intermediatePositionsSize = prim.intermediatePositions.size();
    for( size_t intermediateIdx = 0; intermediateIdx + 1 < intermediatePositionsSize; ++intermediateIdx ) {
      const auto& intermediate = prim.intermediatePositions[ intermediateIdx ];

      if(debug) {
        cout<<planIdx<<": "<<"position "<<intermediate.position<<" --> "<<offsetToPlanState;
      }


      // check if this intermediate state is an exact match
      GraphState intermediateStateRounded( intermediate.position );
      if( intermediateStateRounded == discreteInputState ) {
        if(debug) {
          cout << " exact: " << discreteInputState << endl;
        }

        distanceToPlan = 0.0f;
        return planIdx;
      }
      
      const float xyDist = State_c::GetDistanceBetween(offsetToPlanState, intermediate.position);

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
    allActions_.ApplyAction(plan.GetAction(planIdx), currID);
    currPlanState = GraphState(currID);
  }

  distanceToPlan = closestXYDist;
  return closestPointIdx;
}

}
}
