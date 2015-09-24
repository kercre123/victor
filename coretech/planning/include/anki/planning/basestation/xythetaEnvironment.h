#ifndef _ANKICORETECH_PLANNING_XYTHETA_ENVIRONMENT_H_
#define _ANKICORETECH_PLANNING_XYTHETA_ENVIRONMENT_H_

#include "anki/common/basestation/math/fastPolygon2d.h"
#include "anki/common/basestation/math/quad.h"
#include "anki/common/basestation/math/polygon.h"
#include "anki/planning/shared/path.h"
#include "json/json-forwards.h"
#include "xythetaPlanner_definitions.h"
#include <assert.h>
#include <cmath>
#include <cfloat>
#include <string>
#include <vector>

namespace Anki
{

class RotatedRectangle;

namespace Util {
class JsonWriter;
}

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

#define MAX_OBSTACLE_COST 1000.0f
#define FATAL_OBSTACLE_COST (MAX_OBSTACLE_COST + 1.0f)

typedef uint8_t StateTheta;
typedef int16_t StateXY;
typedef uint8_t ActionID;

class State;
class xythetaEnvironment;
union StateID;

struct Point
{
  Point(StateXY x, StateXY y) : x(x), y(y) {}

  StateXY x;
  StateXY y;
};

class State
{
  friend std::ostream& operator<<(std::ostream& out, const State& state);
public:

  State() : x(0), y(0), theta(0) {};
  State(StateID sid);
  State(StateXY x, StateXY y, StateTheta theta) : x(x), y(y), theta(theta) {};

  // returns true if successful
  bool Import(const Json::Value& config);
  void Dump(Util::JsonWriter& writer) const;

  bool operator==(const State& other) const;
  bool operator!=(const State& other) const;

  StateID GetStateID() const;

  StateXY x;
  StateXY y;
  StateTheta theta;
};

union StateIDConverter;

// bit field representation that packs into an int
union StateID
{
  StateID() : v(0) {}
  StateID(const StateID& other) : v(other.v) {}
  StateID(const State& state) : s(state) {}

  // Allow for conversion freely to/from int
  StateID(u32 val) : v(val) {}
  operator u32() { return v; }
  operator u32() const { return v; }

  struct S
  {
    S() : theta(0), x(0), y(0) {}
    explicit S(const State& state) : S( state.GetStateID().s ) {}

    unsigned int theta : THETA_BITS;
    signed int x : MAX_XY_BITS;
    signed int y : MAX_XY_BITS;
  };

  S s;
  u32 v;
};

static_assert(sizeof(StateID::S) == sizeof(u32), "StateID must pack into 32 bits");

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
  void Dump(Util::JsonWriter& writer) const;

  float x_mm;
  float y_mm;
  float theta;
};

struct IntermediatePosition
{
  IntermediatePosition()
    : nearestTheta(0)
    , oneOverDistanceFromLastPosition(1.0f)
    {
    }

  IntermediatePosition(State_c s, StateTheta nearestTheta, float d)
    : position(s)
    , nearestTheta(nearestTheta)
    , oneOverDistanceFromLastPosition(d)
    {
    }

  // returns true on success
  bool Import(const Json::Value& config);
  void Dump(Util::JsonWriter& writer) const;

  State_c position;
  StateTheta nearestTheta;
  float oneOverDistanceFromLastPosition;
};

class MotionPrimitive
{
public:

  MotionPrimitive() {}

  // reads raw primitive config and uses the starting angle and environment to compute what is needed
  // Returns true if successful
  bool Create(const Json::Value& config, StateTheta startingAngle, const xythetaEnvironment& env);

  // returns true if successful, reads a complete config as created by the Dump() call
  bool Import(const Json::Value& config);
  void Dump(Util::JsonWriter& writer) const;

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
  std::vector<IntermediatePosition> intermediatePositions;
private:

  Path pathSegments_;
};

class Successor
{
public:
  // The successor state
  StateID stateID;

  // The action thats takes you to state
  ActionID actionID;

  // Value associated with state
  Cost g;

  // Penalty paid to transition into state (not counting normal action cost)
  Cost penalty;
};


class SuccessorIterator
{
public:
  SuccessorIterator(const xythetaEnvironment* env, StateID startID, Cost startG);

  // Returns true if there are no more results left
  bool Done(const xythetaEnvironment& env) const;

  // Returns the next action results pair
  inline const Successor& Front() const {return nextSucc_;}

  void Next(const xythetaEnvironment& env);

private:

  State_c start_c_;
  State start_;
  Cost startG_;

  // once Next() is called, it will evaluate this action
  ActionID nextAction_;

  Successor nextSucc_;
};

// TODO:(bn) move some of these to seperate files
class xythetaPlan
{
public:
  State start_;
  std::vector<ActionID> actions_;
  
  // same size as actions, stores the penalty expected for each
  // action. This allows replanning if any of these penalties
  // increase. The sum of this should be the penalty of the entire
  // plan
  std::vector<Cost> penalties_;

