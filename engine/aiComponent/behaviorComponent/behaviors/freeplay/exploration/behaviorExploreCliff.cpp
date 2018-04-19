// read note in header about commented out code 

///**
// * File: behaviorExploreCliff
// *
// * Author: Raul
// * Created: 02/03/16
// *
// * Description: Behavior that looks for nearby areas marked as cliffs and tries to follow along their line
// * in order to identify the borders or the platform the robot is currently on.
// *
// * Copyright: Anki, Inc. 2016
// *
// **/
//#include "behaviorExploreCliff.h"
//
//#include "engine/actions/driveToActions.h"
//#include "engine/navMemoryMap/quadData/navMemoryMapQuadData_Cliff.h"
//#include "engine/robot.h"
//
//#include "clad/externalInterface/messageEngineToGame.h"
//
//#include "util/console/consoleInterface.h"
//#include "util/logging/logging.h"
//#include "util/math/numericCast.h"
//#include "util/random/randomGenerator.h"
//
//namespace Anki {
//namespace Cozmo {
//  
//CONSOLE_VAR(uint8_t, kEcMaxBorderGoals, "BehaviorExploreCliff", 1); // max number of goals to ask the planner
//CONSOLE_VAR(float, kEcDistanceMinWidthFactor, "BehaviorExploreCliff", 0.25f); // minimum factor applied to the robot size to find destination from border center
//CONSOLE_VAR(float, kEcDistanceMaxWidthFactor, "BehaviorExploreCliff", 0.35f); // maximum factor applied to the robot size to find destination from border center
//CONSOLE_VAR(bool, kEcDrawDebugInfo, "BehaviorExploreCliff", true); // if set to true the behavior renders debug privimitives
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//namespace {
//
//// This is the configuration of memory map types that this behavior checks against. If a type
//// is set to true, it means that if they are a border to a cliff, we will explore that cliff
//constexpr MemoryMapTypes::FullContentArray typesToExploreFrom =
//{
//  {MemoryMapTypes::EContentType::Unknown               , true },
//  {MemoryMapTypes::EContentType::ClearOfObstacle       , true },
//  {MemoryMapTypes::EContentType::ClearOfCliff          , false},
//  {MemoryMapTypes::EContentType::ObstacleCube          , false},
//  {MemoryMapTypes::EContentType::ObstacleCubeRemoved   , false},
//  {MemoryMapTypes::EContentType::ObstacleCharger       , false},
//  {MemoryMapTypes::EContentType::ObstacleChargerRemoved, false},
//  {MemoryMapTypes::EContentType::ObstacleUnrecognized  , false},
//  {MemoryMapTypes::EContentType::Cliff                 , false},
//  {MemoryMapTypes::EContentType::InterestingEdge       , false},
//  {MemoryMapTypes::EContentType::NotInterestingEdge    , false}
//};
//static_assert(MemoryMapTypes::IsSequentialArray(typesToExploreFrom),
//  "This array does not define all types once and only once.");
//  
//};
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//BehaviorExploreCliff::BehaviorExploreCliff(Robot& robot, const Json::Value& config)
//: ICozmoBehavior(behaviorExternalInterface, config)
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
//BehaviorExploreCliff::~BehaviorExploreCliff()
//{  
//}
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//bool BehaviorExploreCliff::WantsToBeActivatedBehavior() const
//{
//  const INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
//  if ( nullptr == memoryMap ) {
//    return false;
//  }
//  
//  const bool hasBorders = memoryMap->HasBorders(MemoryMapTypes::EContentType::Cliff, typesToExploreFrom);
//  return hasBorders;
//}
//  
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void BehaviorExploreCliff::OnBehaviorActivated()
//{
//  // select borders we want to visit
//  BorderScoreVector borderGoals;
//  PickGoals(robot, borderGoals);
//
//  // for every goal, pick a target point to look at the cube from there. Those are the goals we will feed the planner
//  GenerateVantagePoints(robot, borderGoals, _currentVantagePoints);
//
//  // debugging
//  if ( kEcDrawDebugInfo )
//  {
//    // border goals
//    robot.GetContext()->GetVizManager()->EraseSegments("BehaviorExploreCliff::InitInternal");
//    for ( const auto& bG : borderGoals )
//    {
//      const MemoryMapTypes::Border& b = bG.borderInfo;
//      robot.GetContext()->GetVizManager()->DrawSegment("BehaviorExploreCliff::InitInternal", b.from, b.to, Anki::NamedColors::DARKGREEN, false, 60.0f);
//      Vec3f centerLine = (b.from + b.to)*0.5f;
//      robot.GetContext()->GetVizManager()->DrawSegment("BehaviorExploreCliff::InitInternal", centerLine, centerLine+b.normal*5.0f, Anki::NamedColors::GREEN, false, 60.0f);
//    }
//
//    // vantage points
//    for ( const auto& p : _currentVantagePoints )
//    {
//      Point3f testOrigin{};
//      Point3f testDir = X_AXIS_3D() * 10.0f;
//      testOrigin = p * testOrigin;
//      testDir = p * testDir;
//      robot.GetContext()->GetVizManager()->DrawSegment("BehaviorExploreCliff::InitInternal", testOrigin, testDir, Anki::NamedColors::CYAN, false, 61.0f);
//    }
//  }
//
//  // we shouldn't be running if we don't have borders/vantage points
//  if ( _currentVantagePoints.empty() ) {
//    PRINT_NAMED_ERROR("BehaviorExploreCliff::InitInternal", "Could not calculate vantage points from borders!");
//    return RESULT_FAIL;
//  }
//  
//  // request the action
//  DriveToPoseAction* driveToPoseAction = new DriveToPoseAction( _currentVantagePoints );
//  _currentActionTag = driveToPoseAction->GetTag();
//  robot.GetActionList().QueueActionNow(driveToPoseAction);
//
//  
//}
//
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void BehaviorExploreCliff::BehaviorUpdate()
//{
//  if(!IsActivated()){
//    return;
//  }
//  // while we are moving towards a vantage point, wait patiently
//  if ( _currentActionTag != ActionConstants::INVALID_TAG )
//  {
//    // PRINT_NAMED_INFO("RSAM", "Waiting for the move to action to finish");
//    return;
//  }
//  
//  CancelSelf();
//}
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void BehaviorExploreCliff::OnBehaviorDeactivated()
//{
//  _currentActionTag = ActionConstants::INVALID_TAG;
//  robot.GetContext()->GetVizManager()->EraseSegments("BehaviorExploreCliff::InitInternal");
//}
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void BehaviorExploreCliff::AlwaysHandleInScope(const EngineToGameEvent& event)
//{
//  switch(event.GetData().GetTag())
//  {
//    case EngineToGameTag::RobotCompletedAction:
//      HandleActionCompleted( event.GetData().Get_RobotCompletedAction() );
//      break;
//      
//    default:
//      PRINT_NAMED_ERROR("BehaviorExploreCliff.AlwaysHandle.InvalidTag",
//                        "Received unexpected event with tag %hhu.", event.GetData().GetTag());
//      break;
//  }
//}
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void BehaviorExploreCliff::HandleActionCompleted(const ExternalInterface::RobotCompletedAction& msg)
//{
//  // we completed our action, clear the tag so that the main loop knows
//  if ( _currentActionTag == msg.idTag )
//  {
//    if ( !IsActivated() ) {
//      PRINT_NAMED_INFO("BehaviorExploreCliff", "Our action completed while not running.");
//    }
//  
//    _currentActionTag = Util::numeric_cast<uint32_t>((std::underlying_type<ActionConstants>::type)ActionConstants::INVALID_TAG);
//  }
//}
//
//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void BehaviorExploreCliff::PickGoals(Robot& robot, BorderScoreVector& outGoals) const
//{
//  // check assumptions
//  ASSERT_NAMED(nullptr != robot.GetPose().GetParent(), "BehaviorExploreCliff.PickGoals.RobotPoseHasParent");
//  ASSERT_NAMED(robot.GetPose().GetParent()->IsOrigin(), "BehaviorExploreCliff.PickGoals.RobotPoseParentIsOrigin");
//  
//  outGoals.clear();
//
//  // grab borders
//  INavMemoryMap::BorderVector borders;
//  INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
//  memoryMap->CalculateBorders(MemoryMapTypes::EContentType::Cliff, typesToExploreFrom, borders);
//  if ( !borders.empty() )
//  {
//    outGoals.reserve(kEcMaxBorderGoals);
//    for ( const auto& border : borders )
//    {
//      // grab extra info from the border
//      const INavMemoryMapQuadData* extraInfoPtr = border.extraData.get();
//      const NavMemoryMapQuadData_Cliff* extraInfo = INavMemoryMapQuadDataCast<const NavMemoryMapQuadData_Cliff>( extraInfoPtr );
//      ASSERT_NAMED(nullptr!=extraInfo, "BehaviorExploreCliff.PickGoals.CliffBorderWithoutExtraInfo");
//      if ( nullptr == extraInfo  ) {
//        // in shipping, ignore this border if it ever happened
//        continue;
//      }
//    
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
//      if ( (goalIt != outGoals.end()) || (outGoals.size() < kEcMaxBorderGoals) )
//      {
//        // if we have the max already, we need to kick out the last one
//        if ( outGoals.size() >= kEcMaxBorderGoals )
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
//void BehaviorExploreCliff::GenerateVantagePoints(Robot& robot, const BorderScoreVector& goals, VantagePointVector& outVantagePoints) const
//{
//  // check assumptions
//  ASSERT_NAMED(nullptr != robot.GetPose().GetParent(), "BehaviorExploreCliff.PickGoals.RobotPoseHasParent");
//  ASSERT_NAMED(robot.GetPose().GetParent()->IsOrigin(), "BehaviorExploreCliff.PickGoals.RobotPoseParentIsOrigin");
//  const Pose3d* origin = robot.GetPose().GetParent();
//
//  // TODO Shouldn't this be asked to the robot instance instead? Otherwise we only support 1 robot size
//  const float robotWidth = ROBOT_BOUNDING_Y;
//
//  outVantagePoints.clear();
//  for ( const auto& goal : goals )
//  {
//    const float randomFactor = GetRNG().RandDblInRange(kEcDistanceMinWidthFactor, kEcDistanceMaxWidthFactor);
//    const float distanceFromGoal = robotWidth * randomFactor;
//    
//    const Point3f vantagePointPos = goal.borderInfo.GetCenter() + (goal.borderInfo.normal * distanceFromGoal);
//    
//    const Vec3f& kFwdVector = X_AXIS_3D();
//    const Vec3f& kRightVector = -Y_AXIS_3D();
//    const Vec3f& kUpVector = Z_AXIS_3D();
//    
//    // grab extra info from the border
//    const INavMemoryMapQuadData* extraInfoPtr = goal.borderInfo.extraData.get();
//    const NavMemoryMapQuadData_Cliff* extraInfo = INavMemoryMapQuadDataCast<const NavMemoryMapQuadData_Cliff>( extraInfoPtr );
//    ASSERT_NAMED(nullptr!=extraInfo, "BehaviorExploreCliff.GenerateVantagePoints.CliffBorderWithoutExtraInfo");
//    
//    // use directionality from the extra info to point in the same direction
// NOTE: directionality is replaced by _pose
//    Vec3f vantagePointDir{extraInfo->directionality.x(), extraInfo->directionality.y(), 0.0f};
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
