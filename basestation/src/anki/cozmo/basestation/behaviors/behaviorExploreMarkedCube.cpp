/**
 * File: behaviorExploreMarkedCube
 *
 * Author: Raul
 * Created: 01/22/16
 *
 * Description: Behavior that looks for a nearby marked cube that Cozmo has not fully explored (ie: seen in
 * all directions), and tries to see the sides that are yet to be discovered.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "behaviorExploreMarkedCube.h"

#include "anki/cozmo/basestation/robot.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {
  
CONSOLE_VAR(uint8_t, kMaxBorderGoals, "BehaviorExploreMarkedCube", 3); // max number of goals to ask the planner
CONSOLE_VAR(uint8_t, kDistanceMinFactor, "BehaviorExploreMarkedCube", 2.0f); // minimum factor applied to the robot size to find destination from border center
CONSOLE_VAR(uint8_t, kDistanceMaxFactor, "BehaviorExploreMarkedCube", 4.0f); // maximum factor applied to the robot size to find destination from border center

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreMarkedCube::BehaviorExploreMarkedCube(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _currentActionTag(Util::numeric_cast<uint32_t>( (std::underlying_type<ActionConstants>::type)ActionConstants::INVALID_TAG))
{

  SubscribeToTags({
    EngineToGameTag::RobotCompletedAction
  });
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreMarkedCube::~BehaviorExploreMarkedCube()
{  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploreMarkedCube::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  const INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
  if ( nullptr == memoryMap ) {
    // should not happen in prod, but at the moment it's null in master
    return false;
  }
  const bool hasBorders = memoryMap->HasBorders(NavMemoryMapTypes::EContentType::ObstacleCube, NavMemoryMapTypes::EContentType::Unknown);
  //return hasBorders;

  if ( hasBorders ) {
    PRINT_NAMED_INFO("RSAM", "IsRunnable YES");
  } else {
    PRINT_NAMED_INFO("RSAM", "IsRunnable NO");
  }
  
  INavMemoryMap::BorderVector borders;
  INavMemoryMap* memoryMapTest = robot.GetBlockWorld().GetNavMemoryMap();
  memoryMapTest->CalculateBorders(NavMemoryMapTypes::EContentType::ObstacleCube, NavMemoryMapTypes::EContentType::Unknown, borders);
  
  BorderScoreVector borderGoals;
  PickGoals(robot, borderGoals);

  // for every goal, pick a target point to look at the cube from there. Those are the goals we will feed the planner
  VantagePointVector vantagePoints;
  GenerateVantagePoints(robot, borderGoals, vantagePoints);

    VizManager::getInstance()->EraseSegments("RSAM2");
    for ( const auto& bG : borderGoals )
    {
      const NavMemoryMapTypes::Border& b = bG.borderInfo;
      VizManager::getInstance()->DrawSegment("RSAM2", b.from, b.to, Anki::NamedColors::RED, false, 60.0f);
      Vec3f centerLine = (b.from + b.to)*0.5f;
      VizManager::getInstance()->DrawSegment("RSAM2", centerLine, centerLine+b.normal*20.0f, Anki::NamedColors::CYAN, false, 60.0f);
    }
  
    // render vantage points - delete me
    VizManager::getInstance()->EraseSegments("BehaviorExploreMarkedCube::GenerateVantagePoints");
    for ( const auto& p : vantagePoints )
    {
      Point3f testOrigin{};
      Point3f testDir = X_AXIS_3D() * 20.0f;
      testOrigin = p * testOrigin;
      testDir = p * testDir;
      VizManager::getInstance()->DrawSegment("BehaviorExploreMarkedCube::GenerateVantagePoints", testOrigin, testDir, Anki::NamedColors::GREEN, false, 60.0f);
    }
  
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorExploreMarkedCube::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
{
//  PickGoals();
//  if ( outGoals.empty() )
//      PRINT_NAMED_ERROR("BehaviorExploreMarkedCube.BadInit", "Behavior was initialized, but no borders where found!");
  return Result::RESULT_OK;
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreMarkedCube::Status BehaviorExploreMarkedCube::UpdateInternal(Robot& robot, double currentTime_sec)
{
  PRINT_NAMED_INFO("RSAM", "Update");
  Status retval = Status::Complete;
  return retval;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorExploreMarkedCube::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
{
  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreMarkedCube::StopInternal(Robot& robot, double currentTime_sec)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreMarkedCube::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotCompletedAction:
      HandleActionCompleted(robot,
        event.GetData().Get_RobotCompletedAction(),
        event.GetCurrentTime());
      break;
      
    default:
      PRINT_NAMED_ERROR("BehaviorExploreMarkedCube.HandleWhileRunning.InvalidTag",
                        "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreMarkedCube::HandleActionCompleted(Robot& robot, const ExternalInterface::RobotCompletedAction& msg, double currentTime_sec)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreMarkedCube::PickGoals(const Robot& robot, BorderScoreVector& outGoals) const
{
  // check assumptions
  ASSERT_NAMED(nullptr != robot.GetPose().GetParent(), "BehaviorExploreMarkedCube.PickGoals.RobotPoseHasParent");
  ASSERT_NAMED(robot.GetPose().GetParent()->IsOrigin(), "BehaviorExploreMarkedCube.PickGoals.RobotPoseParentIsOrigin");
  
  outGoals.clear();

  // grab borders
  INavMemoryMap::BorderVector borders;
  INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
  memoryMap->CalculateBorders(NavMemoryMapTypes::EContentType::ObstacleCube, NavMemoryMapTypes::EContentType::Unknown, borders);
  if ( !borders.empty() )
  {
    outGoals.reserve(kMaxBorderGoals);
    for ( const auto& border : borders )
    {
      // find where this goal would go with respect to the rest
      // note we compare the center of the border, not the destination point we will choose to go to
      const float curDistSQ = (border.GetCenter() - robot.GetPose().GetTranslation()).LengthSq();
      BorderScoreVector::iterator goalIt = outGoals.begin();
      while( goalIt != outGoals.end() )
      {
        if ( curDistSQ < goalIt->distanceSQ ) {
          break;
        }
        ++goalIt;
      }
      
      // this border should be in the goals, if it's better or if we don't have the max
      if ( (goalIt != outGoals.end()) || (outGoals.size() < kMaxBorderGoals) )
      {
        // if we have the max already, we need to kick out the last one
        if ( outGoals.size() >= kMaxBorderGoals )
        {
          // kicking out the last one invalidates iterators pointing at it, check if we were pointing at last
          bool newIsLast = (goalIt == outGoals.end()-1);
          // kick out last one
          outGoals.pop_back();
          // if we were pointing at the last one, grab end() now, since we destroyed the last one
          if ( newIsLast ) {
            goalIt = outGoals.end();
          }
        }

        // add this goal where it should go within the vector (sorted by distance)
        outGoals.emplace(goalIt, border, curDistSQ);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreMarkedCube::GenerateVantagePoints(const Robot& robot, const BorderScoreVector& goals, VantagePointVector& outVantagePoints) const
{
  Util::RandomGenerator rng; // TODO: rsam replay-ability issue

  // TODO Shouldn't this be asked to the robot instance instead? Otherwise we only support 1 robot size
  const float robotFwdLen = ROBOT_BOUNDING_X_FRONT + ROBOT_BOUNDING_X_LIFT;

  outVantagePoints.clear();
  for ( const auto& goal : goals )
  {
    const float randomFactor = rng.RandDblInRange(kDistanceMinFactor, kDistanceMaxFactor);
    const float distanceFromGoal = robotFwdLen * randomFactor;
    
    const Point3f vantagePointPos = goal.borderInfo.GetCenter() + (goal.borderInfo.normal * distanceFromGoal);
    
    const Vec3f& kFwdVector = X_AXIS_3D();
    const Vec3f& kRightVector = -Y_AXIS_3D();
    const Vec3f& kUpVector = Z_AXIS_3D();
    
    Vec3f vantagePointDir = -goal.borderInfo.normal;
    const float fwdAngleCos = DotProduct(vantagePointDir, kFwdVector);
    const bool isPositiveAngle = (DotProduct(vantagePointDir, kRightVector) >= 0.0f);
    float rotRads = isPositiveAngle ? -std::acos(fwdAngleCos) : std::acos(fwdAngleCos);
    
    // add pose to vector
    outVantagePoints.emplace_back(rotRads, kUpVector, vantagePointPos);
  }

}


} // namespace Cozmo
} // namespace Anki
