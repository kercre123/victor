#ifndef _ANKICORETECH_PLANNING_XYTHETA_ENVIRONMENT_H_
#define _ANKICORETECH_PLANNING_XYTHETA_ENVIRONMENT_H_

#include "coretech/common/engine/math/fastPolygon2d.h"
#include "coretech/common/engine/math/quad.h"
#include "coretech/planning/engine/robotActionParams.h"
#include "coretech/planning/shared/path.h"
#include "json/json-forwards.h"
#include "xythetaActions.h"
#include "util/math/math.h"
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


#define INVALID_ACTION_ID 255

#define MAX_OBSTACLE_COST 1000.0f
#define FATAL_OBSTACLE_COST (MAX_OBSTACLE_COST + 1.0f)
#define REVERSE_OVER_OBSTACLE_COST 0.0f


class xythetaEnvironment
{
  friend class SuccessorIterator;
  friend class MotionPrimitive;
public:
  // for testing, has no prims or map!
  xythetaEnvironment();

  ~xythetaEnvironment();

  // returns true if everything worked
  bool Init(const char* mprimFilename);

  // inits with motion primitives and an empty environment
  bool Init(const Json::Value& mprimJson);

  // Imports motion primitives from the given json file. Returns true if success
  bool ReadMotionPrimitives(const char* mprimFilename);

  // Dumps the entire environment to JSON. after dumping, deleting, then importing, the obstacles should be
  // identical. Does NOT dump motion primitives
  void Dump(Util::JsonWriter& writer) const;

  // Imports the environment from the config. Returns true on success, must have matching motion primitives to
  // work
  bool Import(const Json::Value& config);

  // Adds the same obstacle to all thetas (doesn't really make any physical sense, basically assumes a point
  // robot. useful for unit tests) defaults to a fatal obstacle
  void AddObstacleAllThetas(const RotatedRectangle& rect, Cost cost = FATAL_OBSTACLE_COST);
  void AddObstacleAllThetas(const Quad2f& quad, Cost cost = FATAL_OBSTACLE_COST);
 
  // returns a polygon which represents the obstacle expanded to the
  // c-space of robot, where the origin of tobot is (0,0)
  static FastPolygon ExpandCSpace(const ConvexPolygon& obstacle,
                                  const ConvexPolygon& robot);

  // adds an obstacle into the c-space map for the given angle with
  // the given robot footprint, centered at (0,0). Returns reference
  // to the polygon it inserted
  const FastPolygon& AddObstacleWithExpansion(const ConvexPolygon& obstacle,
                                              const ConvexPolygon& robot,
                                              GraphTheta theta,
                                              Cost cost = FATAL_OBSTACLE_COST);

  void ClearObstacles();

  size_t GetNumObstacles() const;

  // Returns an iterator to the successors from state "start". Use this one if you want to check each
  // action. If reverse is true, then get predecessors (reverse successors) instead
  SuccessorIterator GetSuccessors(StateID startID, Cost currG, bool reverse = false) const;

  // This function tries to apply the given action to the state. It
  // returns the penalty of the path (i.e. no cost for actions, just
  // obstacle penalties). If the action finishes without a fatal
  // penalty, stateID will be updated to the successor state
  Cost ApplyAction(const ActionID& action, StateID& stateID, bool checkCollisions = true) const;
  
  // Continuous version of ApplyAction for path segments
  Cost ApplyPathSegment(const PathSegment& pathSegment,
                        State_c& state_c,
                        bool checkCollisions = true,
                        float maxPenalty = Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT) const;

  // Returns the state at the end of the given plan (e.g. following
  // along the plans start and executing every action). No collision
  // checks are performed
  GraphState GetPlanFinalState(const xythetaPlan& plan) const;


  // This essentially projects the given pose onto the plan (held in this member). The projection is just the
  // closest euclidean distance point on the plan, and the return value is the number of complete plan actions
  // that are finished by the time you get to this point. Returns the best distance in argument, with the
  // distance set to 0.0 if it seems to be within the discretization error of the planner
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
  // Returns true if segments in path comprise a safe and complete plan. Clears and fills in a list of
  // path segments whose cumulative penalty doesnt exceed the max penalty. Unlike PlanIsSafe, does
  // not include action penalties along the path, because Path doesnt have that whereas a plan does
  bool PathIsSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const;

