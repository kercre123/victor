#include "anki/planning/basestation/xythetaEnvironment.h"
#include "rectangle.h"


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
  return x | y<<MAX_XY_BITS | theta<<(2*MAX_XY_BITS);
}


std::ostream& operator<<(std::ostream& out, const State& state)
{
  return out<<'('<<state.x<<','<<state.y<<','<<state.theta<<')';
}

SuccessorIterator::SuccessorIterator(const xythetaEnvironment* env, StateID startID, Cost startG)
  : start_c_(env->StateID2State_c(startID)),
    startG_(startG),
    nextAction_(0),
    motionPrimitives_(env->allMotionPrimitives_[xythetaEnvironment::GetThetaFromStateID(startID)]),
    obstacles_(env->obstacles_)
{
}

// TODO:(bn) inline?
bool SuccessorIterator::Done() const
{
  return nextAction_ >= motionPrimitives_.size();
}

void SuccessorIterator::Next()
{
  while(nextAction_ < motionPrimitives_.size()) {
    const MotionPrimitive& prim = motionPrimitives_[nextAction_];

    // collision checking
    size_t endObs = obstacles_.size();
    int endPoints = prim.intermediatePositions.size();
    bool collision = false;

    // iterate first through action, starting at the end because this
    // is more likely to be a collision
    for(int pointIdx = endPoints-1; pointIdx >= 0 && !collision; --pointIdx) {
      for(size_t obsIdx=0; obsIdx<endObs; ++obsIdx) {
        float x,y;
        x = start_c_.x_cm + prim.intermediatePositions[pointIdx].x_cm;
        y = start_c_.y_cm + prim.intermediatePositions[pointIdx].y_cm;
        if(obstacles_[obsIdx]->IsPointInside(x, y)) {
          collision = true;
          break;
        }
      }
    }

    nextAction_++;

    if(!collision) {
      nextSucc_.stateID = prim.endID;
      nextSucc_.g = startG_ + prim.cost;
      nextSucc_.actionID = nextAction_;
      break;
    }
  }
}

xythetaEnvironment::xythetaEnvironment(const char* mprimFilename, const char* mapFile)
{

}

SuccessorIterator xythetaEnvironment::GetSuccessors(StateID startID, Cost currG) const
{
  return SuccessorIterator(this, startID, currG);
}


}
}
