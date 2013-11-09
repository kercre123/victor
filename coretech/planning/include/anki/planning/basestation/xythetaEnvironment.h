#ifndef _ANKICORETECH_PLANNING_XYTHETA_ENVIRONMENT_H_
#define _ANKICORETECH_PLANNING_XYTHETA_ENVIRONMENT_H_


#include <vector>

namespace Anki
{
namespace Planning
{

/** NAMING CONVENTION:
 *
 * a variable x, y, or theta normally refers to the discreet
 * position. if an _c is appended, then it refers to the continuous
 * position
 */


#define THETA_BITS 4
#define MAX_XY_BITS 14
#define INVALID_ACTION_ID 255

typedef uint8_t StateTheta;
typedef int16_t StateXY;
typedef float Cost;
typedef uint8_t ActionID;

class Rectangle;
class xythetaEnvironment;
class State;
class StateID;

class State
{
  friend std::ostream& operator<<(std::ostream& out, const State& state);
public:

  State() : x(0), y(0), theta(0) {};
  State(StateID sid);
  State(StateXY x, StateXY y, StateTheta theta) : x(x), y(y), theta(theta) {};

  bool operator==(const State& other) const;

  StateID GetStateID() const;

  StateXY x;
  StateXY y;
  StateTheta theta;
};

// bit field representation that packs into an int
class StateID
{
public:
  // This constructor adds the offset to the given state
  StateID() : theta(0), x(0), y(0) {};

  unsigned int theta : THETA_BITS;
  signed int x : MAX_XY_BITS;
  signed int y : MAX_XY_BITS;
};

// continuous states are set up with the exact state being at the
// bottom left of the cell // TODO:(bn) think more about that
class State_c
{
public:
  State_c() : x_cm(0), y_cm(0), theta(0) {};
  State_c(float x, float y, float theta) : x_cm(x), y_cm(y), theta(theta) {};

  float x_cm;
  float y_cm;
  float theta;
};

class MotionPrimitive
{
public:

  // id of this action (unique per starting angle)
  ActionID id;

  // The starting angle (discretized). This and the id uniquely define
  // this primitive
  StateTheta startTheta;

  // Cost of this action
  Cost cost;

  // This is a state where x and y will be added to the current state,
  // but the resulting angle will be set by exactly the theta
  // here. This is possible because we already know the starting theta
  State endStateOffset;

  // vector containing continuous relative offsets for positions in
  // between (0,0,startTheta) and (end)
  std::vector<State_c> intermediatePositions;
};

class Successor
{
public:
  // The successor state
  StateID stateID;

  // The action thats takes you to state
  ActionID actionID;

  // Values associated with state
  Cost g;
};


class SuccessorIterator
{
public:
  SuccessorIterator(const xythetaEnvironment* env, StateID startID, Cost startG);

  // Returns true if there are no more results left
  bool Done() const;

  // Returns the next action results pair
  inline const Successor& Front() const {return nextSucc_;}

  void Next();

private:

  State_c start_c_;
  State start_;
  Cost startG_;

  // once Next() is called, it will evaluate this action
  ActionID nextAction_;

  Successor nextSucc_;

  const std::vector<MotionPrimitive>& motionPrimitives_;
  const std::vector<Rectangle*>& obstacles_;
};

// TODO:(bn) move some of these to seperate files
class xythetaPlan
{
public:
  State start_;
  std::vector<ActionID> actions_;
  
  void Push(ActionID action) {actions_.push_back(action);}
};

class xythetaEnvironment
{
  friend class SuccessorIterator;
public:
  // for testing, has no prims or map!
  xythetaEnvironment();

  // just for now, eventually we won't use filenames like this, obviously......
  xythetaEnvironment(const char* mprimFilename, const char* mapFile);

  // Returns an iterator to the successors from state "start"
  SuccessorIterator GetSuccessors(StateID startID, Cost currG) const;

  inline State_c State2State_c(const State& c) const;
  inline State_c StateID2State_c(StateID sid) const;

  inline static float GetXFromStateID(StateID sid);
  inline static float GetYFromStateID(StateID sid);
  inline static float GetThetaFromStateID(StateID sid);

  inline float GetX_cm(StateXY x) const;
  inline float GetY_cm(StateXY y) const;
  inline float GetTheta_c(StateTheta theta) const;

  // This function fills up the given vector with (x,y,theta) coordinates of
  // following the plan
  void ConvertToXYPlan(const xythetaPlan& plan, std::vector<State_c>& continuousPlan) const;

private:

  // returns true on success
  bool ReadMotionPrimitives(FILE* fMotPrims);
  bool ReadinMotionPrimitive(MotionPrimitive& prim, FILE* fMotPrims);

  float resolution_cm_;

  unsigned int numAngles_;
  float radiansPerAngle_;

  // First index is starting angle, second is prim ID
  std::vector< std::vector<MotionPrimitive> > allMotionPrimitives_;

  // Obstacles
  std::vector<Rectangle*> obstacles_;

};

// TODO:(bn) pull out into _inline.cpp
State_c xythetaEnvironment::State2State_c(const State& c) const
{
  return State_c(GetX_cm(c.x), GetY_cm(c.y), GetTheta_c(c.theta));
}

State_c xythetaEnvironment::StateID2State_c(StateID sid) const
{
  return State_c(GetX_cm(sid.x), GetY_cm(sid.y), GetTheta_c(sid.theta));
}

float xythetaEnvironment::GetXFromStateID(StateID sid)
{
  return sid.x; //sid & ((1<<MAX_XY_BITS) - 1);
}

float xythetaEnvironment::GetYFromStateID(StateID sid)
{
  return sid.y; //(sid >> MAX_XY_BITS) & ((1<<MAX_XY_BITS) - 1);
}

float xythetaEnvironment::GetThetaFromStateID(StateID sid)
{
  return sid.theta; //sid >> (2*MAX_XY_BITS);
}

float xythetaEnvironment::GetX_cm(StateXY x) const
{
  return resolution_cm_ * x;
}

float xythetaEnvironment::GetY_cm(StateXY y) const
{
  return resolution_cm_ * y;
}

float xythetaEnvironment::GetTheta_c(StateTheta theta) const
{
  return theta * radiansPerAngle_;
}


}
}

#endif
