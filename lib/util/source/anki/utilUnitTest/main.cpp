//
//  main.cpp
//  native
//
//  Created by damjan stulic on 5/25/14.
//  Copyright (c) 2014 anki. All rights reserved.
//

#include <iostream>
#include "util/helpers/includeGTest.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "tests/utilUnitTestShared.h"


GTEST_API_ int main(int argc, char * argv[])
{
  Anki::Util::PrintfLoggerProvider printfLoggerProvider;
  Anki::Util::gLoggerProvider = &printfLoggerProvider;
  for(int i=1; i<argc; ++i) {
    int newLevel = -1;
    char buf[20];
    if(sscanf(argv[i], "-d%d", &newLevel) == 1) {
      printf("Setting default log level to %d\n", newLevel);
      printfLoggerProvider.SetMinLogLevel((Anki::Util::ILoggerProvider::LogLevel)newLevel);
    }
    else if (sscanf(argv[i], "-timing=%19s", buf) == 1) {
      if (strcmp(buf, "valgrind") == 0) {
        UtilUnitTestShared::SetTimingMode(UtilUnitTestShared::TimingMode::Valgrind);
      }
    }
  }
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
  Anki::Util::gLoggerProvider = nullptr;
}
