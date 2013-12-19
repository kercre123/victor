#include "anki/planning/basestation/xythetaEnvironment.h"
#include "rectangle.h"
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
        x = start_c_.x_cm + prim->intermediatePositions[pointIdx].x_cm;
        y = start_c_.y_cm + prim->intermediatePositions[pointIdx].y_cm;
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
  
  FILE* fMotPrims = fopen(mprimFilename, "r");
  ReadMotionPrimitives(fMotPrims);
  fclose(fMotPrims);

  if(mapFile != NULL) {
    FILE* fMap = fopen(mapFile, "r");
    ReadEnvironment(fMap);
    fclose(fMap);
  }

  numAngles_ = 1<<THETA_BITS;
  radiansPerAngle_ = 2*M_PI/numAngles_;
}

bool xythetaEnvironment::ReadMotionPrimitives(FILE* fMotPrims)
{
  char sTemp[1024], sExpected[1024];
  float fTemp;
  int dTemp;
  int totalNumofActions = 0;

  printf("Reading in motion primitives...\n");

  //read in the resolution
  strcpy(sExpected, "resolution_m:");
  if (fscanf(fMotPrims, "%s", sTemp) == 0) return false;
  if (strcmp(sTemp, sExpected) != 0) {
    printf("ERROR: expected %s but got %s\n", sExpected, sTemp);
    return false;
  }
  if (fscanf(fMotPrims, "%f", &fTemp) == 0) return false;
  resolution_cm_ = fTemp;
  // if (fabs(fTemp - EnvNAVXYTHETALATCfg.cellsize_m) > ERR_EPS) {
  //   printf("ERROR: invalid resolution %f (instead of %f) in the dynamics file\n", fTemp,
  //                  EnvNAVXYTHETALATCfg.cellsize_m);
  //   return false;
  // }

  //read in the angular resolution
  strcpy(sExpected, "numberofangles:");
  if (fscanf(fMotPrims, "%s", sTemp) == 0) return false;
  if (strcmp(sTemp, sExpected) != 0) {
    printf("ERROR: expected %s but got %s\n", sExpected, sTemp);
    return false;
  }
  if (fscanf(fMotPrims, "%d", &dTemp) == 0) return false;
  // if (dTemp != EnvNAVXYTHETALATCfg.NumThetaDirs) {
  //   printf("ERROR: invalid angular resolution %d angles (instead of %d angles) in the motion primitives file\n",
  //                  dTemp, EnvNAVXYTHETALATCfg.NumThetaDirs);
  //   return false;
  // }

  //read in the total number of actions
  strcpy(sExpected, "totalnumberofprimitives:");
  if (fscanf(fMotPrims, "%s", sTemp) == 0) return false;
  if (strcmp(sTemp, sExpected) != 0) {
    printf("ERROR: expected %s but got %s\n", sExpected, sTemp);
    return false;
  }
  if (fscanf(fMotPrims, "%d", &totalNumofActions) == 0) {
    return false;
  }

  for (int i = 0; i < totalNumofActions; i++) {
    MotionPrimitive motprim;

    if (ReadinMotionPrimitive(motprim, fMotPrims) == false) return false;

    // TODO:(bn) pre-allocate
    if(allMotionPrimitives_.size() <= motprim.startTheta)
      allMotionPrimitives_.resize(motprim.startTheta+1);

    if(allMotionPrimitives_[motprim.startTheta].size() <= motprim.id)
      allMotionPrimitives_[motprim.startTheta].resize(motprim.id+1);

    allMotionPrimitives_[motprim.startTheta][motprim.id] = motprim;
  }

  // TODO:(bn) yeah
  printf("done! have %lu angles and first one has %lu prims\n", allMotionPrimitives_.size(), allMotionPrimitives_[0].size());

  return true;
}

