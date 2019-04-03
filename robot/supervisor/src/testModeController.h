/**
 * File: testModeController.h
 *
 * Author: Kevin Yoon (kevin)
 * Created: 11/13/2013
 *
 *
 * Description:
 *
 *   Controller for executing various test modes (for development).
 *
 * Copyright: Anki, Inc. 2013
 *
 **/


#ifndef TEST_MODE_CONTROLLER_H_
#define TEST_MODE_CONTROLLER_H_

#include "coretech/common/shared/types.h"
#include "clad/types/robotTestModes.h"

#ifdef USES_CLAD_CPPLITE
#define CLAD(ns) CppLite::Anki::Vector::ns
#else
#define CLAD(ns) ns
#endif

namespace Anki {
  namespace Vector {
    namespace TestModeController {
      
      // Sets up controller to run specified test mode
      Result Start(const CLAD(TestMode) mode, s32 p1 = 0, s32 p2 = 0, s32 p3 = 0);

      Result Update();
      
      CLAD(TestMode) GetMode();
      
    } // namespace TestModeController
  } // namespace Vector
} // namespace Anki

#undef CLAD

#endif // TEST_MODE_CONTROLLER_H_