  // If we are going to be doing a full planner cycle, this function
  // will be called to prepare the environment, including
  // precomputing things, etc.
  void PrepareForPlanning();

  // returns the raw underlying motion primitive. Note that it is centered at (0,0)
  const MotionPrimitive& GetRawMotionPrimitive(GraphTheta theta, ActionID action) const;

  // Returns true if there is a fatal collision at the given state
  bool IsInCollision(GraphState s) const;
  bool IsInCollision(State_c c) const;

  bool IsInSoftCollision(GraphState s) const;

  Cost GetCollisionPenalty(GraphState s) const;

  inline State_c State2State_c(const GraphState& s) const;
  inline State_c StateID2State_c(StateID sid) const;

  // round the continuous state to the nearest discrete state, ignoring obstacles
  inline GraphState State_c2State(const State_c& c) const;

  // round the continuous state to the nearest discrete state which is
  // safe. Return true if success, false if all 4 corners are unsafe
  // (unlikely but possible)
  bool RoundSafe(const State_c& c, GraphState& rounded) const;

  unsigned int GetNumAngles() const {return numAngles_;}

  inline static float GetXFromStateID(StateID sid);
  inline static float GetYFromStateID(StateID sid);
  inline static float GetThetaFromStateID(StateID sid);

  inline float GetX_mm(GraphXY x) const;
  inline float GetY_mm(GraphXY y) const;
  inline float GetTheta_c(GraphTheta theta) const;

  inline GraphXY GetX(float x_mm) const;
  inline GraphXY GetY(float y_mm) const;
  inline GraphTheta GetTheta(float theta_rad) const;

  float GetDistanceBetween(const State_c& start, const GraphState& end) const;
  static float GetDistanceBetween(const State_c& start, const State_c& end);

  float GetMinAngleBetween(const State_c& start, const GraphState& end) const;
  static float GetMinAngleBetween(const State_c& start, const State_c& end);

  // Get a motion primitive. Returns true if the action is retrieved,
  // false otherwise. Returns primitive in arguments
  inline bool GetMotion(GraphTheta theta, ActionID actionID, MotionPrimitive& prim) const;

  inline size_t GetNumActions() const { return actionTypes_.size(); }
  
  inline const ActionType& GetActionType(ActionID aid) const { return actionTypes_[aid]; }

  // This function fills up the given vector with (x,y,theta) coordinates of
  // following the plan.
  void ConvertToXYPlan(const xythetaPlan& plan, std::vector<State_c>& continuousPlan) const;

  void PrintPlan(const xythetaPlan& plan) const;

  // Convert the plan to Planning::PathSegment's and append it to path. Also updates plan to set the
  // robotPathSegmentIdx_ If skipActions is nonzero, then that many actions from plan will be skipped (not
  // copied into path), but the state will still be correctly updated
  void AppendToPath(xythetaPlan& plan, Path& path, int numActionsToSkip = 0) const;

  double GetMaxVelocity_mmps() const {return _robotParams.maxVelocity_mmps;}
  double GetMaxReverseVelocity_mmps() const {return _robotParams.maxReverseVelocity_mmps;}
  double GetOneOverMaxVelocity() const {return _robotParams.oneOverMaxVelocity;}
  double GetHalfWheelBase_mm() const {return _robotParams.halfWheelBase_mm;}

  float GetResolution_mm() const { return resolution_mm_; }

private:

  // returns true on success
  bool ParseObstacles(const Json::Value& config);
  void DumpObstacles(Util::JsonWriter& writer) const;

  // These are deprecated! Consider removing them. They could come in handy again if we need to dump the fully
  // parsed-out mprims
  void DumpMotionPrims(Util::JsonWriter& writer) const;
  bool ParseMotionPrims(const Json::Value& config, bool useDumpFormat = false);