bool xythetaEnvironment::ReadinMotionPrimitive(MotionPrimitive& prim, FILE* fIn)
{
  char sTemp[1024];
  int dTemp;
  char sExpected[1024];
  int numofIntermPoses;

  //read in actionID
  strcpy(sExpected, "primID:");
  if (fscanf(fIn, "%s", sTemp) == 0) return false;
  if (strcmp(sTemp, sExpected) != 0) {
    printf("ERROR: expected %s but got %s\n", sExpected, sTemp);
    return false;
  }
  if (fscanf(fIn, "%d", &prim.id) != 1) return false;

  //read in start angle
  strcpy(sExpected, "startangle_c:");
  if (fscanf(fIn, "%s", sTemp) == 0) return false;
  if (strcmp(sTemp, sExpected) != 0) {
    printf("ERROR: expected %s but got %s\n", sExpected, sTemp);
    return false;
  }
  if (fscanf(fIn, "%d", &dTemp) == 0) {
    printf("ERROR reading startangle\n");
    return false;
  }
  prim.startTheta = dTemp;

  //read in end pose
  strcpy(sExpected, "endpose_c:");
  if (fscanf(fIn, "%s", sTemp) == 0) return false;
  if (strcmp(sTemp, sExpected) != 0) {
    printf("ERROR: expected %s but got %s\n", sExpected, sTemp);
    return false;
  }

  int x, y, theta;
  if (fscanf(fIn, "%d %d %d", &x, &y, &theta) != 3) {
    printf("ERROR: failed to read in endsearchpose\n");
    return false;
  }
  prim.endStateOffset.x = x;
  prim.endStateOffset.y = y;
  prim.endStateOffset.theta = theta;

  //read in action cost
  strcpy(sExpected, "additionalactioncostmult:");
  if (fscanf(fIn, "%s", sTemp) == 0) return false;
  if (strcmp(sTemp, sExpected) != 0) {
    printf("ERROR: expected %s but got %s\n", sExpected, sTemp);
    return false;
  }
  if (fscanf(fIn, "%d", &dTemp) != 1) return false;
  prim.cost = dTemp;

  //read in intermediate poses
  strcpy(sExpected, "intermediateposes:");
  if (fscanf(fIn, "%s", sTemp) == 0) return false;
  if (strcmp(sTemp, sExpected) != 0) {
    printf("ERROR: expected %s but got %s\n", sExpected, sTemp);
    return false;
  }
  if (fscanf(fIn, "%d", &numofIntermPoses) != 1) return false;
  //all intermposes should be with respect to 0,0 as starting pose since it will be added later and should be done
  //after the action is rotated by initial orientation
  for (int i = 0; i < numofIntermPoses; i++) {
    State_c pose;
    if(fscanf(fIn, "%f %f %f", &pose.x_cm, &pose.y_cm, &pose.theta) != 3) {
      printf("ERROR: failed to read in intermediate poses\n");
      return false;
    }
    prim.intermediatePositions.push_back(pose);
  }

  // //check that the last pose corresponds correctly to the last pose
  // sbpl_xy_theta_pt_t sourcepose;
  // sourcepose.x = DISCXY2CONT(0, EnvNAVXYTHETALATCfg.cellsize_m);
  // sourcepose.y = DISCXY2CONT(0, EnvNAVXYTHETALATCfg.cellsize_m);
  // sourcepose.theta = DiscTheta2Cont(pMotPrim->starttheta_c, EnvNAVXYTHETALATCfg.NumThetaDirs);
  // double mp_endx_m = sourcepose.x + pMotPrim->intermptV[pMotPrim->intermptV.size() - 1].x;
  // double mp_endy_m = sourcepose.y + pMotPrim->intermptV[pMotPrim->intermptV.size() - 1].y;
  // double mp_endtheta_rad = pMotPrim->intermptV[pMotPrim->intermptV.size() - 1].theta;
  // int endx_c = CONTXY2DISC(mp_endx_m, EnvNAVXYTHETALATCfg.cellsize_m);
  // int endy_c = CONTXY2DISC(mp_endy_m, EnvNAVXYTHETALATCfg.cellsize_m);
  // int endtheta_c = ContTheta2Disc(mp_endtheta_rad, EnvNAVXYTHETALATCfg.NumThetaDirs);
  // if (endx_c != pMotPrim->endcell.x || endy_c != pMotPrim->endcell.y || endtheta_c != pMotPrim->endcell.theta) {
  //   printf( "ERROR: incorrect primitive %d with startangle=%d "
  //                   "last interm point %f %f %f does not match end pose %d %d %d\n",
  //                   pMotPrim->motprimID, pMotPrim->starttheta_c,
  //                   pMotPrim->intermptV[pMotPrim->intermptV.size() - 1].x,
  //                   pMotPrim->intermptV[pMotPrim->intermptV.size() - 1].y,
  //                   pMotPrim->intermptV[pMotPrim->intermptV.size() - 1].theta,
  //                   pMotPrim->endcell.x, pMotPrim->endcell.y,
  //                   pMotPrim->endcell.theta);
  //   return false;
  // }

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
    // printf("curr = (%f, %f, %f [%d])\n", curr_c.x_cm, curr_c.y_cm, curr_c.theta, currTheta);

    if(currTheta >= allMotionPrimitives_.size() || plan.actions_[i] >= allMotionPrimitives_[currTheta].size()) {
      printf("ERROR: can't look up prim for angle %d and action id %d\n", currTheta, plan.actions_[i]);
      break;
    }
    else {

      const MotionPrimitive* prim = &allMotionPrimitives_[currTheta][plan.actions_[i]];
      for(size_t j=0; j<prim->intermediatePositions.size(); ++j) {
        float x = curr_c.x_cm + prim->intermediatePositions[j].x_cm;
        float y = curr_c.y_cm + prim->intermediatePositions[j].y_cm;
        float theta = prim->intermediatePositions[j].theta;

        // printf("  (%+5f, %+5f, %+5f) -> (%+5f, %+5f, %+5f)\n",
        //            prim->intermediatePositions[j].x_cm,
        //            prim->intermediatePositions[j].y_cm,
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
