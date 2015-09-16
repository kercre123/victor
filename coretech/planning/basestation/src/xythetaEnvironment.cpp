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

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/rotatedRect.h"
#include "anki/common/shared/radians.h"
#include "anki/common/shared/utilities_shared.h"
#include "anki/planning/basestation/xythetaEnvironment.h"
#include "json/json.h"
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdio.h>

using namespace std;

namespace Anki {
namespace Planning {

#define LATTICE_PLANNER_ACCEL 200
#define LATTICE_PLANNER_DECEL 200

#define LATTICE_PLANNER_ROT_ACCEL 100
#define LATTICE_PLANNER_ROT_DECEL 100

#define REPLAN_PENALTY_BUFFER 0.5

#define HACK_USE_FIXED_SPEED 60.0

State::State(StateID sid)
  :
  x(xythetaEnvironment::GetXFromStateID(sid)),
  y(xythetaEnvironment::GetYFromStateID(sid)),
  theta(xythetaEnvironment::GetThetaFromStateID(sid))
{
}

bool State::Import(const Json::Value& config)
{
  if(!JsonTools::GetValueOptional(config, "x", x) ||
         !JsonTools::GetValueOptional(config, "y", y) ||
         !JsonTools::GetValueOptional(config, "theta", theta)) {
    printf("error: could not parse State\n");
    JsonTools::PrintJson(config, 1);
    return false;
  }
  return true;
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
  if(!JsonTools::GetValueOptional(config, "x_mm", x_mm) ||
         !JsonTools::GetValueOptional(config, "y_mm", y_mm) ||
         !JsonTools::GetValueOptional(config, "theta_rads", theta)) {
    printf("error: could not parse State_c\n");
    JsonTools::PrintJson(config, 1);
    return false;
  }
  return true;
}


SuccessorIterator::SuccessorIterator(const xythetaEnvironment* env, StateID startID, Cost startG)
  : start_c_(env->StateID2State_c(startID)),
    start_(startID),
    startG_(startG),
    nextAction_(0)
{
  assert(start_.theta == xythetaEnvironment::GetThetaFromStateID(startID));
}

// TODO:(bn) inline?
bool SuccessorIterator::Done(const xythetaEnvironment& env) const
{
  return nextAction_ > env.allMotionPrimitives_[start_.theta].size();
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

          penalty += obstaclesPerAngle_[angle][obs].second;
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
  if(plan.actions_.empty())
    return false;

  validPlan.actions_.clear();

  size_t numActions = plan.actions_.size();

  StateID curr(plan.start_.GetStateID());
  validPlan.start_ = curr;

  bool useOldPlan = true;

  // first go through all the actions that we are "skipping"
  for(size_t i=0; i<currentPathIndex; ++i) {
    // advance without checking collisions
    ApplyAction(plan.actions_[i], curr, false);
  }

  State currentRobotState(curr);
  lastSafeState = State2State_c(curr);
  validPlan.start_ = curr;

  float totalPenalty = 0.0;

  // now go through the rest of the plan, checking for collisions at each step
  for(size_t i = currentPathIndex; i < numActions; ++i) {

    // check for collisions and possibly update curr
    Cost actionPenalty = ApplyAction(plan.actions_[i], curr, true);
    assert(i < plan.penalties_.size());

    if(actionPenalty > plan.penalties_[i] + REPLAN_PENALTY_BUFFER) {
      printf("Collision along plan action %lu (starting from %d) Penalty increased from %f to %f\n",
             i,
             currentPathIndex,
             plan.penalties_[i],
             actionPenalty);
      // there was a collision trying to follow action i, so we are done
      return false;
    }

    totalPenalty += actionPenalty;

    // no collision. If we are still within
    // maxDistancetoFollowOldPlan_mm, update the valid old plan
    if(useOldPlan) {

      validPlan.Push(plan.actions_[i], plan.penalties_[i]);
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

void xythetaEnvironment::PrepareForPlanning()
{
  for(size_t angle = 0; angle < numAngles_; ++angle) {
    for( auto& obstaclePair : obstaclesPerAngle_[angle] ) {
      obstaclePair.first.SortEdgeVectors();
    }
  }
}

const MotionPrimitive& xythetaEnvironment::GetRawMotionPrimitive(StateTheta theta, ActionID action) const
{
  return allMotionPrimitives_[theta][action];
}

void SuccessorIterator::Next(const xythetaEnvironment& env)
{
  size_t numActions = env.allMotionPrimitives_[start_.theta].size();
  while(nextAction_ < numActions) {
    const MotionPrimitive* prim = &env.allMotionPrimitives_[start_.theta][nextAction_];

    // collision checking
    long endPoints = prim->intermediatePositions.size();
    bool collision = false; // fatal collision

    nextSucc_.g = 0;

    Cost penalty = 0.0f;

    for(long pointIdx = endPoints-1; pointIdx >= 0; --pointIdx) {

      StateTheta angle = prim->intermediatePositions[pointIdx].nearestTheta;
      size_t endObs = env.obstaclesPerAngle_[angle].size();

      for(size_t obsIdx=0; obsIdx<endObs && !collision; ++obsIdx) {

        if( env.obstaclesPerAngle_[angle][obsIdx].first.Contains(
              start_c_.x_mm + prim->intermediatePositions[pointIdx].position.x_mm,
              start_c_.y_mm + prim->intermediatePositions[pointIdx].position.y_mm ) ) {

          if(env.obstaclesPerAngle_[angle][obsIdx].second >= MAX_OBSTACLE_COST) {
            collision = true;
            break;
          }
          else {
            // apply soft penalty, but allow the action

            penalty += env.obstaclesPerAngle_[angle][obsIdx].second *
              prim->intermediatePositions[pointIdx].oneOverDistanceFromLastPosition;

            assert(!isinf(penalty));
            assert(!isnan(penalty));
          }
        }
      }
    }

    assert(!isinf(penalty));
    assert(!isnan(penalty));

    nextSucc_.g += penalty;

    if(!collision) {
      State result(start_);
      result.x += prim->endStateOffset.x;
      result.y += prim->endStateOffset.y;
      result.theta = prim->endStateOffset.theta;

      nextSucc_.stateID = result.GetStateID();
      nextSucc_.g += startG_ + prim->cost;

      assert(!isinf(nextSucc_.g));
      assert(!isnan(nextSucc_.g));

      nextSucc_.penalty = penalty;
      nextSucc_.actionID = nextAction_;
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
  if(actions_.empty()) {
    start_ = other.start_;
  }

  actions_.insert(actions_.end(), other.actions_.begin(), other.actions_.end());
  penalties_.insert(penalties_.end(), other.penalties_.begin(), other.penalties_.end());
  assert(actions_.size() == penalties_.size());
}

ActionType::ActionType()
  : extraCostFactor_(0.0)
  , id_(-1)
  , name_("<invalid>")
  , reverse_(false)
{
}

bool ActionType::Import(const Json::Value& config)
{
  if(!JsonTools::GetValueOptional(config, "extra_cost_factor", extraCostFactor_) ||
         !JsonTools::GetValueOptional(config, "index", id_) ||
         !JsonTools::GetValueOptional(config, "name", name_)) {
    printf("error: could not parse ActionType\n");
    JsonTools::PrintJson(config, 1);
    return false;
  }
  JsonTools::GetValueOptional(config, "reverse_action", reverse_);

  return true;
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
  // TODO:(bn) params!
  halfWheelBase_mm_ = 25.0;
  maxVelocity_mmps_ = 60.0;
  oneOverMaxVelocity_ = 1.0 / maxVelocity_mmps_;
  maxReverseVelocity_mmps_ = 25.0;
}

xythetaEnvironment::xythetaEnvironment(const char* mprimFilename, const char* mapFile)
{
  Init(mprimFilename, mapFile);
  halfWheelBase_mm_ = 25.0;
  maxVelocity_mmps_ = 60.0;
  oneOverMaxVelocity_ = 1.0 / maxVelocity_mmps_;
  maxReverseVelocity_mmps_ = 25.0;
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

bool xythetaEnvironment::Init(const char* mprimFilename, const char* mapFile)
{
  if(ReadMotionPrimitives(mprimFilename)) {
    if(mapFile != NULL) {
      FILE* fMap = fopen(mapFile, "r");
      if(!ReadEnvironment(fMap)) {
        printf("error: could not read environmenn");
        fclose(fMap);
        return false;
      }
      fclose(fMap);
    }

    numAngles_ = 1<<THETA_BITS;
    obstaclesPerAngle_.resize(numAngles_);

    radiansPerAngle_ = 2*M_PI/numAngles_;
    oneOverRadiansPerAngle_ = (float)(1.0 / ((double)radiansPerAngle_));
  }
  else {
    printf("error: could not parse motion primitives!\n");
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
  
  return ParseMotionPrims(mprimTree);
}

bool xythetaEnvironment::ParseMotionPrims(const Json::Value& config)
{
  if(!JsonTools::GetValueOptional(config, "resolution_mm", resolution_mm_) ||
         !JsonTools::GetValueOptional(config, "num_angles", numAngles_)) {
    printf("error: could not find key 'resolution_mm' or 'num_angles' in motion primitives\n");
    JsonTools::PrintJson(config, 1);
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

  for(const auto & angle : config["angle_definitions"]) {
    angles_.push_back(angle.asFloat());
  }

  if(angles_.size() != numAngles_) {
    printf("ERROR: numAngles is %u, but we read %lu angle definitions\n",
               numAngles_,
               angles_.size());
    return false;
  }

  // parse through each starting angle
  if(config["angles"].size() != numAngles_) {
    printf("error: could not find key 'angles' in motion primitives\n");
    JsonTools::PrintJson(config, 1);
    return false;
  }

  allMotionPrimitives_.resize(numAngles_);

  unsigned int numPrims = 0;

  for(unsigned int angle = 0; angle < numAngles_; ++angle) {
    Json::Value prims = config["angles"][angle]["prims"];
    for(unsigned int i = 0; i < prims.size(); ++i) {
      MotionPrimitive p;
      if(!p.Import(prims[i], angle, *this)) {
        printf("error: failed to import motion primitive\n");
        return false;
      }
      allMotionPrimitives_[angle].push_back(p);
      numPrims++;
    }
  }

  printf("adeded %d motion primitives\n", numPrims);

  return true;
}

void xythetaEnvironment::AddObstacle(const Quad2f& quad, Cost cost)
{
  // for legacy purposes, add this to every single angle!
  Poly2f poly;
  poly.ImportQuad2d(quad);
  FastPolygon fastPoly(poly);

  for(size_t i=0; i<numAngles_; ++i) {
    obstaclesPerAngle_[i].push_back( std::make_pair( fastPoly, cost ) );
  }
}
  
void xythetaEnvironment::AddObstacle(const RotatedRectangle& rect, Cost cost)
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

FastPolygon xythetaEnvironment::ExpandCSpace(const Poly2f& obstacle,
                                             const Poly2f& robot)
{
  // TODO:(bn) don't copy polygon into vector

  assert(obstacle.size() > 0);
  assert(robot.size() > 0);

  // CoreTechPrint("obstacle: ");
  // obstacle.Print();

  // CoreTechPrint("robot: ");
  // robot.Print();

  // This algorithm is a Minkowski difference. It requires both
  // obstacle and robot to be convex complete polygons in clockwise
  // order. You can also see a python implementation with plotting and
  // such in coretech/planning/matlab/minkowski.py


  float startingAngle = obstacle.GetEdgeAngle(0);

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
  Poly2f expansion;

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

  return FastPolygon{ expansion };
}


const FastPolygon& xythetaEnvironment::AddObstacleWithExpansion(const Poly2f& obstacle,
                                                                const Poly2f& robot,
                                                                StateTheta theta,
                                                                Cost cost)
{
  if(theta >= obstaclesPerAngle_.size()) {
    CoreTechPrint("ERROR: theta = %d, but only have %lu obstacle angles and %u angles total\n",
                  theta,
                  obstaclesPerAngle_.size(),
                  numAngles_);
    
    theta = 0;
  }

  obstaclesPerAngle_[theta].emplace_back(std::make_pair(ExpandCSpace(obstacle, robot), cost));

  return obstaclesPerAngle_[theta].back().first;
}


bool MotionPrimitive::Import(const Json::Value& config, StateTheta startingAngle, const xythetaEnvironment& env)
{
  startTheta = startingAngle;

  if(!JsonTools::GetValueOptional(config, "action_index", id)) {
    printf("error: missing key 'action_index'\n");
    JsonTools::PrintJson(config, 1);
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
      cost += std::abs(deltaTheta.ToFloat()) * env.GetHalfWheelBase_mm() * env.GetOneOverMaxVelocity();

      penalty = 1.0 / cost;
    }

    intermediatePositions.emplace_back(s, env.GetTheta(s.theta),  penalty);
  }

  if(config.isMember("extra_cost_factor")) {
    printf("ERROR: individual primitives shouldn't have cost factors. Old file format?\n");
    return false;
  }

  double linearSpeed = env.GetMaxVelocity_mmps();
  double oneOverLinearSpeed = env.GetOneOverMaxVelocity();
  bool isReverse = env.GetActionType(id).IsReverseAction();
  if(isReverse) {
    linearSpeed = env.GetMaxReverseVelocity_mmps();
    oneOverLinearSpeed = 1.0 / env.GetMaxReverseVelocity_mmps();
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
      pathSegments_.AppendLine(0,
                               0.0,
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
    double radius_mm = turningRadius + env.GetHalfWheelBase_mm();

    // the total time is the arclength of the outer wheel arc divided by the max outer wheel speed
    Cost arcTime = deltaTheta * radius_mm * oneOverLinearSpeed;
    cost += arcTime;

    Cost arcSpeed = deltaTheta * turningRadius / arcTime;

    // TODO:(bn) these don't work properly backwards at the moment

#ifdef HACK_USE_FIXED_SPEED
    arcSpeed = HACK_USE_FIXED_SPEED;
#endif

    pathSegments_.AppendArc(0,
                            config["arc"]["centerPt_x_mm"].asFloat(),
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

    Cost turnTime = std::abs(deltaTheta) * env.GetHalfWheelBase_mm() * oneOverLinearSpeed;
    cost += turnTime;

    float rotSpeed = deltaTheta / turnTime;

    pathSegments_.AppendPointTurn(0,
                                  0.0,
                                  0.0,
                                  env.GetTheta_c(endStateOffset.theta),
                                  rotSpeed,
                                  LATTICE_PLANNER_ROT_ACCEL,
                                  LATTICE_PLANNER_ROT_DECEL,
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

  return true;
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

bool xythetaEnvironment::ReadEnvironment(FILE* fEnv)
{
  //float x0,y0,x1,y1,len;

  assert(false);

  // TEMP: fix this with polygons

  // while(fscanf(fEnv, "%f %f %f %f %f", &x0, &y0, &x1, &y1, &len) == 5) {
  //   obstacles_.emplace_back(std::make_pair(RotatedRectangle{x0, y0, x1, y1, len}, FATAL_OBSTACLE_COST));
  // }

  return true;
}

void xythetaEnvironment::WriteEnvironment(const char *filename) const
{
  // ofstream outfile(filename);
  // for(const auto& it : obstacles_) {
  //   it.first.Dump(outfile);
  // }
  assert(false);
  // TEMP: fix this with polygons
}

SuccessorIterator xythetaEnvironment::GetSuccessors(StateID startID, Cost currG) const
{
  return SuccessorIterator(this, startID, currG);
}

void xythetaEnvironment::AppendToPath(xythetaPlan& plan, Path& path) const
{
  State curr = plan.start_;

  for(const auto& actionID : plan.actions_) {
    if(curr.theta >= allMotionPrimitives_.size() || actionID >= allMotionPrimitives_[curr.theta].size()) {
      printf("ERROR: can't look up prim for angle %d and action id %d\n", curr.theta, actionID);
      break;
    }

    // printf("(%d) %s\n", curr.theta, actionTypes_[actionID].GetName().c_str());

    const MotionPrimitive* prim = &allMotionPrimitives_[curr.theta][actionID];
    /*u8 pathSegmentOffset =*/ prim->AddSegmentsToPath(State2State_c(curr), path);

    curr.x += prim->endStateOffset.x;
    curr.y += prim->endStateOffset.y;
    curr.theta = prim->endStateOffset.theta;
  }
}

void xythetaEnvironment::PrintPlan(const xythetaPlan& plan) const
{
  State_c curr_c = State2State_c(plan.start_);
  StateID currID = plan.start_.GetStateID();

  cout<<"plan start: "<<plan.start_<<endl;

  for(size_t i=0; i<plan.actions_.size(); ++i) {
    printf("%2lu: (%f, %f, %f [%d]) --> %s (penalty = %f)\n",
           i,
           curr_c.x_mm, curr_c.y_mm, curr_c.theta, currID.s.theta, 
           actionTypes_[plan.actions_[i]].GetName().c_str(),
           plan.penalties_[i]);
    ApplyAction(plan.actions_[i], currID, false);
    curr_c = State2State_c(State(currID));
  }
}


State xythetaEnvironment::GetPlanFinalState(const xythetaPlan& plan) const
{
  StateID currID = plan.start_.GetStateID();

  for(const auto& action : plan.actions_) {
    ApplyAction(action, currID, false);
  }

  return State(currID);
}

size_t xythetaEnvironment::FindClosestPlanSegmentToPose(const xythetaPlan& plan,
                                                        const State_c& state,
                                                        float& distanceToPlan,
                                                        bool debug) const
{
  // for now, this is implemented by simply going over every
  // intermediate pose and finding the closest one
  float closest = 999999.9;  // TODO:(bn) talk to people about this
  size_t startPoint = 0;

  State curr = plan.start_;

  using namespace std;
  if(debug)
    cout<<"Searching for position near "<<state<<" along plan of length "<<plan.Size()<<endl;

  size_t planSize = plan.Size();
  for(size_t planIdx = 0; planIdx < planSize; ++planIdx) {
    const MotionPrimitive& prim(GetRawMotionPrimitive(curr.theta, plan.actions_[planIdx]));

    // the intermediate position (x,y) coordinates are all centered at
    // 0. Instead of shifting them over each time, we just shift the
    // state we are searching for (fewer ops)
    State_c target(state.x_mm - GetX_mm(curr.x),
                   state.y_mm - GetY_mm(curr.y),
                   0.0f);

    // first check the exact state.  // TODO:(bn) no sqrt!
    float initialDist = sqrt(pow(target.x_mm, 2) + pow(target.y_mm, 2));
    if(debug)
      cout<<planIdx<<": "<<"iniitial "<<target<<" = "<<initialDist;
    if(initialDist <= closest + 1e-6) {
      closest = initialDist;
      startPoint = planIdx;
      if(debug)
        cout<<"  closest\n";
    }
    else {
      if(debug)
        cout<<endl;
    }

    for(const auto& intermediate : prim.intermediatePositions) {
      // TODO:(bn) get squared distance
      float dist = GetDistanceBetween(target, intermediate.position);

      if(debug)
        cout<<planIdx<<": "<<"position "<<intermediate.position<<" --> "<<target<<" = "<<dist;

      if(dist < closest) {
        closest = dist;
        startPoint = planIdx;
        if(debug)
          cout<<"  closest\n";
      }
      else {
        if(debug)
          cout<<endl;
      }
    }

    StateID currID(curr);
    ApplyAction(plan.actions_[planIdx], currID, false);
    curr = State(currID);
  }

  distanceToPlan = closest;

  return startPoint;
}

void xythetaEnvironment::ConvertToXYPlan(const xythetaPlan& plan, std::vector<State_c>& continuousPlan) const
{
  continuousPlan.clear();

  State_c curr_c = State2State_c(plan.start_);
  StateTheta currTheta = plan.start_.theta;
  // TODO:(bn) replace theta with radians? maybe just cast it here

  for(size_t i=0; i<plan.actions_.size(); ++i) {
    printf("curr = (%f, %f, %f [%d]) : %s\n", curr_c.x_mm, curr_c.y_mm, curr_c.theta, currTheta, 
               actionTypes_[plan.actions_[i]].GetName().c_str());

    if(currTheta >= allMotionPrimitives_.size() || plan.actions_[i] >= allMotionPrimitives_[currTheta].size()) {
      printf("ERROR: can't look up prim for angle %d and action id %d\n", currTheta, plan.actions_[i]);
      break;
    }
    else {

      const MotionPrimitive* prim = &allMotionPrimitives_[currTheta][plan.actions_[i]];
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
