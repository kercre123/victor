#ifndef _ANKICORETECH_PLANNING_XYTHETA_ENVIRONMENT_H_
#define _ANKICORETECH_PLANNING_XYTHETA_ENVIRONMENT_H_

#include "json/json-forwards.h"
#include <assert.h>
#include <math.h>
#include <string>
#include <vector>
#include "anki/planning/shared/path.h"

#include "anki/common/basestation/math/quad.h"

namespace Anki
{

class RotatedRectangle;

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

  // returns true if successful
  bool Import(const Json::Value& config);

  bool operator==(const State& other) const;
  bool operator!=(const State& other) const;

  StateID GetStateID() const;

  StateXY x;
  StateXY y;
  StateTheta theta;
};

// bit field representation that packs into an int
class StateID
{
  friend bool operator==(const StateID& lhs, const StateID& rhs);
public:

  bool operator<(const StateID& rhs) const;

  StateID() : theta(0), x(0), y(0) {};
  StateID(const State& state) { *this = state.GetStateID(); };

  // TODO:(bn) check that these are packed properly
  unsigned int theta : THETA_BITS;
  signed int x : MAX_XY_BITS;
  signed int y : MAX_XY_BITS;
};

// continuous states are set up with the exact state being at the
// bottom left of the cell // TODO:(bn) think more about that, should probably add half a cell width
class State_c
{
  friend std::ostream& operator<<(std::ostream& out, const State_c& state);
public:
  State_c() : x_mm(0), y_mm(0), theta(0) {};
  State_c(float x, float y, float theta) : x_mm(x), y_mm(y), theta(theta) {};

  // returns true if successful
  bool Import(const Json::Value& config);

  float x_mm;
  float y_mm;
  float theta;
};

class MotionPrimitive
{
public:

  MotionPrimitive() {}

  // returns true if successful
  bool Import(const Json::Value& config, StateTheta startingAngle, const xythetaEnvironment& env);

  // returns the minimum PathSegmentOffset associated with this action
  u8 AddSegmentsToPath(State_c start, Path& path) const;

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
private:

  std::vector<PathSegment> pathSegments_;
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
  const std::vector<RotatedRectangle>& obstacles_;
};

// TODO:(bn) move some of these to seperate files
class xythetaPlan
{
public:
  State start_;
  std::vector<ActionID> actions_;

  size_t Size() const {return actions_.size();}
  void Push(ActionID action) {actions_.push_back(action);}
  void Clear() {actions_.clear();}
};

// This class contains generic information for a type of action
class ActionType
{
public:
  
  ActionType();

  // returns true if successful
  bool Import(const Json::Value& config);

  const std::string& GetName() const {return name_;}
  Cost GetExtraCostFactor() const {return extraCostFactor_;}  
  bool IsReverseAction() const {return reverse_;}

private:

  Cost extraCostFactor_;
  ActionID id_;
  std::string name_;
  bool reverse_;
};

class xythetaEnvironment
{
  friend class SuccessorIterator;
public:
  // for testing, has no prims or map!
  xythetaEnvironment();

  // just for now, eventually we won't use filenames like this, obviously......
  // TEMP: remove this and only use Init instead
  xythetaEnvironment(const char* mprimFilename, const char* mapFile);

  ~xythetaEnvironment();

  // returns true if everything worked
  bool Init(const char* mprimFilename, const char* mapFile);

  // inits with motion primitives and an empty environment
  bool Init(const Json::Value& mprimJson);

  // dumps the obstacles to the given map file
  void WriteEnvironment(const char* mapFile) const;

  // Imports motion primitives from the given json file. Returns true if success
  bool ReadMotionPrimitives(const char* mprimFilename);

  void AddObstacle(const RotatedRectangle& rect);
  void AddObstacle(const Quad2f& quad);
  void ClearObstacles();

  // Returns an iterator to the successors from state "start". Use
  // this one if you want to check each action
  SuccessorIterator GetSuccessors(StateID startID, Cost currG) const;

  // This function tries to apply the given action to the state. If it
  // is a valid, collision-free action, it updates state to be the
  // successor and returns true, otherwise it returns false and does
  // not change state
  bool ApplyAction(const ActionID& action, StateID& stateID, bool checkCollisions = true) const;

  // returns the raw underlying motion primitive. Note that it is centered at (0,0)
  const MotionPrimitive& GetRawMotionPrimitive(StateTheta theta, ActionID action) const;

  // Returns true if there is a collision at the given state
  bool IsInCollision(State s) const;
  bool IsInCollision(State_c c) const;

  inline State_c State2State_c(const State& s) const;
  inline State_c StateID2State_c(StateID sid) const;

  inline State State_c2State(const State_c& c) const;

  inline static float GetXFromStateID(StateID sid);
  inline static float GetYFromStateID(StateID sid);
  inline static float GetThetaFromStateID(StateID sid);

  inline float GetX_mm(StateXY x) const;
  inline float GetY_mm(StateXY y) const;
  inline float GetTheta_c(StateTheta theta) const;

