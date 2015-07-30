#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header
#include <iostream>

GTEST_API_ int main(int argc, char * argv[])
{
  char *path=NULL;
  size_t size = 0;
  path=getcwd(path,size);
  std::cout<<"hello world! I am " << argv[0] << "\nRunning in " << path << "\nLet the testing commence\n\n";
  free(path);
  ::testing::InitGoogleTest(&argc, argv);
 
  return RUN_ALL_TESTS();
  
}