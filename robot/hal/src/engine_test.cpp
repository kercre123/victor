
#include "anki/cozmo/robot/hal.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include <stdint.h>

namespace Anki {
namespace Cozmo {

namespace Factory {

  enum enginetest {
    et_NO_TEST,
    et_MIC_TEST,
    et_SPEAKER_TEST,
    et_TEST_COUNT
  };
  
  //lifted from cc_commander.c
  enum {
    ERR_PENDING = -1, //does not return right away
    ERR_OK = 0,
    ERR_UNKNOWN,
    ERR_SYNTAX,
    ERR_SYSTEM,
    ERR_LOCKED    //packout
  };

  int RunEngineTest(uint8_t id, uint8_t args[4])
  {
    using namespace RobotInterface;
    RunFactoryTest msg;
    memcpy(msg.args, args, 4);
    switch (id)
    {
      default:
      case et_NO_TEST:
        return ERR_UNKNOWN;
      case et_MIC_TEST:
        msg.test = FactoryTest::MIC_TEST;
        break;
      case et_SPEAKER_TEST:
        msg.test = FactoryTest::SPEAKER_TEST;
        break;
    }
    SendMessage(msg);
    return ERR_OK;

    //Future possible TODO:
    // Provide a method for engine to send a TestResult message
    // back to this process, containing a status code and 32 chars.

    // return a negative value from this process, which causes
    // CCC to wait.
    // When the TestResult arrives, do a `ccc_test_result(status, str)`
    // to report final status
  }
}
}
}


extern "C" {
  int engine_test_run(uint8_t id, uint8_t args[4])
  {
    return Anki::Cozmo::Factory::RunEngineTest(id, args);
  }
}