  inline StateXY GetX(float x_mm) const;
  inline StateXY GetY(float y_mm) const;
  inline StateTheta GetTheta(float theta_rad) const;

  float GetDistanceBetween(const State_c& start, const State& end) const;
  static float GetDistanceBetween(const State_c& start, const State_c& end);

  // Get a motion primitive. Returns true if the action is retrieved,
  // false otherwise. Returns primitive in arguments
  inline bool GetMotion(StateTheta theta, ActionID actionID, MotionPrimitive& prim) const;

  inline size_t GetNumActions() const { return actionTypes_.size(); }
  
  inline const ActionType& GetActionType(ActionID aid) const { return actionTypes_[aid]; }

  // This function fills up the given vector with (x,y,theta) coordinates of
  // following the plan.
  void ConvertToXYPlan(const xythetaPlan& plan, std::vector<State_c>& continuousPlan) const;

  void PrintPlan(const xythetaPlan& plan) const;

  // Convert the plan to Planning::PathSegment's and append it to
  // path. Also updates plan to set the robotPathSegmentIdx_
  void AppendToPath(xythetaPlan& plan, Path& path) const;

  // TODO:(bn) move these??

  double GetHalfWheelBase_mm() const {return halfWheelBase_mm_;}
  double GetMaxVelocity_mmps() const {return maxVelocity_mmps_;}
  double GetMaxReverseVelocity_mmps() const {return maxReverseVelocity_mmps_;}

  double GetOneOverMaxVelocity() const {return oneOverMaxVelocity_;}

  float GetResolution_mm() const { return resolution_mm_; }

private:

  // returns true on success
  bool ReadEnvironment(FILE* fEnv);
  bool ParseMotionPrims(const Json::Value& config);

  float resolution_mm_;
  float oneOverResolution_;

  unsigned int numAngles_;

  // NOTE: these are approximate. The actual angles aren't evenly
  // distrubuted, but this will get you close
  float radiansPerAngle_;
  float oneOverRadiansPerAngle_;

  // First index is starting angle, second is prim ID
  std::vector< std::vector<MotionPrimitive> > allMotionPrimitives_;

  // Obstacles
  std::vector<RotatedRectangle> obstacles_;

  // index is actionID
  std::vector<ActionType> actionTypes_;

  // index is StateTheta, value is theta in radians, between -pi and
  // pi. If there are more than 8 angles, they are not perfectly
  // evenly divided (see docs)  // TODO:(bn) write docs....
  std::vector<float> angles_;

  // robot parameters. These probably don't belong here, but are needed for computing costs

  double halfWheelBase_mm_;
  double maxVelocity_mmps_;
  double maxReverseVelocity_mmps_;
  double oneOverMaxVelocity_;
};

bool xythetaEnvironment::GetMotion(StateTheta theta, ActionID actionID, MotionPrimitive& prim) const
{
  if(theta < allMotionPrimitives_.size() && actionID < allMotionPrimitives_[theta].size()) {
    prim = allMotionPrimitives_[theta][actionID];
    return true;
  }
  return false;
}

// TODO:(bn) pull out into _inline.cpp
State_c xythetaEnvironment::State2State_c(const State& s) const
{
  return State_c(GetX_mm(s.x), GetY_mm(s.y), GetTheta_c(s.theta));
}

State xythetaEnvironment::State_c2State(const State_c& c) const
{
  return State(GetX(c.x_mm), GetY(c.y_mm), GetTheta(c.theta));
}

State_c xythetaEnvironment::StateID2State_c(StateID sid) const
{
  return State_c(GetX_mm(sid.x), GetY_mm(sid.y), GetTheta_c(sid.theta));
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

float xythetaEnvironment::GetX_mm(StateXY x) const
{
  return resolution_mm_ * x;
}

float xythetaEnvironment::GetY_mm(StateXY y) const
{
  return resolution_mm_ * y;
}

float xythetaEnvironment::GetTheta_c(StateTheta theta) const
{
  assert(theta >= 0 && theta < angles_.size());
  return angles_[theta];
}

StateXY xythetaEnvironment::GetX(float x_mm) const
{
  return (StateXY) roundf(x_mm * oneOverResolution_);
}

StateXY xythetaEnvironment::GetY(float y_mm) const
{
  return (StateXY) roundf(y_mm * oneOverResolution_);
}

StateTheta xythetaEnvironment::GetTheta(float theta_rad) const
{
  float positiveTheta = theta_rad;
  // TODO:(bn) something faster (std::remainder ??)
  while(positiveTheta < 0.0) {
    positiveTheta += 2*M_PI;
  }
  
  while(positiveTheta >= 2*M_PI) {
    positiveTheta -= 2*M_PI;
  }
  
  const StateTheta stateTheta = (StateTheta) std::floor(positiveTheta * oneOverRadiansPerAngle_);

  assert(stateTheta >= 0 && stateTheta < angles_.size());
  
  return stateTheta;
}



}
}

#endif
