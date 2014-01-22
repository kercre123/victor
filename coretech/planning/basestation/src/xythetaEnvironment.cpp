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

bool State::Import(Json::Value& config)
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
  return x==other.x && y==other.y && theta==other.theta;
}


std::ostream& operator<<(std::ostream& out, const State& state)
{
  return out<<'('<<(int)state.x<<','<<(int)state.y<<','<<(int)state.theta<<')';
}

bool State_c::Import(Json::Value& config)
{
  if(!JsonTools::GetValueOptional(config, "x_mm", x_mm) ||
         !JsonTools::GetValueOptional(config, "y_mm", y_mm) ||
         !JsonTools::GetValueOptional(config, "theta_rad", theta)) {
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

xythetaEnvironment::~xythetaEnvironment()
{
  for(size_t i=0; i<obstacles_.size(); ++i)
    delete obstacles_[i];
}


xythetaEnvironment::xythetaEnvironment()
{
  numAngles_ = 1<<THETA_BITS;
  radiansPerAngle_ = 2*M_PI/numAngles_;
}

xythetaEnvironment::xythetaEnvironment(const char* mprimFilename, const char* mapFile)
{
  // TODO:(bn) replace this with less horrible code!
  /* unused variables, remove?
  char sTemp[1024], sExpected[1024];
  float fTemp;
  int dTemp;
  int totalNumofActions = 0;
  */

  if(!ReadMotionPrimitives(mprimFilename)) {
    if(mapFile != NULL) {
      FILE* fMap = fopen(mapFile, "r");
      ReadEnvironment(fMap);
      fclose(fMap);
    }

    numAngles_ = 1<<THETA_BITS;
    radiansPerAngle_ = 2*M_PI/numAngles_;
  }
  else {
    printf("error: could not parse motion primitives!\n");
  }
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
      if(!p.Import(prims[i], angle)) {
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

bool MotionPrimitive::Import(Json::Value& config, StateTheta startingAngle)
{
  startTheta = startingAngle;

  if(!JsonTools::GetValueOptional(config, "prim_id", id)) {
    printf("error: missing key 'prim_id'\n");
    JsonTools::PrintJson(config, 1);
    return false;
  }

  if(!endStateOffset.Import(config["end_pose"])) {
    printf("error: could not read 'end_pose'\n");
    return false;
  }

  name = config.get("name", "").asString();

  Cost costFactor = config.get("extra_cost_factor", 1.0f).asFloat();
  cost = 1.0 * costFactor;  // TODO:(bn) cost!

  unsigned int numIntermediatePoses = config["intermediate_poses"].size();
  for(unsigned int i=0; i<numIntermediatePoses; ++i) {
    State_c s;
    if(!s.Import(config["intermediate_poses"][i])) {
      printf("error: could not read 'intermediate_poses'[%d]\n", i);
        return false;
    }
    intermediatePositions.push_back(s);
  }

  return true;
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
    // printf("curr = (%f, %f, %f [%d])\n", curr_c.x_mm, curr_c.y_mm, curr_c.theta, currTheta);

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
