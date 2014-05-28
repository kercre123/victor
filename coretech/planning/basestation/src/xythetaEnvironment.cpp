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
#include "anki/common/shared/radians.h"
#include "anki/planning/basestation/xythetaEnvironment.h"
#include "json/json.h"
#include "rectangle.h"
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdio.h>

using namespace std;

namespace Anki {
namespace Planning {

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
  return out<<'('<<(int)state.x<<','<<(int)state.y<<','<<(int)state.theta<<')';
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
        if(obstacles_[obsIdx]->IsPointInside(x, y)) {
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

ActionType::ActionType()
  : extraCostFactor_(0.0)
  , id_(-1)
  , name_("<invalid>")
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
  return true;
}

xythetaEnvironment::~xythetaEnvironment()
{
  for(size_t i=0; i<obstacles_.size(); ++i)
    delete obstacles_[i];
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
}

xythetaEnvironment::xythetaEnvironment(const char* mprimFilename, const char* mapFile)
{
  Init(mprimFilename, mapFile);
  halfWheelBase_mm_ = 25.0;
  maxVelocity_mmps_ = 50.0;
  oneOverMaxVelocity_ = 1.0 / maxVelocity_mmps_;
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

bool xythetaEnvironment::ParseMotionPrims(Json::Value& config)
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

  // Compute cost based on the action. Cost is time in seconds
  cost = 0.0;
  if(config.isMember("arc")) {
    // the section of the angle we will sweep through
    double deltaTheta = std::abs(config["arc"]["sweepRad"].asDouble());

    // the radius of the circle that the outer wheel will follow
    double radius_mm = std::abs(config["arc"]["radius_mm"].asDouble()) + env.GetHalfWheelBase_mm();

    // the total time is the arclength of the outer wheel arc divided by the max outer wheel speed
    cost += deltaTheta * radius_mm * env.GetOneOverMaxVelocity();
  }
  else if(config.isMember("turn_in_place_direction")) {
    double direction = config["turn_in_place_direction"].asDouble();

    // turn in place is just like an arc with radius 0
    Radians startRads(env.GetTheta_c(startTheta));
    double deltaTheta = startRads.angularDistance(env.GetTheta_c(endStateOffset.theta), direction < 0);

    cost +=  std::abs(deltaTheta) * env.GetHalfWheelBase_mm() * env.GetOneOverMaxVelocity();
  }

  double length = std::abs(config["straight_length_mm"].asDouble());
  if(length > 0.0) {
    cost += length * env.GetOneOverMaxVelocity();
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
    obstacles_.push_back(new Rectangle(x0, y0, x1, y1, len));
  }

  return true;
}


SuccessorIterator xythetaEnvironment::GetSuccessors(StateID startID, Cost currG) const
{
  return SuccessorIterator(this, startID, currG);
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
