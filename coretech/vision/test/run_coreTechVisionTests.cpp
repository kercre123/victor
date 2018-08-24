#include "coretech/common/engine/math/matrix_impl.h"

#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"

#include "gtest/gtest.h"

using namespace Anki;

int main(int argc, char ** argv)
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
