/**
 * File: xythetaActions.cpp
 *
 * Author: Michael Willett
 * Created: 2018-03-13
 *
 * Description: classes relating to actions including motion primitives
 *
 * Copyright: Anki, Inc. 2014-2018
 *
 **/

#include "xythetaActions.h"
#include "xythetaEnvironment.h"

#include "util/console/consoleInterface.h"
#include "util/jsonWriter/jsonWriter.h"

#include "coretech/common/shared/radians.h"
#include "coretech/common/engine/jsonTools.h"

#include "json/json.h"


namespace Anki {
namespace Planning {

#define LATTICE_PLANNER_ACCEL 200
#define LATTICE_PLANNER_DECEL 200

#define LATTICE_PLANNER_ROT_ACCEL 10
#define LATTICE_PLANNER_ROT_DECEL 10
  
#define LATTICE_PLANNER_POINT_TURN_TOL DEG_TO_RAD(2)
  
#define LATTICE_PLANNER_MAX_LINE_LENGTH 50

CONSOLE_VAR(f32, kXYTPlanner_PointTurnTolOverride_deg, "Planner", 2.0f);

void xythetaPlan::Append(const xythetaPlan& other)
{
  if(_actionCostPairs.empty()) {
    start_ = other.start_;
  }
  _actionCostPairs.insert(_actionCostPairs.end(), other._actionCostPairs.begin(), other._actionCostPairs.end() );
}

bool MotionPrimitive::IntermediatePosition::Import(const Json::Value& config)
{
  try {
    if( config.isNull() ) {
      PRINT_NAMED_ERROR("IntermediatePosition.Import.Null", "config value is null");
      return false;
    }

    if( ! position.Import(config["position"]) ) {
      return false;
    }
  
    nearestTheta = config["theta"].asInt();
    oneOverDistanceFromLastPosition = config["inverseDist"].asFloat();
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("IntermediatePosition.Import.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }


  return true;
}

void  MotionPrimitive::IntermediatePosition::Dump(Util::JsonWriter& writer) const
{
  writer.StartGroup("position");
  position.Dump(writer);
  writer.EndGroup();

  writer.AddEntry("theta", nearestTheta);
  writer.AddEntry("inverseDist", oneOverDistanceFromLastPosition);
}

ActionType::ActionType()
  : _extraCostFactor(0.0)
  , _id(-1)
  , _name("<invalid>")
  , _reverse(false)
{
}

bool ActionType::Import(const Json::Value& config)
{
  try {
    if( config.isNull() ) {
      PRINT_NAMED_ERROR("ActionType.Import.Null", "config value is null");
      return false;
    }

    if(!JsonTools::GetValueOptional(config, "extra_cost_factor", _extraCostFactor) ||
       !JsonTools::GetValueOptional(config, "index", _id) ||
       !JsonTools::GetValueOptional(config, "name", _name)) {
      printf("error: could not parse ActionType\n");
      JsonTools::PrintJsonCout(config, 1);
      return false;
    }
    JsonTools::GetValueOptional(config, "reverse_action", _reverse);
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("ActionType.Import.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }

  return true;
}

void ActionType::Dump(Util::JsonWriter& writer) const
{
  writer.AddEntry("extra_cost_factor", _extraCostFactor);
  writer.AddEntry("index", _id);
  writer.AddEntry("name", _name);
  writer.AddEntry("reverse_action", _reverse);
}


bool MotionPrimitive::Import(const Json::Value& config)
{
  try {
    if( config.isNull() ) {
      return false;
    }

    if( config["action_index"].isNull() ) {
      PRINT_NAMED_ERROR("MotionPrimitive.Import.InvalidConfig",
                        "no action_index in config. dump follows");
      JsonTools::PrintJsonCout(config, 3);
      return false;
    }

    if( config["cost"].isNull() ) {
      PRINT_NAMED_ERROR("MotionPrimitive.Import.InvalidConfig2",
                        "missing 'cost' key. Did you mean to call Create() instead of Import()?");
      JsonTools::PrintJsonCout(config, 3);
      return false;
    }

    id = config["action_index"].asInt();
    startTheta = config["start_theta"].asInt();
    cost = config["cost"].asFloat();
    if( ! endStateOffset.Import(config["end_state_offset"]) ) {
      return false;
    }
  
    if(config["intermediate_poses"].size() <= 1 ) {
      PRINT_NAMED_ERROR("MotionPrimitive.Import.InvalidConfig3",
                        "'intermediate_poses' size %d too small (or not a list). Dump follows",
                        config["intermediate_poses"].size());
      JsonTools::PrintJsonCout(config["intermediate_poses"], 3);
      return false;
    }

    intermediatePositions.clear();

    for(const auto& poseConfig : config["intermediate_poses"]) {
      IntermediatePosition p;
      if( ! p.Import( poseConfig ) ) {
        return false;
      }
      intermediatePositions.push_back(p);
    }
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("MotionPrimitive.Import.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }

  CacheBoundingBox();

  return true;
}

void MotionPrimitive::CacheBoundingBox()
{
  minX = std::numeric_limits<decltype(minX)>::max();
  maxX = std::numeric_limits<decltype(maxX)>::min();
  minY = std::numeric_limits<decltype(minY)>::max();
  maxY = std::numeric_limits<decltype(maxY)>::min();

  for( const auto& pt : intermediatePositions ) {
    if( pt.position.x_mm < minX ) {
      minX = pt.position.x_mm;
    }
    if( pt.position.x_mm > maxX ) {
      maxX = pt.position.x_mm;
    }

    if( pt.position.y_mm < minY ) {
      minY = pt.position.y_mm;
    }
    if( pt.position.y_mm > maxY ) {
      maxY = pt.position.y_mm;
    }
  }
}

void MotionPrimitive::Dump(Util::JsonWriter& writer) const
{
  writer.AddEntry("action_index", id);
  writer.AddEntry("start_theta", startTheta);
  writer.AddEntry("cost", cost);

  writer.StartGroup("end_state_offset");
  endStateOffset.Dump(writer);
  writer.EndGroup();

  writer.StartList("intermediate_poses");
  for(const auto& pose : intermediatePositions) {
    writer.NextListItem();
    pose.Dump(writer);
  }
  writer.EndList();
}


u8 MotionPrimitive::AddSegmentsToPath(State_c start, Path& path) const
{
  State_c curr(start);

  bool added = false;
  u8 firstSegment = path.GetNumSegments();

  for(u8 pathIdx = 0; pathIdx < pathSegments_.GetNumSegments(); ++pathIdx) {
    const PathSegment& seg(pathSegments_.GetSegmentConstRef(pathIdx));
    PathSegment segment(seg);
    segment.OffsetStart(curr.x_mm, curr.y_mm);

#if REMOTE_CONSOLE_ENABLED
    if( segment.GetType() == PST_POINT_TURN) {
      segment.GetDef().turn.angleTolerance = DEG_TO_RAD(kXYTPlanner_PointTurnTolOverride_deg);
    }
#endif


    float xx, yy, aa;
    segment.GetStartPoint(xx,yy);
    // printf("start: (%f, %f)\n", xx, yy);

    // if this segment can be combined with the previous one, do
    // that. otherwise, append a new segment.
    bool shouldAdd = true;
    if(path.GetNumSegments() > 0 && path[path.GetNumSegments()-1].GetType() == segment.GetType()) {
      size_t endIdx = path.GetNumSegments()-1;

      switch(segment.GetType()) {

      case PST_LINE:
      {
        bool oldSign = path[path.GetNumSegments()-1].GetTargetSpeed() > 0;
        bool newSign = segment.GetTargetSpeed() > 0;
        if(oldSign == newSign) {
          const float dx = (segment.GetDef().line.endPt_x - path[endIdx].GetDef().line.startPt_x);
          const float dy = (segment.GetDef().line.endPt_y - path[endIdx].GetDef().line.startPt_y);
          if( dx*dx + dy*dy <= LATTICE_PLANNER_MAX_LINE_LENGTH*LATTICE_PLANNER_MAX_LINE_LENGTH ) {
            path[endIdx].GetDef().line.endPt_x = segment.GetDef().line.endPt_x;
            path[endIdx].GetDef().line.endPt_y = segment.GetDef().line.endPt_y;
            shouldAdd = false;
          }
        }
        break;
      }

      case PST_ARC:
        // TODO:(bn) had to disable this because it was combing arcs
        // that the robot was going to split up. This doesn't happen
        // in the lattice planner anyway (its always line, arc for
        // each turn action)

        // if(FLT_NEAR(path[endIdx].GetDef().arc.centerPt_x, segment.GetDef().arc.centerPt_x) &&
        //        FLT_NEAR(path[endIdx].GetDef().arc.centerPt_y, segment.GetDef().arc.centerPt_y) &&
        //        FLT_NEAR(path[endIdx].GetDef().arc.radius, segment.GetDef().arc.radius)) {

        //   path[endIdx].GetDef().arc.sweepRad += segment.GetDef().arc.sweepRad;
        //   shouldAdd = false;
        // }
        break;

      case PST_POINT_TURN:
        // only combine point turns if they are the same and the new
        // target angle is less that 180 degrees away from the current
        // angle
        if(FLT_NEAR(path[endIdx].GetDef().turn.x, segment.GetDef().turn.x) &&
           FLT_NEAR(path[endIdx].GetDef().turn.y, segment.GetDef().turn.y) &&
           FLT_NEAR(path[endIdx].GetTargetSpeed(), segment.GetTargetSpeed())) {
          path[endIdx].GetDef().turn.targetAngle = segment.GetDef().turn.targetAngle;
          shouldAdd = false;
        }
        break;

      default:
        printf("ERROR (AddSegmentsToPath): Undefined segment %d\n", segment.GetType());
        assert(false);
      }
    }

    if(shouldAdd) {
      path.AppendSegment(segment);
      added = true;
    }

    segment.GetEndPose(xx,yy,aa);
    // printf("end: (%f, %f)\n", xx, yy);
  }

  if(!added && firstSegment > 0)
    firstSegment--;

  return firstSegment;
}


bool MotionPrimitive::Create(const Json::Value& config, GraphTheta startingAngle, const ActionSpace& env)
{
  startTheta = startingAngle;

  if(!JsonTools::GetValueOptional(config, "action_index", id)) {
    printf("error: missing key 'action_index'\n");
    JsonTools::PrintJsonCout(config, 1);
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
    float penalty = 0.0;

    if(!intermediatePositions.empty()) {
      old = intermediatePositions.back().position;

      float dist = State_c::GetDistanceBetween(old, s);

      // TODO:(bn) use actual time / cost computation!
      float cost = dist;

      Radians deltaTheta = Radians(s.theta) - Radians(old.theta);
      cost += std::abs(deltaTheta.ToFloat()) *
        env.GetHalfWheelBase_mm() *
        env.GetOneOverMaxVelocity();
      penalty = 1.0 / cost;
    }

    intermediatePositions.emplace_back(s, s.GetGraphTheta(),  penalty);
  }

  if(config.isMember("extra_cost_factor")) {
    printf("ERROR: individual primitives shouldn't have cost factors. Old file format?\n");
    return false;
  }

  double linearSpeed = env.GetMaxVelocity_mmps();
  double oneOverLinearSpeed = env.GetOneOverMaxVelocity();
  bool isReverse = env.GetActionType(id).IsReverseAction();
  if(isReverse) {
    linearSpeed = env.GetMaxReverseVelocity_mmps();
    oneOverLinearSpeed = 1.0 / env.GetMaxReverseVelocity_mmps();
  }

  // Compute cost based on the action. Cost is time in seconds
  cost = 0.0;

#ifdef HACK_USE_FIXED_SPEED
  linearSpeed = HACK_USE_FIXED_SPEED;
  oneOverLinearSpeed = 1.0 / linearSpeed;
#endif

  double length = std::abs(config["straight_length_mm"].asDouble());
  if(length > 0.0) {
    cost += length * oneOverLinearSpeed;

    float signedLength = config["straight_length_mm"].asFloat();
    
    if(std::abs(signedLength) > 0.001) {
      pathSegments_.AppendLine(0.0,
                               0.0,
                               signedLength * cos(env.LookupTheta(startingAngle)),
                               signedLength * sin(env.LookupTheta(startingAngle)),
                               isReverse ? -linearSpeed : linearSpeed,
                               LATTICE_PLANNER_ACCEL,
                               LATTICE_PLANNER_DECEL);
    }
  }

  if(config.isMember("arc")) {
    // the section of the angle we will sweep through
    double deltaTheta = std::abs(config["arc"]["sweepRad"].asDouble());

    // the radius of the circle that the outer wheel will follow
    double turningRadius = std::abs(config["arc"]["radius_mm"].asDouble());
    double radius_mm = turningRadius + env.GetHalfWheelBase_mm();

    // the total time is the arclength of the outer wheel arc divided by the max outer wheel speed
    Cost arcTime = deltaTheta * radius_mm * oneOverLinearSpeed;
    cost += arcTime;

    Cost arcSpeed = deltaTheta * turningRadius / arcTime;

    // TODO:(bn) these don't work properly backwards at the moment

#ifdef HACK_USE_FIXED_SPEED
    arcSpeed = HACK_USE_FIXED_SPEED;
#endif

    pathSegments_.AppendArc(config["arc"]["centerPt_x_mm"].asFloat(),
                            config["arc"]["centerPt_y_mm"].asFloat(),
                            config["arc"]["radius_mm"].asFloat(),
                            config["arc"]["startRad"].asFloat(),
                            config["arc"]["sweepRad"].asFloat(),
                            isReverse ? -arcSpeed : arcSpeed,
                            LATTICE_PLANNER_ACCEL,
                            LATTICE_PLANNER_DECEL);
  }
  else if(config.isMember("turn_in_place_direction")) {
    double direction = config["turn_in_place_direction"].asDouble();

    // turn in place is just like an arc with radius 0
    Radians startRads(env.LookupTheta(startTheta));
    double deltaTheta = startRads.angularDistance(env.LookupTheta(endStateOffset.theta), direction < 0);

    Cost turnTime = std::abs(deltaTheta) * env.GetHalfWheelBase_mm() * oneOverLinearSpeed;
    cost += turnTime;

    float rotSpeed = deltaTheta / turnTime;

    pathSegments_.AppendPointTurn(0.0,
                                  0.0,
                                  startRads.ToFloat(),
                                  env.LookupTheta(endStateOffset.theta),
                                  rotSpeed,
                                  LATTICE_PLANNER_ROT_ACCEL,
                                  LATTICE_PLANNER_ROT_DECEL,
                                  LATTICE_PLANNER_POINT_TURN_TOL,
                                  true);
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

  CacheBoundingBox();

  return true;
}

bool ActionSpace::ParseMotionPrims(const Json::Value& config, bool useDumpFormat)
{
  try {
    float configResolution = 0.f;
    unsigned int configAngles = 0;
    if(!JsonTools::GetValueOptional(config, "resolution_mm", configResolution) ||
       !JsonTools::GetValueOptional(config, "num_angles", configAngles)) {
      printf("error: could not find key 'resolution_mm' or 'num_angles' in motion primitives\n");
      JsonTools::PrintJsonCout(config, 1);
      return false;
    }

    assert(GraphState::resolution_mm_ == configResolution);
    assert(GraphState::numAngles_ == configAngles);

    // parse through the action types
    if(config["actions"].size() == 0) {
      printf("empty or non-existant actions section! (old format, perhaps?)\n");
      return false;
    }

    for(const auto & actionConfig : config["actions"]) {
      ActionType at;
      at.Import(actionConfig);
      _actionTypes.push_back(at);
    }

    try {
      for(const auto & angle : config["angle_definitions"]) {
        _angles.push_back(angle.asFloat());
      }
    }
    catch( const std::exception&  e ) {
      PRINT_NAMED_ERROR("ParseMotionPrims.angle_definitions.Exception",
                        "json exception: %s",
                        e.what());
      return false;
    }

    if(_angles.size() != configAngles) {
      printf("ERROR: numAngles is %u, but we read %lu angle definitions\n",
             configAngles,
             (unsigned long)_angles.size());
      return false;
    }

    // parse through each starting angle
    if(config["angles"].size() != configAngles) {
      printf("error: could not find key 'angles' in motion primitives\n");
      JsonTools::PrintJsonCout(config, 1);
      return false;
    }

    unsigned int numPrims = 0;

    try {
      for(unsigned int angle = 0; angle < configAngles; ++angle) {
        Json::Value prims = config["angles"][angle]["prims"];
        for(unsigned int i = 0; i < prims.size(); ++i) {
          MotionPrimitive p;

          if( useDumpFormat ) {
            if( ! p.Import(prims[i]) ) {
              return false;
            }
          }
          else {
            if(!p.Create(prims[i], angle, *this)) {
              PRINT_NAMED_ERROR("ParseMotionPrims.CreateFormat.Mprim", "Failed to import motion primitive");
              return false;
            }
          }

          _forwardPrims[angle].push_back(p);
          numPrims++;
        }
      }
    }
    catch( const std::exception&  e ) {
      PRINT_NAMED_ERROR("ParseMotionPrims.anglesPrims.Exception",
                        "json exception: %s",
                        e.what());
      return false;
    }

    PRINT_NAMED_INFO("ParseMotionPrims.Added", "Added %d motion primitives", numPrims);
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("ParseMotionPrims.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }
  
  PopulateReversePrims();
  return true;
}

void ActionSpace::DumpMotionPrims(Util::JsonWriter& writer) const
{
  writer.AddEntry("resolution_mm", GraphState::resolution_mm_);
  writer.AddEntry("num_angles", (int)GraphState::numAngles_);

  writer.StartList("actions");
  for(const auto& action : _actionTypes) {
    writer.NextListItem();
    action.Dump(writer);
  }
  writer.EndList();

  writer.StartList("angle_definitions");
  for(const auto& angleDef : _angles) {
    writer.AddRawListEntry(angleDef);
  }
  writer.EndList();

  writer.StartList("angles");
  for(unsigned int angle = 0; angle < GraphState::numAngles_; ++angle) {
    writer.NextListItem();
    writer.StartList("prims");
    for(const auto& prim : _forwardPrims[angle]) {
      writer.NextListItem();
      prim.Dump(writer);
    }
    writer.EndList();
  }
  writer.EndList();  

  writer.StartGroup("robotParams");
  _robotParams.Dump(writer);
  writer.EndGroup(); 
}

void ActionSpace::PopulateReversePrims()
{
  // go through each motion primitive, and populate the corresponding reverse primitive
  for(int startAngle = 0; startAngle < GraphState::numAngles_; ++startAngle) {
    for(int actionID = 0; actionID < _forwardPrims[ startAngle ].size(); ++actionID) {
      const MotionPrimitive& prim( _forwardPrims[ startAngle ][ actionID ] );
      int endAngle = prim.endStateOffset.theta;

      MotionPrimitive reversePrim(prim);
      reversePrim.endStateOffset.theta = startAngle;
      reversePrim.endStateOffset.x = -reversePrim.endStateOffset.x;
      reversePrim.endStateOffset.y = -reversePrim.endStateOffset.y;

      _reversePrims[endAngle].push_back( reversePrim );
    }
  }
}

void ActionSpace::PrintPlan(const xythetaPlan& plan) const
{
  State_c curr_c = State2State_c(plan.start_);

  PRINT_STREAM_DEBUG("xythetaEnvironment.PrintPlan", "plan start: " << plan.start_);

  for(size_t i=0; i<plan.Size(); ++i) {
    PRINT_NAMED_DEBUG("xythetaEnvironment.PrintPlan", "%2lu: (%f, %f, %f [%d]) --> %s (penalty = %f)",
           (unsigned long)i,
           curr_c.x_mm, curr_c.y_mm, curr_c.theta, curr_c.GetGraphTheta(), 
           _actionTypes[plan.GetAction(i)].GetName().c_str(),
           plan.GetPenalty(i));
    ApplyAction(plan.GetAction(i), curr_c);
  }
}

void ActionSpace::ApplyAction(ActionID action, State_c& state) const
{
  const MotionPrimitive* prim = & GetForwardMotion(state.GetGraphTheta(), action);

  state.x_mm += prim->endStateOffset.x;
  state.y_mm += prim->endStateOffset.y;
  state.theta = prim->endStateOffset.theta;
}

void ActionSpace::ApplyAction(ActionID action, GraphState& state) const
{
  const MotionPrimitive* prim = &GetForwardMotion(state.theta, action);

  state.x += prim->endStateOffset.x;
  state.y += prim->endStateOffset.y;
  state.theta = prim->endStateOffset.theta;
}

void ActionSpace::ApplyAction(ActionID action, StateID& stateID) const
{
  GraphState state(stateID);
  const MotionPrimitive* prim = &GetForwardMotion(state.theta, action);

  state.x += prim->endStateOffset.x;
  state.y += prim->endStateOffset.y;
  state.theta = prim->endStateOffset.theta;

  stateID = state.GetStateID();
}

GraphState ActionSpace::GetPlanFinalState(const xythetaPlan& plan) const
{
  StateID currID = plan.start_.GetStateID();

  for(size_t i = 0; i < plan.Size(); i++) {
    const ActionID& action = plan.GetAction(i);
    ApplyAction(action, currID);
  }

  return GraphState(currID);
}

void ActionSpace::AppendToPath(xythetaPlan& plan, Path& path, int numActionsToSkip) const
{
  GraphState curr = plan.start_;

  int actionsLeftToSkip = numActionsToSkip;

  for (size_t i = 0; i < plan.Size(); ++i) {
    const ActionID& actionID = plan.GetAction(i);
    
    if(curr.theta >= GraphState::numAngles_ || actionID >= GetNumActions()) {
      printf("ERROR: can't look up prim for angle %d and action id %d\n", curr.theta, actionID);
      break;
    }

    const MotionPrimitive* prim = &GetForwardMotion(curr.theta, actionID);

    if( actionsLeftToSkip == 0 ) {
      prim->AddSegmentsToPath(State2State_c(curr), path);
    }
    else {
      actionsLeftToSkip--;
    }

    curr.x += prim->endStateOffset.x;
    curr.y += prim->endStateOffset.y;
    curr.theta = prim->endStateOffset.theta;
  }
}

void ActionSpace::ConvertToXYPlan(const xythetaPlan& plan, std::vector<State_c>& continuousPlan) const
{
  continuousPlan.clear();

  State_c curr_c = State2State_c(plan.start_);
  GraphTheta currTheta = plan.start_.theta;
  // TODO:(bn) replace theta with radians? maybe just cast it here

  for(size_t i=0; i<plan.Size(); ++i) {
    printf("curr = (%f, %f, %f [%d]) : %s\n", curr_c.x_mm, curr_c.y_mm, curr_c.theta, currTheta, 
               _actionTypes[plan.GetAction(i)].GetName().c_str());

    if(currTheta >= GraphState::numAngles_ || plan.GetAction(i) >= GetNumActions()) {
      printf("ERROR: can't look up prim for angle %d and action id %d\n", currTheta, plan.GetAction(i));
      break;
    }
    else {
      const MotionPrimitive* prim = &GetForwardMotion(currTheta, plan.GetAction(i));
      for(size_t j=0; j<prim->intermediatePositions.size(); ++j) {
        float x = curr_c.x_mm + prim->intermediatePositions[j].position.x_mm;
        float y = curr_c.y_mm + prim->intermediatePositions[j].position.y_mm;
        float theta = prim->intermediatePositions[j].position.theta;

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

} // Planning
} // Anki