  // add the given plan to the end of this plan
  void Append(const xythetaPlan& other);

  size_t Size() const {return actions_.size();}
  void Push(ActionID action, Cost penalty = 0.0) {
    actions_.push_back(action);
    penalties_.push_back(penalty);
  }
  void Clear() {
    actions_.clear();
    penalties_.clear();
  }
};

// This class contains generic information for a type of action
class ActionType
{
public:
  
  ActionType();

  // returns true if successful
  bool Import(const Json::Value& config);
  void Dump(Util::JsonWriter& writer) const;


  const std::string& GetName() const {return _name;}
  Cost GetExtraCostFactor() const {return _extraCostFactor;}  
  bool IsReverseAction() const {return _reverse;}

private:

  Cost _extraCostFactor;
  ActionID _id;
  std::string _name;
  bool _reverse;
};


// This struct holds values that the motions will use to calculate costs, based on the turning radius and
// velocity of the robot
struct RobotActionParams
{
  RobotActionParams();

  void Reset();

  // returns true on success
  bool Import(const Json::Value& config);
  void Dump(Util::JsonWriter& writer) const;

  double halfWheelBase_mm;
  double maxVelocity_mmps;
  double maxReverseVelocity_mmps;
  double oneOverMaxVelocity;
};


class xythetaEnvironment
{
  friend class SuccessorIterator;
  friend class MotionPrimitive;
public:
  // for testing, has no prims or map!
  xythetaEnvironment();

  // just for now, eventually we won't use filenames like this, obviously......
  // TEMP: remove this and only use Init instead
  xythetaEnvironment(const char* mprimFilename);

  ~xythetaEnvironment();

  // returns true if everything worked
  bool Init(const char* mprimFilename);

  // inits with motion primitives and an empty environment
  bool Init(const Json::Value& mprimJson);

  // Imports motion primitives from the given json file. Returns true if success
  bool ReadMotionPrimitives(const char* mprimFilename);

  // Dumps the entire environment to JSON. after dumping, deleting, then importing, the environment should be
  // identical
  void Dump(Util::JsonWriter& writer) const;

  // Imports the environment from the config. Returns true on success
  bool Import(const Json::Value& config);

  // defaults to a fatal obstacle  // TEMP: will be removed
  void AddObstacle(const RotatedRectangle& rect, Cost cost = FATAL_OBSTACLE_COST);
  void AddObstacle(const Quad2f& quad, Cost cost = FATAL_OBSTACLE_COST);

  // returns a polygon which represents the obstacle expanded to the
  // c-space of robot, where the origin of tobot is (0,0)
  static FastPolygon ExpandCSpace(const Poly2f& obstacle,
                                  const Poly2f& robot);

  // adds an obstacle into the c-space map for the given angle with
  // the given robot footprint, centered at (0,0). Returns reference
  // to the polygon it inserted
  const FastPolygon& AddObstacleWithExpansion(const Poly2f& obstacle,
                                              const Poly2f& robot,
                                              StateTheta theta,
                                              Cost cost = FATAL_OBSTACLE_COST);

  void ClearObstacles();

  size_t GetNumObstacles() const;

  // Returns an iterator to the successors from state "start". Use
  // this one if you want to check each action
  SuccessorIterator GetSuccessors(StateID startID, Cost currG) const;

  // This function tries to apply the given action to the state. It
  // returns the penalty of the path (i.e. no cost for actions, just
  // obstacle penalties). If the action finishes without a fatal
  // penalty, stateID will be updated to the successor state
  Cost ApplyAction(const ActionID& action, StateID& stateID, bool checkCollisions = true) const;

  // Returns the state at the end of the given plan (e.g. following
  // along the plans start and executing every action). No collision
  // checks are performed
  State GetPlanFinalState(const xythetaPlan& plan) const;


  // This essentially projects the given pose onto the plan (held in
  // this member). The projection is just the closest euclidean
  // distance point on the plan, and the return value is the number of
  // complete plan actions that are finished by the time you get to
  // this point. Returns the best distance in argument
  size_t FindClosestPlanSegmentToPose(const xythetaPlan& plan,
                                      const State_c& state,
                                      float& distanceToPlan,
                                      bool debug = false) const;