  void PopulateReverseMotionPrims();

  float resolution_mm_;
  float oneOverResolution_;

  unsigned int numAngles_;

  // NOTE: these are approximate. The actual angles aren't evenly
  // distrubuted, but this will get you close
  float radiansPerAngle_;
  float oneOverRadiansPerAngle_;

  // First index is starting angle, second is prim ID
  std::vector< std::vector<MotionPrimitive> > allMotionPrimitives_;

  // Reverse motion primitives, for getting predecessors to a state. The first index is the ending angle, the
  // second is meaningless. The primitive will have the action ID corresponding to the correct forward
  // primitive, but the endStateOffset will be reverse, so you can follow backwards
  std::vector< std::vector<MotionPrimitive> > reverseMotionPrimitives_;

  // index is actionID
  std::vector<ActionType> actionTypes_;

  // index is GraphTheta, value is theta in radians, between -pi and
  // pi. If there are more than 8 angles, they are not perfectly
  // evenly divided (see docs)  // TODO:(bn) write docs....
  std::vector<float> angles_;

  // Obstacles per theta. First index is theta, second of pair is cost
  std::vector< std::vector< std::pair<FastPolygon, Cost> > > obstaclesPerAngle_;

  struct Bounds {
    Bounds()
      : minX(FLT_MAX)
      , maxX(FLT_MIN)
      , minY(FLT_MAX)
      , maxY(FLT_MIN)
      {
      }

    float minX;
    float maxX;
    float minY;
    float maxY;
  };

  // one per obstacle, this is the bounding box that will bound that obstacle at every angle. If you are clear
  // of all of these, then you can skip the check. Order is the same as the obstacle order within the angles
  // in obstaclesPerAngle_
  std::vector< Bounds > obstacleBounds_;

  // TODO:(bn) this won't support multiple different robots!
  const RobotActionParams _robotParams;
};

bool xythetaEnvironment::GetMotion(GraphTheta theta, ActionID actionID, MotionPrimitive& prim) const
{
  if(theta < allMotionPrimitives_.size() && actionID < allMotionPrimitives_[theta].size()) {
    prim = allMotionPrimitives_[theta][actionID];
    return true;
  }
  return false;
}

// TODO:(bn) pull out into _inline.cpp
State_c xythetaEnvironment::State2State_c(const GraphState& s) const
{
  return State_c(GetX_mm(s.x), GetY_mm(s.y), GetTheta_c(s.theta));
}

GraphState xythetaEnvironment::State_c2State(const State_c& c) const
{
  return GraphState(GetX(c.x_mm), GetY(c.y_mm), GetTheta(c.theta));
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

float xythetaEnvironment::GetX_mm(GraphXY x) const
{
  return resolution_mm_ * x;
}

float xythetaEnvironment::GetY_mm(GraphXY y) const
{
  return resolution_mm_ * y;
}

float xythetaEnvironment::GetTheta_c(GraphTheta theta) const
{
  assert(theta >= 0 && theta < angles_.size());
  return angles_[theta];
}

GraphXY xythetaEnvironment::GetX(float x_mm) const
{
  return (GraphXY) roundf(x_mm * oneOverResolution_);
}

GraphXY xythetaEnvironment::GetY(float y_mm) const
{
  return (GraphXY) roundf(y_mm * oneOverResolution_);
}

GraphTheta xythetaEnvironment::GetTheta(float theta_rad) const
{
  float positiveTheta = theta_rad;
  // TODO:(bn) something faster (std::remainder ??)
  while(positiveTheta < 0.0) {
    positiveTheta += 2*M_PI;
  }
  
  while(positiveTheta >= 2*M_PI) {
    positiveTheta -= 2*M_PI;
  }
  
  const GraphTheta theta = (GraphTheta) std::round(positiveTheta * oneOverRadiansPerAngle_) % numAngles_;

  assert(numAngles_ == angles_.size());
  assert(theta >= 0 && theta < angles_.size());
  
  return theta;
}



}
}

#endif
