// read note in header about commented out code

///**
// * File: behaviorExploreMarkedCube
// *
// * Author: Raul
// * Created: 01/22/16
// *
// * Description: Behavior that looks for a nearby marked cube that Cozmo has not fully explored (ie: seen in
// * all directions), and tries to see the sides that are yet to be discovered.
// *
// * Copyright: Anki, Inc. 2016
// *
// **/
//#include "behaviorExploreMarkedCube.h"
//
//#include "anki/cozmo/basestation/actions/driveToActions.h"
//#include "anki/cozmo/basestation/robot.h"
//
//#include "util/console/consoleInterface.h"
//#include "util/logging/logging.h"
//#include "util/math/numericCast.h"
//#include "util/random/randomGenerator.h"
//
//namespace Anki {
//namespace Cozmo {
//  
//CONSOLE_VAR(uint8_t, kEmcMaxBorderGoals, "BehaviorExploreMarkedCube", 1); // max number of goals to ask the planner
//CONSOLE_VAR(float, kEmcDistanceMinFactor, "BehaviorExploreMarkedCube", 2.0f); // minimum factor applied to the robot size to find destination from border center
//CONSOLE_VAR(float, kEmcDistanceMaxFactor, "BehaviorExploreMarkedCube", 4.0f); // maximum factor applied to the robot size to find destination from border center
//CONSOLE_VAR(bool, kEmcDrawDebugInfo, "BehaviorExploreMarkedCube", false); // if set to true the behavior renders debug privimitives
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//namespace {
//
//// This is the configuration of memory map types that this behavior checks against. If a type
//// is set to true, it means that if they are a border to a marked cube, we will explore that marked cube
//constexpr NavMemoryMapTypes::FullContentArray typesToExploreFrom =
//{
//  {NavMemoryMapTypes::EContentType::Unknown               , true },
//  {NavMemoryMapTypes::EContentType::ClearOfObstacle       , false},
//  {NavMemoryMapTypes::EContentType::ClearOfCliff          , false},
//  {NavMemoryMapTypes::EContentType::ObstacleCube          , false},
//  {NavMemoryMapTypes::EContentType::ObstacleCubeRemoved   , false},
//  {NavMemoryMapTypes::EContentType::ObstacleCharger       , false},
//  {NavMemoryMapTypes::EContentType::ObstacleChargerRemoved, false},
//  {NavMemoryMapTypes::EContentType::ObstacleUnrecognized  , false},
//  {NavMemoryMapTypes::EContentType::Cliff                 , false},
//  {NavMemoryMapTypes::EContentType::InterestingEdge       , false},
//  {NavMemoryMapTypes::EContentType::NotInterestingEdge    , false}
//};
//static_assert(NavMemoryMapTypes::IsSequentialArray(typesToExploreFrom),
//  "This array does not define all types once and only once.");
//  
//};
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//BehaviorExploreMarkedCube::BehaviorExploreMarkedCube(Robot& robot, const Json::Value& config)
//: IBehavior(robot, config)
//, _currentActionTag(ActionConstants::INVALID_TAG)
//{
//
//  SubscribeToTags({
//    EngineToGameTag::RobotCompletedAction
//  });
//  
//}
//  
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//BehaviorExploreMarkedCube::~BehaviorExploreMarkedCube()
//{  
//}
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//bool BehaviorExploreMarkedCube::IsRunnableInternal(const Robot& robot) const
//{
//  const INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
//  if ( nullptr == memoryMap ) {
//    return false;
//  }
//  const bool hasBorders = memoryMap->HasBorders(NavMemoryMapTypes::EContentType::ObstacleCube, typesToExploreFrom);
//  return hasBorders;
//}
//  
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//Result BehaviorExploreMarkedCube::InitInternal(Robot& robot)
//{
//  // select borders we want to visit
//  BorderScoreVector borderGoals;
//  PickGoals(robot, borderGoals);
//
//  // for every goal, pick a target point to look at the cube from there. Those are the goals we will feed the planner
//  GenerateVantagePoints(robot, borderGoals, _currentVantagePoints);
//
//  // debugging
//  if ( kEmcDrawDebugInfo )
//  {
//    // border goals
//    robot.GetContext()->GetVizManager()->EraseSegments("BehaviorExploreMarkedCube::InitInternal");
//    for ( const auto& bG : borderGoals )
//    {
//      const NavMemoryMapTypes::Border& b = bG.borderInfo;
//      robot.GetContext()->GetVizManager()->DrawSegment("BehaviorExploreMarkedCube::InitInternal", b.from, b.to, Anki::NamedColors::RED, false, 60.0f);
//      Vec3f centerLine = (b.from + b.to)*0.5f;
//      robot.GetContext()->GetVizManager()->DrawSegment("BehaviorExploreMarkedCube::InitInternal", centerLine, centerLine+b.normal*20.0f, Anki::NamedColors::CYAN, false, 60.0f);
//    }
//
//    // vantage points
//    for ( const auto& p : _currentVantagePoints )
//    {
//      Point3f testOrigin{};
//      Point3f testDir = X_AXIS_3D() * 20.0f;
//      testOrigin = p * testOrigin;
//      testDir = p * testDir;
//      robot.GetContext()->GetVizManager()->DrawSegment("BehaviorExploreMarkedCube::InitInternal", testOrigin, testDir, Anki::NamedColors::GREEN, false, 60.0f);
//    }
//  }
//
//  // we shouldn't be running if we don't have borders/vantage points
//  if ( _currentVantagePoints.empty() ) {
//    PRINT_NAMED_ERROR("BehaviorExploreMarkedCube::InitInternal", "Could not calculate vantage points from borders!");
//    return RESULT_FAIL;
//  }
//  
//  // request the action
//  DriveToPoseAction* driveToPoseAction = new DriveToPoseAction( robot, _currentVantagePoints );
//  _currentActionTag = driveToPoseAction->GetTag();
//  robot.GetActionList().QueueActionNow(driveToPoseAction);
//
//  return Result::RESULT_OK;
//}
//
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//BehaviorExploreMarkedCube::Status BehaviorExploreMarkedCube::UpdateInternal(Robot& robot)
//{
//  // while we are moving towards a vantage point, wait patiently
//  if ( _currentActionTag != ActionConstants::INVALID_TAG )
//  {
//    // PRINT_NAMED_INFO("RSAM", "Waiting for the move to action to finish");
//    return Status::Running;
//  }
//  
//  // done
//  Status retval = Status::Complete;
//  return retval;
//}
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void BehaviorExploreMarkedCube::StopInternal(Robot& robot)
//{
//  _currentActionTag = ActionConstants::INVALID_TAG;
//  robot.GetContext()->GetVizManager()->EraseSegments("BehaviorExploreMarkedCube::InitInternal");
//}
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void BehaviorExploreMarkedCube::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
//{
//  switch(event.GetData().GetTag())
//  {
//    case EngineToGameTag::RobotCompletedAction:
//      HandleActionCompleted( event.GetData().Get_RobotCompletedAction() );
//      break;
//      
//    default:
//      PRINT_NAMED_ERROR("BehaviorExploreMarkedCube.AlwaysHandle.InvalidTag",
//                        "Received unexpected event with tag %hhu.", event.GetData().GetTag());
//      break;
//  }
//}
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void BehaviorExploreMarkedCube::HandleActionCompleted(const ExternalInterface::RobotCompletedAction& msg)
//{
//  // we completed our action, clear the tag so that the main loop knows
//  if ( _currentActionTag == msg.idTag )
//  {
//    if ( !IsRunning() ) {
//      PRINT_NAMED_INFO("BehaviorExploreMarkedCube", "Our action completed while not running.");
//    }
//  
//    _currentActionTag = Util::numeric_cast<uint32_t>((std::underlying_type<ActionConstants>::type)ActionConstants::INVALID_TAG);
//  }
//}
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void BehaviorExploreMarkedCube::PickGoals(Robot& robot, BorderScoreVector& outGoals) const
//{
//  // check assumptions
//  ASSERT_NAMED(nullptr != robot.GetPose().GetParent(), "BehaviorExploreMarkedCube.PickGoals.RobotPoseHasParent");
//  ASSERT_NAMED(robot.GetPose().GetParent()->IsOrigin(), "BehaviorExploreMarkedCube.PickGoals.RobotPoseParentIsOrigin");
//  
//  outGoals.clear();
//
//  // grab borders
//  INavMemoryMap::BorderVector borders;
//  INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
//  memoryMap->CalculateBorders(NavMemoryMapTypes::EContentType::ObstacleCube, typesToExploreFrom, borders);
//  if ( !borders.empty() )
//  {
//    outGoals.reserve(kEmcMaxBorderGoals);
//    for ( const auto& border : borders )
//    {
//      // find where this goal would go with respect to the rest
//      // note we compare the center of the border, not the destination point we will choose to go to
//      const float curDistSQ = (border.GetCenter() - robot.GetPose().GetTranslation()).LengthSq();
//      BorderScoreVector::iterator goalIt = outGoals.begin();
//      while( goalIt != outGoals.end() )
//      {
//        if ( curDistSQ < goalIt->distanceSQ ) {
//          break;
//        }
//        ++goalIt;
//      }
//      
//      // this border should be in the goals, if it's better or if we don't have the max
//      if ( (goalIt != outGoals.end()) || (outGoals.size() < kEmcMaxBorderGoals) )
//      {
//        // if we have the max already, we need to kick out the last one
//        if ( outGoals.size() >= kEmcMaxBorderGoals )
//        {
//          // kicking out the last one invalidates iterators pointing at it, check if we were pointing at last
//          bool newIsLast = (goalIt == outGoals.end()-1);
//          // kick out last one
//          outGoals.pop_back();
//          // if we were pointing at the last one, grab end() now, since we destroyed the last one
//          if ( newIsLast ) {
//            goalIt = outGoals.end();
//          }
//        }
//
//        // add this goal where it should go within the vector (sorted by distance)
//        outGoals.emplace(goalIt, border, curDistSQ);
//      }
//    }
//  }
//}
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void BehaviorExploreMarkedCube::GenerateVantagePoints(Robot& robot, const BorderScoreVector& goals, VantagePointVector& outVantagePoints) const
//{
//  // check assumptions
//  ASSERT_NAMED(nullptr != robot.GetPose().GetParent(), "BehaviorExploreMarkedCube.PickGoals.RobotPoseHasParent");
//  ASSERT_NAMED(robot.GetPose().GetParent()->IsOrigin(), "BehaviorExploreMarkedCube.PickGoals.RobotPoseParentIsOrigin");
//  const Pose3d* origin = robot.GetPose().GetParent();
//
//  // TODO Shouldn't this be asked to the robot instance instead? Otherwise we only support 1 robot size
//  const float robotFwdLen = ROBOT_BOUNDING_X_FRONT + ROBOT_BOUNDING_X_LIFT;
//
//  outVantagePoints.clear();
//  for ( const auto& goal : goals )
//  {
//    const float randomFactor = GetRNG().RandDblInRange(kEmcDistanceMinFactor, kEmcDistanceMaxFactor);
//    const float distanceFromGoal = robotFwdLen * randomFactor;
//    
//    const Point3f vantagePointPos = goal.borderInfo.GetCenter() + (goal.borderInfo.normal * distanceFromGoal);
//    
//    const Vec3f& kFwdVector = X_AXIS_3D();
//    const Vec3f& kRightVector = -Y_AXIS_3D();
//    const Vec3f& kUpVector = Z_AXIS_3D();
//    
//    Vec3f vantagePointDir = -goal.borderInfo.normal;
//    const float fwdAngleCos = DotProduct(vantagePointDir, kFwdVector);
//    const bool isPositiveAngle = (DotProduct(vantagePointDir, kRightVector) >= 0.0f);
//    float rotRads = isPositiveAngle ? -std::acos(fwdAngleCos) : std::acos(fwdAngleCos);
//    
//    // add pose to vector
//    outVantagePoints.emplace_back(rotRads, kUpVector, vantagePointPos, origin);
//  }
//
//}
//
//
//} // namespace Cozmo
//} // namespace Anki
