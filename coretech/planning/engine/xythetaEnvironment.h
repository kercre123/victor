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

class xythetaEnvironment;

// helper class to prevent excess dynamic memory allocation. Additionally, the `Next()` method will 
// evaluate for potential collisions and not produce successors to invalid states
class SuccessorIterator
{
public:
  // helper container for Successor data
  struct Successor
  {
    StateID stateID;    // The successor state
    ActionID actionID;  // The action thats takes you to state
    Cost g;             // Value associated with state  
    Cost penalty;       // Penalty paid to transition into state (not counting normal action cost)
  };

  // if reverse is true, then advance through predecessors instead of successors
  SuccessorIterator(const xythetaEnvironment* env, StateID startID, Cost startG, bool reverse = false);

  // Returns true if there are no more results left
  bool Done(const xythetaEnvironment& env) const;

  // Returns the next action results pair
  inline const Successor& Front() const {return nextSucc_;}

  void Next(const xythetaEnvironment& env);

private:

  State_c    start_c_;
  GraphState start_;
  Cost       startG_;

  // once Next() is called, it will evaluate this action
  ActionID   nextAction_;
  Successor  nextSucc_;
  
  bool reverse_;
};

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
  Cost CheckAndApplyAction(const ActionID& action, StateID& stateID) const;
  
  // Continuous version of CheckAndApplyAction for path segments
  Cost CheckAndApplyPathSegment(const PathSegment& pathSegment,
                        State_c& state_c,
                        float maxPenalty = Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT) const;

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

  // Returns true if there is a fatal collision at the given state
  bool IsInCollision(GraphState s) const;
  bool IsInCollision(State_c c) const;

  bool IsInSoftCollision(GraphState s) const;

  Cost GetCollisionPenalty(GraphState s) const;

  // round the continuous state to the nearest discrete state which is
  // safe. Return true if success, false if all 4 corners are unsafe
  // (unlikely but possible)
  bool RoundSafe(const State_c& c, GraphState& rounded) const;

  // ActionSpace Helpers

  // Get ActionSpace for this environment
  inline const ActionSpace& GetActionSpace() const { return allActions_; }

  inline State_c State2State_c(const GraphState& s) const { return allActions_.State2State_c(s); }

  // returns the raw underlying motion primitive. Note that it is centered at (0,0)
  inline const MotionPrimitive& GetRawMotion(GraphTheta theta, ActionID aID) const { return allActions_.GetForwardMotion(theta, aID); }

  // Get a motion primitive. Returns true if the action is retrieved,
  // false otherwise. Returns primitive in arguments
  inline bool TryGetRawMotion(GraphTheta theta, ActionID aID, MotionPrimitive& prim) const;

private:
  // returns true on success
  bool ParseObstacles(const Json::Value& config);
  void DumpObstacles(Util::JsonWriter& writer) const;

  // class for calculating exact changes in state for given actions
  ActionSpace allActions_;

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
  // NOTE: (mrw) FastPolygon already has the AABB for us...
  std::vector< Bounds > obstacleBounds_;
};


bool xythetaEnvironment::TryGetRawMotion(GraphTheta theta, ActionID aID, MotionPrimitive& prim) const
{
  if(theta < GraphState::numAngles_ && aID < allActions_.GetNumActions()) {
    prim = allActions_.GetForwardMotion(theta, aID);
    return true;
  }
  return false;
}

}
}

#endif
