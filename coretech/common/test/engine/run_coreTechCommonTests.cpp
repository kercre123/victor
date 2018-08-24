#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header
#include <iostream>
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"

Anki::Util::PrintfLoggerProvider* loggerProvider = nullptr;

GTEST_API_ int main(int argc, char * argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  int rc = 0;
  {
    Anki::Util::PrintfLoggerProvider printfLoggerProvider;
    printfLoggerProvider.SetMinLogLevel(Anki::Util::LOG_LEVEL_DEBUG);
    Anki::Util::gLoggerProvider = &printfLoggerProvider;
    rc = RUN_ALL_TESTS();
    Anki::Util::gLoggerProvider = nullptr;
  }
  
  return rc;
}
