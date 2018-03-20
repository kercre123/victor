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

Cost xythetaEnvironment::ApplyAction(const ActionID& action, StateID& stateID, bool checkCollisions) const
{
  GraphState curr(stateID);
  Point2f start = curr.GetPointXY_mm();

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
  }

  GraphState result(curr);
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
    ApplyAction(plan.GetAction(i), curr, false);
  }

  GraphState currentRobotState(curr);
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
      lastSafeState = State2State_c(GraphState(curr));

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

const MotionPrimitive& xythetaEnvironment::GetRawMotionPrimitive(GraphTheta theta, ActionID action) const
{
  return allMotionPrimitives_[theta][action];
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

void xythetaPlan::Append(const xythetaPlan& other)
{
  if(_actionCostPairs.empty()) {
    start_ = other.start_;
  }
  _actionCostPairs.insert(_actionCostPairs.end(), other._actionCostPairs.begin(), other._actionCostPairs.end() );
}

xythetaEnvironment::~xythetaEnvironment()
{
}

xythetaEnvironment::xythetaEnvironment()
{
  obstaclesPerAngle_.resize(numAngles_);
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
    obstaclesPerAngle_.resize(numAngles_);
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
    float configResolution = 0.f;
    unsigned int configAngles = 0;
    if(!JsonTools::GetValueOptional(config, "resolution_mm", configResolution) ||
       !JsonTools::GetValueOptional(config, "num_angles", configAngles)) {
      printf("error: could not find key 'resolution_mm' or 'num_angles' in motion primitives\n");
      JsonTools::PrintJsonCout(config, 1);
      return false;
    }

    assert(GraphState::resolution_mm_ == configResolution);
    assert(GraphState::numAngles_ == configAngles);

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
  writer.AddEntry("resolution_mm", GraphState::resolution_mm_);
  writer.AddEntry("num_angles", (int)GraphState::numAngles_);

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

    for (auto obstacles : obstaclesPerAngle_) {
      obstacles.clear();
    }

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
                                                                GraphTheta theta,
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

float xythetaEnvironment::GetDistanceBetween(const State_c& start, const GraphState& end) const
{
  return (end.GetPointXY_mm() - start.GetPointXY_mm()).Length();
}

float xythetaEnvironment::GetDistanceBetween(const State_c& start, const State_c& end)
{
  float distSq = 
    pow(end.x_mm - start.x_mm, 2)
    + pow(end.y_mm - start.y_mm, 2);

  return sqrtf(distSq);
}

float xythetaEnvironment::GetMinAngleBetween(const State_c& start, const GraphState& end) const
{
  const float diff1 = std::abs(LookupTheta(end.theta) - start.theta);
  const float diff2 = std::abs( std::abs(LookupTheta(end.theta) - start.theta) - M_PI );
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
  GraphState curr = plan.start_;

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
    curr_c = State2State_c(GraphState(currID));
  }
}


GraphState xythetaEnvironment::GetPlanFinalState(const xythetaPlan& plan) const
{
  StateID currID = plan.start_.GetStateID();

  for(size_t i = 0; i < plan.Size(); i++) {
    const ActionID& action = plan.GetAction(i);
    ApplyAction(action, currID, false);
  }

  return GraphState(currID);
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
    ApplyAction(plan.GetAction(planIdx), currID, false);
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
    
    const MotionPrimitive& prim(GetRawMotionPrimitive(currPlanState.theta, plan.GetAction(planIdx)));

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
    currPlanState = GraphState(currID);
  }

  distanceToPlan = closestXYDist;
  return closestPointIdx;
}

void xythetaEnvironment::ConvertToXYPlan(const xythetaPlan& plan, std::vector<State_c>& continuousPlan) const
{
  continuousPlan.clear();

  State_c curr_c = State2State_c(plan.start_);
  GraphTheta currTheta = plan.start_.theta;
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
