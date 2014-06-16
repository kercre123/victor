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
#include "anki/common/basestation/math/rotatedRect.h"
#include "anki/common/shared/radians.h"
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

State::State(StateID sid)
  :
  x(xythetaEnvironment::GetXFromStateID(sid)),
  y(xythetaEnvironment::GetYFromStateID(sid)),
  theta(xythetaEnvironment::GetThetaFromStateID(sid))
{
}

bool operator==(const StateID& lhs, const StateID& rhs)
{
  // TODO:(bn) efficient comparison of bit fields?
  return lhs.theta == rhs.theta && lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator!=(const StateID& lhs, const StateID& rhs)
{
  // TODO:(bn) efficient comparison of bit fields?
  return lhs.theta != rhs.theta || lhs.x != rhs.x || lhs.y != rhs.y;
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
  sid.x = x;
  sid.y = y;
  sid.theta = theta;
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

bool StateID::operator<(const StateID& rhs) const
{
  // TODO:(bn) use union?
  if(x < rhs.x)
    return true;
  if(x > rhs.x)
    return false;

  if(y < rhs.y)
    return true;
  if(y > rhs.y)
    return false;

  if(theta < rhs.theta)
    return true;
  // if(theta > rhs.theta)
  return false;
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
    nextAction_(0),
    motionPrimitives_(env->allMotionPrimitives_[xythetaEnvironment::GetThetaFromStateID(startID)]),
    obstacles_(env->obstacles_)
{
}

// TODO:(bn) inline?
bool SuccessorIterator::Done() const
{
  return nextAction_ > motionPrimitives_.size();
}

bool xythetaEnvironment::ApplyAction(const ActionID& action, StateID& stateID, bool checkCollisions) const
{
  State curr(stateID);
  float start_x = GetX_mm(curr.x);
  float start_y = GetY_mm(curr.y);

  assert(curr.theta >= 0);
  assert(curr.theta < allMotionPrimitives_.size());

  if(action >= allMotionPrimitives_[curr.theta].size())
    return false;

  const MotionPrimitive* prim = & allMotionPrimitives_[curr.theta][action];

  if(checkCollisions) {
    size_t endObs = obstacles_.size();
    size_t endPoints = prim->intermediatePositions.size();
    for(size_t point=0; point < endPoints; ++point) {
      for(size_t obs=0; obs < endObs; ++obs) {
        float x = start_x + prim->intermediatePositions[point].x_mm;
        float y = start_y + prim->intermediatePositions[point].y_mm;
        if(obstacles_[obs].Contains(x, y)) {
          return false;
        }
      }
    }
  }

  State result(curr);
  result.x += prim->endStateOffset.x;
  result.y += prim->endStateOffset.y;
  result.theta = prim->endStateOffset.theta;

  stateID = result.GetStateID();
  return true;
}

const MotionPrimitive& xythetaEnvironment::GetRawMotionPrimitive(StateTheta theta, ActionID action) const
{
  return allMotionPrimitives_[theta][action];
}

void SuccessorIterator::Next()
{
  while(nextAction_ < motionPrimitives_.size()) {
    const MotionPrimitive* prim = &motionPrimitives_[nextAction_];

    // collision checking
    size_t endObs = obstacles_.size();
    long endPoints = prim->intermediatePositions.size();
    bool collision = false;

    // iterate first through action, starting at the end because this
    // is more likely to be a collision
    for(long pointIdx = endPoints-1; pointIdx >= 0 && !collision; --pointIdx) {
      for(size_t obsIdx=0; obsIdx<endObs; ++obsIdx) {
        float x,y;
        x = start_c_.x_mm + prim->intermediatePositions[pointIdx].x_mm;
        y = start_c_.y_mm + prim->intermediatePositions[pointIdx].y_mm;
        if(obstacles_[obsIdx].Contains(x, y)) {
          collision = true;
          break;
        }
      }
    }

    if(!collision) {
      State result(start_);
      result.x += prim->endStateOffset.x;
      result.y += prim->endStateOffset.y;
      result.theta = prim->endStateOffset.theta;

      nextSucc_.stateID = result.GetStateID();
      nextSucc_.g = startG_ + prim->cost;
      nextSucc_.actionID = nextAction_;
      break;
    }

    nextAction_++;
  }

  nextAction_++;
}

bool xythetaEnvironment::IsInCollision(State s) const
{
  size_t endObs = obstacles_.size();
  for(size_t obsIdx=0; obsIdx<endObs; ++obsIdx) {
    if(obstacles_[obsIdx].Contains(GetX_mm(s.x), GetX_mm(s.y)))
      return true;
  }
  return false;
}

bool xythetaEnvironment::IsInCollision(State_c c) const
{
  size_t endObs = obstacles_.size();
  for(size_t obsIdx=0; obsIdx<endObs; ++obsIdx) {
    if(obstacles_[obsIdx].Contains(c.x_mm, c.y_mm))
      return true;
  }
  return false;
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
  //for(size_t i=0; i<obstacles_.size(); ++i)
  //  delete obstacles_[i];
}


xythetaEnvironment::xythetaEnvironment()
{
  numAngles_ = 1<<THETA_BITS;
  radiansPerAngle_ = 2*M_PI/numAngles_;
  oneOverRadiansPerAngle_ = (float)(1.0 / ((double)radiansPerAngle_));
  // TODO:(bn) params!
  halfWheelBase_mm_ = 25.0;
  maxVelocity_mmps_ = 50.0;
  oneOverMaxVelocity_ = 1.0 / maxVelocity_mmps_;
  maxReverseVelocity_mmps_ = 25.0;
}

xythetaEnvironment::xythetaEnvironment(const char* mprimFilename, const char* mapFile)
{
  Init(mprimFilename, mapFile);
  halfWheelBase_mm_ = 25.0;
  maxVelocity_mmps_ = 50.0;
  oneOverMaxVelocity_ = 1.0 / maxVelocity_mmps_;
  maxReverseVelocity_mmps_ = 25.0;
}


bool xythetaEnvironment::Init(const Json::Value& mprimJson)
{
  ClearObstacles();
  return ParseMotionPrims(mprimJson);
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

void xythetaEnvironment::AddObstacle(const Quad2f& quad)
{
  obstacles_.emplace_back(quad);
}
  
void xythetaEnvironment::AddObstacle(const RotatedRectangle& rect)
{
  obstacles_.emplace_back(rect);
}

void xythetaEnvironment::ClearObstacles()
{
//  for(size_t i=0; i<obstacles_.size(); ++i)
//    delete obstacles_[i];
  obstacles_.clear();
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
    if(!intermediatePositions.empty())
      old = intermediatePositions.back();
    
    intermediatePositions.push_back(s);
  }

  if(config.isMember("extra_cost_factor")) {
    printf("ERROR: individual primitives shouldn't have cost factors. Old file format?\n");
    return false;
  }

  double linearSpeed = env.GetMaxVelocity_mmps();
  double oneOverLinearSpeed = env.GetOneOverMaxVelocity();
  if(env.GetActionType(id).IsReverseAction()) {
    linearSpeed = env.GetMaxReverseVelocity_mmps();
    oneOverLinearSpeed = 1.0 / env.GetMaxReverseVelocity_mmps();
  }

  // Compute cost based on the action. Cost is time in seconds
  cost = 0.0;

  double length = std::abs(config["straight_length_mm"].asDouble());
  if(length > 0.0) {
    cost += length * oneOverLinearSpeed;

    float signedLength = config["straight_length_mm"].asFloat();

    PathSegment seg;
    seg.DefineLine(0.0,
                       0.0,
                       signedLength * cos(env.GetTheta_c(startingAngle)),
                       signedLength * sin(env.GetTheta_c(startingAngle)),
                       linearSpeed,
                       LATTICE_PLANNER_ACCEL,
                       LATTICE_PLANNER_DECEL);

    pathSegments_.push_back(seg);
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

    PathSegment seg;
    seg.DefineArc(config["arc"]["centerPt_x_mm"].asFloat(),
                      config["arc"]["centerPt_y_mm"].asFloat(),
                      config["arc"]["radius_mm"].asFloat(),
                      config["arc"]["startRad"].asFloat(),
                      config["arc"]["sweepRad"].asFloat(),
                      arcSpeed,
                      LATTICE_PLANNER_ACCEL,
                      LATTICE_PLANNER_DECEL);
    pathSegments_.push_back(seg);
  }
  else if(config.isMember("turn_in_place_direction")) {
    double direction = config["turn_in_place_direction"].asDouble();

    // turn in place is just like an arc with radius 0
    Radians startRads(env.GetTheta_c(startTheta));
    double deltaTheta = startRads.angularDistance(env.GetTheta_c(endStateOffset.theta), direction < 0);

    Cost turnTime = std::abs(deltaTheta) * env.GetHalfWheelBase_mm() * oneOverLinearSpeed;
    cost += turnTime;

    float rotSpeed = deltaTheta / turnTime;

    PathSegment seg;
    seg.DefinePointTurn(0.0,
                            0.0,
                            env.GetTheta_c(endStateOffset.theta),
                            rotSpeed,
                            LATTICE_PLANNER_ROT_ACCEL,
                            LATTICE_PLANNER_ROT_DECEL);
    pathSegments_.push_back(seg);
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

  for(const auto& seg : pathSegments_) {
    PathSegment segment(seg);
    segment.OffsetStart(curr.x_mm, curr.y_mm);

    float xx, yy;
    segment.GetStartPoint(xx,yy);
    // printf("start: (%f, %f)\n", xx, yy);

    // if this segment can be combined with the previous one, do
    // that. otherwise, append a new segment.
    bool shouldAdd = true;
    if(path.GetNumSegments() > 0 && path[path.GetNumSegments()-1].GetType() == segment.GetType()) {
      size_t endIdx = path.GetNumSegments()-1;

      switch(segment.GetType()) {

      case PST_LINE:
        path[endIdx].GetDef().line.endPt_x = segment.GetDef().line.endPt_x;
        path[endIdx].GetDef().line.endPt_y = segment.GetDef().line.endPt_y;
        shouldAdd = false;
        break;

      case PST_ARC:
        if(FLT_NEAR(path[endIdx].GetDef().arc.centerPt_x, segment.GetDef().arc.centerPt_x) &&
               FLT_NEAR(path[endIdx].GetDef().arc.centerPt_y, segment.GetDef().arc.centerPt_y) &&
               FLT_NEAR(path[endIdx].GetDef().arc.radius, segment.GetDef().arc.radius)) {

          path[endIdx].GetDef().arc.sweepRad += segment.GetDef().arc.sweepRad;
          shouldAdd = false;
        }
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

    segment.GetEndPoint(xx,yy);
    // printf("end: (%f, %f)\n", xx, yy);
  }

  if(!added && firstSegment > 0)
    firstSegment--;

  return firstSegment;
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
  float x0,y0,x1,y1,len;

  while(fscanf(fEnv, "%f %f %f %f %f", &x0, &y0, &x1, &y1, &len) == 5) {
    obstacles_.emplace_back(x0, y0, x1, y1, len);
  }

  return true;
}

void xythetaEnvironment::WriteEnvironment(const char *filename) const
{
  ofstream outfile(filename);
  for(const auto& it : obstacles_) {
    it.Dump(outfile);
  }
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

    printf("(%d) %s\n", curr.theta, actionTypes_[actionID].GetName().c_str());

    const MotionPrimitive* prim = &allMotionPrimitives_[curr.theta][actionID];
    u8 pathSegmentOffset = prim->AddSegmentsToPath(State2State_c(curr), path);

    curr.x += prim->endStateOffset.x;
    curr.y += prim->endStateOffset.y;
    curr.theta = prim->endStateOffset.theta;
  }
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
        float x = curr_c.x_mm + prim->intermediatePositions[j].x_mm;
        float y = curr_c.y_mm + prim->intermediatePositions[j].y_mm;
        float theta = prim->intermediatePositions[j].theta;

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
