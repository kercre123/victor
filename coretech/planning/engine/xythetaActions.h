/**
 * File: xythetaActions.h
 *
 * Author: Michael Willett
 * Created: 2018-03-13
 *
 * Description: classes relating to actions including motion primitives
 *
 * Copyright: Anki, Inc. 2014-2018
 *
 **/

#ifndef __PLANNING_BASESTATION_XYTHETA_ACTIONS_H__
#define __PLANNING_BASESTATION_XYTHETA_ACTIONS_H__

#include "coretech/planning/shared/path.h"
#include "xythetaStates.h"

#include <vector>

namespace Anki
{

namespace Util {
class JsonWriter;
}

namespace Planning
{

using ActionID = uint8_t;
using Cost     = float;;

// Note: (mrw) we should consider not-requiring the environment in these classes, as the implication is that all of
// these types exist independant of the world.
class xythetaEnvironment;

// denotes the sequence of Cost-ActionID pairs. It is worth noting that this is distinct from a path, which only
// has a sequence of raw actions (no cost), and is a pure geometric object, while a Plan has no geometry
class xythetaPlan
{
public:
  GraphState start_;

  // add the given plan to the end of this plan
  void Append(const xythetaPlan& other);

  ActionID GetAction(size_t idx)  const              { return _actionCostPairs[idx].first; }
  Cost     GetPenalty(size_t idx) const              { return _actionCostPairs[idx].second; }  
   
  size_t   Size() const                              { return _actionCostPairs.size(); }
  bool     Empty() const                             { return _actionCostPairs.empty(); }
  void     Reverse()                                 { std::reverse(_actionCostPairs.begin(), _actionCostPairs.end()); }
  
  void     Push(ActionID action, Cost penalty = 0.0) { _actionCostPairs.emplace_back(action, penalty); }
  void     Clear()                                   { _actionCostPairs.clear(); }
  
private:
  std::vector<std::pair<ActionID, Cost>> _actionCostPairs;
};

// Helper for quick lookup of information about about different Actions
class ActionType
{
public:
  
  ActionType();

  // returns true if successful
  bool Import(const Json::Value& config);
  void Dump(Util::JsonWriter& writer) const;


  const std::string& GetName() const {return _name;}
  Cost GetExtraCostFactor()    const {return _extraCostFactor;}  
  bool IsReverseAction()       const {return _reverse;}

private:

  Cost        _extraCostFactor;
  ActionID    _id;
  std::string _name;
  bool        _reverse;
};

// MotionPrimivites represent the a generalization edges of the graph to be searched, rather than the classical
// four or eight connected grid. It can contain one or more path segements, as well as a list of intermediate points
// to aid in collision checking. Each motion primitive should be assigned it's own unique ActionID
class MotionPrimitive
{
public:
  // helper struct for importing all points contained along a MotionPrimitive
  struct IntermediatePosition
  {
    IntermediatePosition() : nearestTheta(0), oneOverDistanceFromLastPosition(1.0f) {}
    IntermediatePosition(State_c s, GraphTheta nearestTheta, float d) : position(s), nearestTheta(nearestTheta), oneOverDistanceFromLastPosition(d) {}

    // returns true on success
    bool Import(const Json::Value& config);
    void Dump(Util::JsonWriter& writer) const;

    State_c    position;
    GraphTheta nearestTheta;
    float      oneOverDistanceFromLastPosition;
  };
  
  MotionPrimitive() {}

  // reads raw primitive config and uses the starting angle and environment to compute what is needed
  // Returns true if successful
  bool Create(const Json::Value& config, GraphTheta startingAngle, const xythetaEnvironment& env);

  // returns true if successful, reads a complete config as created by the Dump() call
  bool Import(const Json::Value& config);
  void Dump(Util::JsonWriter& writer) const;

  // returns the minimum PathSegmentOffset associated with this action
  u8 AddSegmentsToPath(State_c start, Path& path) const;

  // id of this action (unique per starting angle)
  ActionID id;

  // The starting angle (discretized). This and the id uniquely define
  // this primitive
  GraphTheta startTheta;

  // Cost of this action
  Cost cost;

  // This is a state where x and y will be added to the current state,
  // but the resulting angle will be set by exactly the theta
  // here. This is possible because we already know the starting theta
  GraphState endStateOffset;

  // vector containing continuous relative offsets for positions in
  // between (0,0,startTheta) and (end)
  std::vector<IntermediatePosition> intermediatePositions;

  // bounding box for everything inside intermediatePositions
  float minX;
  float maxX;
  float minY;
  float maxY;

private:

  // compute min/max x/y
  void CacheBoundingBox(); 

  Path pathSegments_;
};

} // Planning
} // Anki

#endif