  // Returns true if the plan is safe and complete, false otherwise, including if the penalty increased by too
  // much (see REPLAN_PENALTY_BUFFER in the cpp file). This should always return true immediately after Replan
  // returns true, but if the environment is updated it can be useful to re-check the plan.
  // 
  // Arguments:
  // 
  // plan - the plan to check, will not be modified
  //
  // maxDistancetoFollowOldPlan_mm - Even if the plan is not safe, we may still be able to use part of the old
  // plan. This is the maximum "amount" of old plan that we want to use, in terms of distance. If this number
  // were 0, we'd replan without using any of the old plan, which would maximize efficiency of the new plan
  // (since it isn't artificially limited to follow the old plan), but the robot is currently driving while we
  // plan, so to avoid a jerky sudden stop, or being in the wrong place, we do want to keep some of the old
  // plan. If the number were FLT_MAX or something, we could keep the whole old plan, which would be safe, but
  // might look really dumb in case we need to go a different way after seeing the new obstacle
  // 
  // currentPathIndex - The index in the plan that the robot is currently following. The Plan is the full plan
  // from the last planning cycle, which means that some of it has already been executed. Note that this is
  // different from the robot's currentPathSegment because our plans are different from robot paths
  // 
  // lastSafeState - will be set to the last state which is safe and fits within maxDistancetoFollowOldPlan_mm
  // (this is where a new plan could start)
  // 
  // validPlan - will be set to the potion of the plan which is valid (no collision or increased penalty),
  // within maxDistancetoFollowOldPlan_mm
  bool PlanIsSafe(const xythetaPlan& plan,
                  const float maxDistancetoFollowOldPlan_mm,
                  int currentPathIndex = 0) const;
  bool PlanIsSafe(const xythetaPlan& plan, 
                  const float maxDistancetoFollowOldPlan_mm,
                  int currentPathIndex,
                  State_c& lastSafeState,
                  xythetaPlan& validPlan) const;

  // If we are going to be doing a full planner cycle, this function
  // will be called to prepare the environment, including
  // precomputing things, etc.
  void PrepareForPlanning();

  // returns the raw underlying motion primitive. Note that it is centered at (0,0)
  const MotionPrimitive& GetRawMotionPrimitive(StateTheta theta, ActionID action) const;

  // Returns true if there is a fatal collision at the given state
  bool IsInCollision(State s) const;
  bool IsInCollision(State_c c) const;

  bool IsInSoftCollision(State s) const;

  Cost GetCollisionPenalty(State s) const;

  inline State_c State2State_c(const State& s) const;
  inline State_c StateID2State_c(StateID sid) const;

  // round the continuous state to the nearest discrete state, ignoring obstacles
  inline State State_c2State(const State_c& c) const;

  // round the continuous state to the nearest discrete state which is
  // safe. Return true if success, false if all 4 corners are unsafe
  // (unlikely but possible)
  bool RoundSafe(const State_c& c, State& rounded) const;

  unsigned int GetNumAngles() const {return numAngles_;}

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

  double GetMaxVelocity_mmps() const {return _robotParams.maxVelocity_mmps;}
  double GetMaxReverseVelocity_mmps() const {return _robotParams.maxReverseVelocity_mmps;}
  double GetOneOverMaxVelocity() const {return _robotParams.oneOverMaxVelocity;}

  // TEMP:  // TEMP:  // TEMP: call this with the real values
  void SetRobotActionParams(double halfWheelBase_mm,
                            double maxVelocity_mmps,
                            double maxReverseVelocity_mmps);

  float GetResolution_mm() const { return resolution_mm_; }

private:

  // returns true on success
  bool ParseMotionPrims(const Json::Value& config, bool useDumpFormat = false);
  bool ParseObstacles(const Json::Value& config);

  void DumpMotionPrims(Util::JsonWriter& writer) const;
  void DumpObstacles(Util::JsonWriter& writer) const;

  float resolution_mm_;
  float oneOverResolution_;

  unsigned int numAngles_;

  // NOTE: these are approximate. The actual angles aren't evenly
  // distrubuted, but this will get you close
  float radiansPerAngle_;
  float oneOverRadiansPerAngle_;

  // First index is starting angle, second is prim ID
  std::vector< std::vector<MotionPrimitive> > allMotionPrimitives_;  

  // index is actionID
  std::vector<ActionType> actionTypes_;

  // index is StateTheta, value is theta in radians, between -pi and
  // pi. If there are more than 8 angles, they are not perfectly
  // evenly divided (see docs)  // TODO:(bn) write docs....
  std::vector<float> angles_;

  // Obstacles per theta. First index is theta, second of pair is cost
  std::vector< std::vector< std::pair<FastPolygon, Cost> > > obstaclesPerAngle_;

  // TODO:(bn) this won't support multiple different robots!
  RobotActionParams _robotParams;
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
  return State_c(GetX_mm(sid.s.x), GetY_mm(sid.s.y), GetTheta_c(sid.s.theta));
}

float xythetaEnvironment::GetXFromStateID(StateID sid)
{
  return sid.s.x;
}

float xythetaEnvironment::GetYFromStateID(StateID sid)
{
  return sid.s.y;
}

float xythetaEnvironment::GetThetaFromStateID(StateID sid)
{
  return sid.s.theta;
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
  
  const StateTheta stateTheta = (StateTheta) std::round(positiveTheta * oneOverRadiansPerAngle_) % numAngles_;

  assert(numAngles_ == angles_.size());
  assert(stateTheta >= 0 && stateTheta < angles_.size());
  
  return stateTheta;
}



}
}

#endif
