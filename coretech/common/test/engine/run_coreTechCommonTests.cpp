#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header
#include <iostream>
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"

Anki::Util::PrintfLoggerProvider* loggerProvider = nullptr;

GTEST_API_ int main(int argc, char * argv[])
{
  //LEAKING HERE
  loggerProvider = new Anki::Util::PrintfLoggerProvider();
  loggerProvider->SetMinLogLevel(Anki::Util::LOG_LEVEL_DEBUG);
  Anki::Util::gLoggerProvider = loggerProvider;

  char *path=NULL;
  size_t size = 0;
  path=getcwd(path,size);
  std::cout<<"hello world! I am " << argv[0] << "\nRunning in " << path
           << "\nLet the (CoreTechCommon) testing commence\n\n";
  free(path);
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
