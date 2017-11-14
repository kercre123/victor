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
#include "clad/types/behaviorComponent/behaviorTypes.h"

namespace Anki {
namespace Cozmo {
  
enum class TestState {
  Init,
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
      SendMessage(MessageGameToEngine(ExecuteBehaviorByID(BehaviorIDToString(BehaviorID::FindAndGoToHome), -1)));
      
      SET_TEST_STATE(TestDone);
      break;
    }
    case TestState::TestDone:
    {
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(60.f,
                                            !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                            IsRobotStatus(RobotStatusFlag::IS_ON_CHARGER))
      {
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

