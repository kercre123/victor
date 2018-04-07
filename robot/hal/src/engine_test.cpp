
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

  int RunEngineTest(uint8_t id, uint8_t args[4])
  {
    switch (id) {
    default:
      return -1;
    case et_NO_TEST:

    case et_MIC_TEST:
      //SendCladMessage(RunMicTest)
      break;
    case et_SPEAKER_TEST:
      //SendCladMessage(RunSpeakerTest)
      break;
    }
    return 0;

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
