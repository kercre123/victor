/**
 * File: CST_ChargerDocking.cpp
 *
 * Author: Matt Michini
 * Created: 11/13/17
 *
 * Description: This is to test that charger docking is working properly.
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#include "simulator/game/cozmoSimTestController.h"

#include "simulator/controllers/shared/webotsHelpers.h"

namespace Anki {
namespace Cozmo {
  
enum class TestState {
  Init,
  ShiftChargerSlightly,
  TestDone
};
  
// ============ Test class declaration ============
class CST_ChargerDocking : public CozmoSimTestController
{
  virtual s32 UpdateSimInternal() override;
  
  TestState _testState = TestState::Init;
};
  
// Register class with factory
REGISTER_COZMO_SIM_TEST_CLASS(CST_ChargerDocking);

// =========== Test class implementation ===========

s32 CST_ChargerDocking::UpdateSimInternal()
{
  switch (_testState) {
    case TestState::Init:
    {
      // Start the charger docking behavior
      using namespace ExternalInterface;
      SendMessage(MessageGameToEngine(ExecuteBehaviorByID("FindAndGoToHome", -1)));
      
      SET_TEST_STATE(ShiftChargerSlightly);
      break;
    }
    case TestState::ShiftChargerSlightly:
    {
      // Wait until the robot is turned around away from the charger
      // and about to dock, then move the charger a tiny bit to force
      // the robot to auto-correct with the cliff sensors
      auto* chargerNode = WebotsHelpers::GetFirstMatchingSceneTreeNode(GetSupervisor(), "VictorCharger").nodePtr;
      auto chargerPose = GetPose3dOfNode(chargerNode);
      const auto& robotPose = GetRobotPoseActual();
      const float distanceAway_mm = ComputeDistanceBetween(chargerPose, robotPose);
      const float angleBetween_deg = (chargerPose.GetRotationAngle<'Z'>() - robotPose.GetRotationAngle<'Z'>()).getDegrees();
      if (distanceAway_mm < 180.f &&
          NEAR(angleBetween_deg, -90.f, 10.f)) {
        auto chargerTranslation = chargerPose.GetTranslation();
        chargerTranslation.x() += 10.f;
        chargerPose.SetTranslation(chargerTranslation);
        SetNodePose(chargerNode, chargerPose);
        SET_TEST_STATE(TestDone);
      }
    }
    case TestState::TestDone:
    {
      const bool onCharger = IsRobotStatus(RobotStatusFlag::IS_ON_CHARGER);
      IF_CONDITION_WITH_TIMEOUT_ASSERT(onCharger, 60.f) {
        StopMovie();
        CST_EXIT();
        break;
      }
    }
  }
  
  return _result;
}


} // end namespace Cozmo
} // end namespace Anki

